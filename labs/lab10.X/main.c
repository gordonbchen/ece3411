#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "uart.h"

// stored in Flash (program memory) not RAM.
// 10-bit resolution: [0, 1023]
// centered at 512 (DC offset), 24-995
#define N_SINES 64
const uint16_t sine_table[N_SINES] PROGMEM = {
    512, 562, 612, 660, 707, 752, 794, 833, 868, 900, 927, 950, 968,
    982, 991, 995, 995, 991, 982, 968, 950, 927, 900, 868, 833, 794,
    752, 707, 660, 612, 562, 512, 461, 411, 363, 316, 271, 229, 190,
    155, 123, 95,  72,  53,  39,  30,  25,  24,  28,  37,  50,  68,
    90,  116, 146, 180, 218, 259, 303, 350, 399, 449, 500, 551};

void init_DAC() {
    // OUTEN -> buffered output at PD6
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    // voltage = VDD * (value / 1023)
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;
}

void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

void init_timer() {
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
    while (RTC.STATUS & RTC_PERBUSY_bm) {
    }
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

#define IN_BUF_MAXLEN 10
volatile char in_buf[IN_BUF_MAXLEN];
volatile int buf_head = 0;
volatile int buf_tail = 0;
volatile int buf_print_tail = 0;

volatile int percent_amp = 100;
volatile int freq_hz = 10;

volatile int asking_freq_or_amp = 1;
volatile int freq_mode = 1;
volatile int ask_question = 1;

void init_usart() {
    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
    sei();
}

ISR(USART3_RXC_vect) {
    in_buf[buf_tail] = USART3.RXDATAL;
    buf_tail = (buf_tail + 1) % IN_BUF_MAXLEN;
}

// read an int of max digits from the circular buffer.
// returns 0 on fail, 1 if success.
int read_int(int max_digits, int *num) {
    int success = 0;

    *num = 0;

    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    int idx = buf_head;

    for (int i = 0; i < max_digits; ++i) {
        if (idx == prev) {
            break;
        }

        int v = in_buf[idx] - '0';
        if ((v < 0) || (v > 9)) {
            break;
        }
        *num = (*num * 10) + v;

        success = 1;
        idx = (idx + 1) % IN_BUF_MAXLEN;
    }
    return success;
}

void handle_input() {
    while (buf_print_tail != buf_tail) {
        if (in_buf[buf_print_tail] == '\r') {
            putchar('\n');
        }
        putchar(in_buf[buf_print_tail]);
        buf_print_tail = (buf_print_tail + 1) % IN_BUF_MAXLEN;
    }

    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    if (in_buf[prev] != '\r') {
        return;
    }

    if (asking_freq_or_amp) {
        if (in_buf[buf_head] == 'F') {
            freq_mode = 1;
            asking_freq_or_amp = 0;
        } else if (in_buf[buf_head] == 'A') {
            freq_mode = 0;
            asking_freq_or_amp = 0;
        }
    } else {
        int new_num;
        if (freq_mode && read_int(4, &new_num)) {
            if ((new_num >= 10) && (new_num <= 1000)) {
                freq_hz = new_num;
                asking_freq_or_amp = 1;
            }
        } else if ((!freq_mode) && read_int(3, &new_num)) {
            if ((new_num >= 10) && (new_num <= 100)) {
                percent_amp = new_num;
                asking_freq_or_amp = 1;
            }
        }
    }

    buf_head = buf_tail;
    ask_question = 1;
}

void my_delay_us(int us) {
    for (int i = 0; i < us; ++i) {
        _delay_us(1);
    }
}

uint16_t calc_scaled_sine(uint16_t x, int percent_amp_scale) {
    int16_t centered = (int16_t)x - 512;
    int32_t prod = (int32_t)centered * (int32_t)percent_amp_scale;
    prod += (prod >= 0 ? 50 : -50);
    int32_t out = (prod / 100) + 512;
    if (out < 0) {
        out = 0;
    }
    else if (out >= 1024) {
        out = 1023;
    }
    return (uint16_t)out;
}

int main(void) {
    init_clock();
    init_usart();
    init_DAC();

    uint8_t i = 0;
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
                if (freq_mode) {
                    printf("Frequency (10-1000Hz): ");
                } else {
                    printf("Percent Amplitude (10-100%%): ");
                }
            }
        }

        if (status_timer_ms == STATUS_TIME_MS) {
            status_timer_ms = 0;
            printf("Freq = %d Hz, %% Amplitude = %d%% \n", freq_hz, percent_amp);
        }

        // read value from lookup table (in program memory)
        uint16_t sin_val = pgm_read_word(&sine_table[i]);
        sin_val = calc_scaled_sine(sin_val, percent_amp);

        // DATAL[7:6] = DACn.DATA[1:0]
        // DATAH = DACn.DATA[1:0]
        DAC0.DATAL = (sin_val & 0x03) << 6;
        DAC0.DATAH = (sin_val >> 2) & 0xFF;

        i = (i + 1) % N_SINES;

        my_delay_us(1000000 / (64 * freq_hz));
    }

    return 0;
}
