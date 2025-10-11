#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "uart.h"

// -------------------------------DAC INIT------------------------------------
#define N_SINES 64
const uint16_t sine_table[N_SINES] PROGMEM = {
    512, 562, 612, 660, 707, 752, 794, 833, 868, 900, 927, 950, 968,
    982, 991, 995, 995, 991, 982, 968, 950, 927, 900, 868, 833, 794,
    752, 707, 660, 612, 562, 512, 461, 411, 363, 316, 271, 229, 190,
    155, 123, 95,  72,  53,  39,  30,  25,  24,  28,  37,  50,  68,
    90,  116, 146, 180, 218, 259, 303, 350, 399, 449, 500, 551};

static inline void init_DAC(void) {
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;
}

// --------------------------------CLOCK INIT-------------------------------
static inline void init_clock(void) {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

// -----------------------------RTC INIT---------------------------------------
static inline void init_rtc(void) {
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
    while (RTC.STATUS & RTC_PERBUSY_bm);
    RTC.PER = 32768 / 1000;
    RTC.INTCTRL = RTC_OVF_bm;
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;
}

const int STATUS_TIME_MS = 5000;
volatile int status_timer_ms = 0;

ISR(RTC_CNT_vect) {
    if (status_timer_ms < STATUS_TIME_MS) {
        ++status_timer_ms;
    }
    RTC.INTFLAGS = RTC_OVF_bm;
}

// ------------------------------TCA0 INIT ---------------------------------
// max clock period = (1 / 1000 hz) / (64 sin_samples) = 15.625 us
// at div16, perbuf 15, we get (16 / 16 MHz) * 16 = 16 us period
// FS_HZ = 1 / 16us = 62500 Hz
#define FS_HZ 31250UL  // ummm...alright man. this is so wrong.
static inline void init_timer(void) {
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.PERBUF = 15;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

// ------------------------------Amplitude scaling----------------------------
static inline uint16_t calc_scaled_sine(uint16_t x, int percent_amp_scale) {
    int16_t centered = (int16_t)x - 512;
    int32_t prod = (int32_t)centered * (int32_t)percent_amp_scale;
    prod += (prod >= 0 ? 50 : -50);
    int32_t out = (prod / 100) + 512;
    if (out < 0) out = 0;
    else if (out >= 1024) out = 1023;
    return (uint16_t)out;
}

// ------------------------------DDS state------------------------------------
#define PHASE_BITS 32
#define INDEX_BITS 6                     // log2(N_SINES)
#define PHASE_INDEX_SHIFT (PHASE_BITS - INDEX_BITS)

volatile int freq_hz = 1000;               // 10..1000 Hz via UART
volatile int percent_amp = 75;           // 10..100 %

static uint16_t sine_scaled[N_SINES];
static inline void rebuild_scaled_table(int percent_amp_scale) {
    for (uint8_t i = 0; i < N_SINES; ++i) {
        uint16_t s = pgm_read_word(&sine_table[i]);
        sine_scaled[i] = calc_scaled_sine(s, percent_amp_scale);
    }
}

static inline uint32_t compute_phase_inc(int f_hz) {
    // ?? = round( f_out * 2^PHASE_BITS / f_s )
    // use 64-bit math for the division, done outside ISR
    uint64_t num = ((uint64_t)f_hz << PHASE_BITS) + (FS_HZ / 2);
    return (uint32_t)(num / FS_HZ);
}

volatile uint32_t phase_acc = 0;
volatile uint32_t phase_inc = 0;

// ------------------------------ISR (DDS)-------------------------------------
ISR(TCA0_OVF_vect) {
    // Advance phase
    uint32_t phi = phase_acc + phase_inc;
    phase_acc = phi;

    // Upper bits select the table entry
    uint8_t idx = (uint8_t)(phi >> PHASE_INDEX_SHIFT);  // 0..63

    uint16_t y = sine_scaled[idx];

    // DAC: 10-bit value across DATAL[7:6] (bits 1:0) and DATAH[7:0] (bits 9:2)
    DAC0.DATAL = (uint8_t)((y & 0x03) << 6);
    DAC0.DATAH = (uint8_t)((y >> 2) & 0xFF);

    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

// ----------------------------------USART INIT-------------------------------
static inline void init_usart(void) {
    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
}

#define IN_BUF_MAXLEN 10
volatile char in_buf[IN_BUF_MAXLEN];
volatile int buf_head = 0;
volatile int buf_tail = 0;
volatile int buf_print_tail = 0;

volatile int asking_freq_or_amp = 1;
volatile int freq_mode = 1;
volatile int ask_question = 1;

ISR(USART3_RXC_vect) {
    in_buf[buf_tail] = USART3.RXDATAL;
    buf_tail = (buf_tail + 1) % IN_BUF_MAXLEN;
}

// read an int of max digits from the circular buffer.
// returns 0 on fail, 1 if success.
static int read_int(int max_digits, int *num) {
    int success = 0;
    *num = 0;

    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    int idx = buf_head;

    for (int i = 0; i < max_digits; ++i) {
        if (idx == prev) break;

        int v = in_buf[idx] - '0';
        if ((v < 0) || (v > 9)) break;

        *num = (*num * 10) + v;
        success = 1;
        idx = (idx + 1) % IN_BUF_MAXLEN;
    }
    return success;
}

static void handle_input(void) {
    while (buf_print_tail != buf_tail) {
        if (in_buf[buf_print_tail] == '\r') putchar('\n');
        putchar(in_buf[buf_print_tail]);
        buf_print_tail = (buf_print_tail + 1) % IN_BUF_MAXLEN;
    }

    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    if (in_buf[prev] != '\r') return;

    if (asking_freq_or_amp) {
        if (in_buf[buf_head] == 'F') { freq_mode = 1; asking_freq_or_amp = 0; }
        else if (in_buf[buf_head] == 'A') { freq_mode = 0; asking_freq_or_amp = 0; }
    } else {
        int new_num;
        if (freq_mode && read_int(4, &new_num)) {
            if ((new_num >= 10) && (new_num <= 1000)) {
                freq_hz = new_num;
                phase_inc = compute_phase_inc(freq_hz);
                asking_freq_or_amp = 1;
            }
        } else if ((!freq_mode) && read_int(3, &new_num)) {
            if ((new_num >= 10) && (new_num <= 100)) {
                percent_amp = new_num;
                rebuild_scaled_table(percent_amp);
                asking_freq_or_amp = 1;
            }
        }
    }

    buf_head = buf_tail;
    ask_question = 1;
}

// ------------------------------------MAIN------------------------------------
int main(void) {
    init_clock();
    init_usart();
    init_DAC();
    init_rtc();
    init_timer();
    sei();

    rebuild_scaled_table(percent_amp);
    phase_inc = compute_phase_inc(freq_hz);

    while (1) {
        if (buf_print_tail != buf_tail) {
            cli();
            handle_input();
            sei();
        }

        if (ask_question) {
            ask_question = 0;
            if (asking_freq_or_amp) {
                printf("(F)requency or (A)mplitude: ");
            } else {
                if (freq_mode) printf("Frequency (10-1000Hz): ");
                else           printf("Percent Amplitude (10-100%%): ");
            }
        }

        if (status_timer_ms == STATUS_TIME_MS) {
            status_timer_ms = 0;
            printf("Freq = %d Hz, %% Amplitude = %d%% \n", freq_hz, percent_amp);
        }
    }
    return 0;
}
