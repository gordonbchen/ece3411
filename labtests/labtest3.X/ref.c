#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "uart.h"


void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}


void init_adc() {
    ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;    // input from PE0.
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;     // CLK_ADC = 1 MHz.
    ADC0.CTRLD = ADC_INITDLY_DLY16_gc;   // delay 16 CLK_ADC cycles (16 us).
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;   // use VDD (3.3V) as reference voltage.
    ADC0.CTRLA = ADC_ENABLE_bm;          // enable ADC.
    ADC0.COMMAND = ADC_STCONV_bm;        // start conversion.
}

float get_voltage() {
    while (ADC0.COMMAND & ADC_STCONV_bm);  // wait until conversion done.
    // (Ain / max 12-bit value) * 3.3.
    float voltage = ((float) ADC0.RES / 4096.0) * 3.3;
    ADC0.COMMAND = ADC_STCONV_bm;
    return voltage;
}


#define N_SINES 64
const uint16_t sines[N_SINES] PROGMEM = {
    512, 562, 612, 660, 707, 752, 794, 833, 868, 900, 927, 950, 968,
    982, 991, 995, 995, 991, 982, 968, 950, 927, 900, 868, 833, 794,
    752, 707, 660, 612, 562, 512, 461, 411, 363, 316, 271, 229, 190,
    155, 123, 95,  72,  53,  39,  30,  25,  24,  28,  37,  50,  68,
    90,  116, 146, 180, 218, 259, 303, 350, 399, 449, 500, 551};

uint16_t scaled_sines[N_SINES] = {
    512, 562, 612, 660, 707, 752, 794, 833, 868, 900, 927, 950, 968,
    982, 991, 995, 995, 991, 982, 968, 950, 927, 900, 868, 833, 794,
    752, 707, 660, 612, 562, 512, 461, 411, 363, 316, 271, 229, 190,
    155, 123, 95,  72,  53,  39,  30,  25,  24,  28,  37,  50,  68,
    90,  116, 146, 180, 218, 259, 303, 350, 399, 449, 500, 551};
volatile int sine_idx = 0;

uint16_t scale_amp(uint16_t x, float scaling) {
    int centered = (int) x - 512;
    int16_t scaled = (int16_t) ((float) centered * scaling) + 512;
    if (scaled < 0) {scaled = 0;}
    else if (scaled > 1023) {scaled = 1023;}
    return (uint16_t) scaled;
}

void calc_scaled_sines(int percent) {
    float scaling = (float) percent / 100.0;
    uint16_t x;
    for (int i = 0; i < N_SINES; ++i) {
        x = pgm_read_word(&sines[i]);
         scaled_sines[i] = scale_amp(x, scaling);
    }
}

volatile int freq_hz = 440;
volatile int percent_amp = 20;

void init_dac() {
    // DAC0 outputs to PD6.
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;

    calc_scaled_sines(percent_amp);
}


int calc_per() {
    // NOTE: assumes DIV64.
    // (DIV / F_CPU) * PER = (1 / (freq_hz * N_SINES))
    // PER = F_CPU / (DIV * freq_hz * N_SINES)
    return (int) (((int32_t) F_CPU) / ((int32_t) freq_hz * N_SINES * 64));
}

// T = 1 / (440 Hz * 64 sine samples) = 35.5 us.
// 16MHz clock, div64 -> 4us tick. PER = 8 -> 36us period.
// 440 Hz. 
void init_timer() {
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.PERBUF = calc_per();
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

ISR(TCA0_OVF_vect) {
    // uint16_t y = pgm_read_word(&sines[sine_idx]);
    uint16_t y = scaled_sines[sine_idx];
    sine_idx = (sine_idx + 1) % N_SINES;

    // 10-bit DAC value.
    // upper 8 in DATAH, lower 2 in DATAL[7:6].
    DAC0.DATAL = (uint8_t) ((y & 0x03) << 6);
    DAC0.DATAH = (uint8_t) ((y >> 2) & 0xFF);

    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}


void write_str_eeprom(char *str, int strlen) {
    for (int i = 0; i < strlen; ++i) {
        eeprom_write_byte((uint8_t*) i, str[i]);
        _delay_ms(10);
    }
}

void read_str_eeprom(char *str, int strlen) {
    for (int i = 0; i < strlen; ++i) {
         str[i] = eeprom_read_byte((uint8_t*) i);
    }
}


void init_usart() {
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
int read_int(int max_digits, int *num) {
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

void handle_input() {
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
    }
    else {
        int new_num;
        if (freq_mode && read_int(4, &new_num)) {
            if ((new_num >= 10) && (new_num <= 1000)) {
                freq_hz = new_num;
                TCA0.SINGLE.PERBUF = calc_per();
                asking_freq_or_amp = 1;
            }
        }
        else if ((!freq_mode) && read_int(3, &new_num)) {
            if ((new_num >= 10) && (new_num <= 100)) {
                percent_amp = new_num;
                calc_scaled_sines(percent_amp);
                asking_freq_or_amp = 1;
            }
        }
    }
    buf_head = buf_tail;
    ask_question = 1;
}


int main() {
    init_clock();
    init_adc();
    init_dac();
    init_timer();
    init_usart();
    sei();

    _delay_ms(3000);

    int prev_eeprom = eeprom_read_byte(0) != 0xFF;
    if (!prev_eeprom) {
        write_str_eeprom("eeprom", 7);
        printf("wrote to eeprom.\n");
    }
    char eeprom_text[7];
    read_str_eeprom(eeprom_text, 7);
    printf("stored eeprom str: %s\n", eeprom_text);

    while (1) {
        if (buf_print_tail != buf_tail) {
            cli();
            handle_input();
            sei();
        }

        if (ask_question) {
            printf("Voltage = %.3f\t\tFreq = %d Hz\t\tAmp = %d%%", get_voltage(), freq_hz, percent_amp);
            printf("\t\tPER = %d\n", TCA0.SINGLE.PERBUF);
            if (asking_freq_or_amp) {printf("(F)requency or (A)mplitude percentage: ");}
            else {
                if (freq_mode) {printf("Frequency: ");}
                else {printf("Amplitude percentage: ");}
            }
            ask_question = 0;
        }
    }
}


// const int VOLTAGE_PERIOD_TIME_MS = 500;
// volatile int voltage_timer_ms = 0;
// volatile int printing_voltage = 1;
// volatile int print_voltage_status = 0;
//
// ISR(RTC_CNT_vect) {
//     if (printing_voltage && (voltage_timer_ms < VOLTAGE_PERIOD_TIME_MS)) {
//         ++voltage_timer_ms;
//     }
//     RTC.INTFLAGS |= RTC_OVF_bm;
// }
//
//
// void init_gpio() {
//     VPORTB.OUT = 0;
//     PORTB.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
// }
//
// ISR(PORTB_PORT_vect) {
//     printing_voltage = !printing_voltage;
//     print_voltage_status = 1;
//     PORTB.INTFLAGS = PIN5_bm;
// }
//
// init_rtc();
// init_gpio();
//
// if (print_voltage_status) {
//     if (printing_voltage) {printf("Printing voltage.\n");}
//     else {printf("Paused, waiting to resume printing voltage.\n");}
//     print_voltage_status = 0;
// }
// if (printing_voltage && (voltage_timer_ms >= VOLTAGE_PERIOD_TIME_MS)) {
//     printf("voltage: %.3f V\n", get_voltage());
//     voltage_timer_ms = 0;
// }
