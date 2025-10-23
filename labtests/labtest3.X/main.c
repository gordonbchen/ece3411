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

void init_rtc() {
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
    while (RTC.STATUS & RTC_PERBUSY_bm);
    RTC.PER = 32768 / 1000;
    RTC.INTCTRL = RTC_OVF_bm;
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;
}

float get_voltage() {
    while (ADC0.COMMAND & ADC_STCONV_bm);  // wait until conversion done.
    // (Ain / max 12-bit value) * 3.3.
    float voltage = ((float) ADC0.RES / 4096.0) * 3.3;
    ADC0.COMMAND = ADC_STCONV_bm;
    return voltage;
}

 const int VOLTAGE_PERIOD_TIME_MS = 10;
 volatile int voltage_timer_ms = 0;
 
 const int PRINT_PERIOD_TIME_MS = 3000;
 volatile int print_timer_ms = 0;
 
 const int PRINT2_PERIOD_TIME_MS = 2000;
 volatile int print2_timer_ms = 0;
 
volatile int sweep_timer_ms = 0;
volatile int mode_sweep = 0;
volatile int freq_hz = 10;
 
#define N_VOLTAGES 10
 volatile float voltages[N_VOLTAGES];
 volatile int voltage_idx = 0;
 volatile float avg_voltage = 0;

 ISR(RTC_CNT_vect) {
     if (voltage_timer_ms < VOLTAGE_PERIOD_TIME_MS) {
         ++voltage_timer_ms;
     }
     else {
        voltages[voltage_idx] = get_voltage();
        if (voltage_idx == (N_VOLTAGES - 1)) {
            avg_voltage = 0;
            for (int i = 0; i < N_VOLTAGES; ++i) {
               avg_voltage += voltages[i] / 10.0;
           }
        }
        voltage_idx = (voltage_idx + 1) % N_VOLTAGES;
        voltage_timer_ms = 0;
     }
     
     if (print_timer_ms < PRINT_PERIOD_TIME_MS) {
         ++print_timer_ms;
     }
     if (print2_timer_ms < PRINT2_PERIOD_TIME_MS) {
         ++print2_timer_ms;
     }
     
    if (mode_sweep) {
        ++sweep_timer_ms;
        if (sweep_timer_ms < 5000) {
            freq_hz = 10 + ((18 * (int32_t) sweep_timer_ms) / 1000);
        }
        else {
            freq_hz = 100 - ((18 * (int32_t) sweep_timer_ms) / 1000) + 90;
        }
        if ((sweep_timer_ms / 1000) > 10) {
            sweep_timer_ms = 0;
            freq_hz = 10;
        }
    }
     
     RTC.INTFLAGS |= RTC_OVF_bm;
 }
 
 
 void init_gpio() {
     VPORTD.DIR = 0b11111111;
     VPORTC.DIR = 0b11111111;
     PORTB.DIRCLR = 0b0010100;
     PORTB.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
     PORTB.PIN2CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
 }
 
volatile int mode_frozen = 0;
volatile int mode = 0;
#define EEPROM_ADDR 0x00

ISR(PORTB_PORT_vect) {
    if ((!(VPORTB.IN & PIN2_bm)) && (!(VPORTB.IN & PIN5_bm))) {
        eeprom_write_byte((uint8_t*) EEPROM_ADDR + 0, mode - '0');
        eeprom_write_byte((uint8_t*) EEPROM_ADDR + 1, mode_frozen - '0');
        eeprom_write_byte((uint8_t*) EEPROM_ADDR + 2, mode_sweep - '0');
        PORTB.INTFLAGS = PIN2_bm | PIN5_bm;
        return;
    }

    if (PORTB.INTFLAGS & PIN5_bm) {
        mode_frozen = !mode_frozen;
        PORTB.INTFLAGS = PIN5_bm;
    }
    else {
        mode_sweep = !mode_sweep;
        freq_hz = 10;
        PORTB.INTFLAGS = PIN2_bm;
    }
}

volatile int duty_cycle = 100;
void init_pwm() {
    // PWM freq of 1000Hz, period = 1ms.
    // period = prescaler * (1 + PER) / F_CPU
    // PER = (period * F_CPU / prescaler) - 1
    // PER = (1/1000 * 16000000 / 64) - 1 = 16000 / 64 - 1 = 250 - 1 = 249.
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTC_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    // single slope PWM output on C1.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP1EN_bm;

    TCA0.SINGLE.PERBUF = 249;
    TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF * duty_cycle / 100;
}


#define N_SINES 64
const uint16_t sines[N_SINES] PROGMEM = {
    512, 562, 612, 660, 707, 752, 794, 833, 868, 900, 927, 950, 968,
    982, 991, 995, 995, 991, 982, 968, 950, 927, 900, 868, 833, 794,
    752, 707, 660, 612, 562, 512, 461, 411, 363, 316, 271, 229, 190,
    155, 123, 95,  72,  53,  39,  30,  25,  24,  28,  37,  50,  68,
    90,  116, 146, 180, 218, 259, 303, 350, 399, 449, 500, 551};

volatile int sine_idx = 0;

static inline uint16_t scale_amp(uint16_t x, float scaling) {
    int centered = (int) x - 512;
    int16_t scaled = (int16_t) ((float) centered * scaling) + 512;
    if (scaled < 0) {scaled = 0;}
    else if (scaled > 1023) {scaled = 1023;}
    return (uint16_t) scaled;
}

void init_dac() {
    // DAC0 outputs to PD6.
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;
}

static inline int calc_per() {
    // NOTE: assumes DIV64.
    // (DIV / F_CPU) * PER = (1 / (freq_hz * N_SINES))
    // PER = F_CPU / (DIV * freq_hz * N_SINES)
    if (!mode_sweep) {
        freq_hz = 10 + (avg_voltage * 90);
    }
    return (int) (((int32_t) F_CPU) / ((int32_t) freq_hz * N_SINES * 64));
}

// T = 1 / (440 Hz * 64 sine samples) = 35.5 us.
// 16MHz clock, div64 -> 4us tick. PER = 8 -> 36us period.
// 440 Hz. 
void init_timer() {
    TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA1.SINGLE.PERBUF = calc_per();
    TCA1.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

ISR(TCA1_OVF_vect) {
    uint16_t y = scale_amp(pgm_read_word(&sines[sine_idx]), avg_voltage / 3.3);
    sine_idx = (sine_idx + 1) % N_SINES;
    
    TCA1.SINGLE.PERBUF = calc_per();

    // 10-bit DAC value.
    // upper 8 in DATAH, lower 2 in DATAL[7:6].
    DAC0.DATAL = (uint8_t) ((y & 0x03) << 6);
    DAC0.DATAH = (uint8_t) ((y >> 2) & 0xFF);

    TCA1.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

int main() {
    init_clock();
    init_adc();
    init_rtc();
    uart_init(3, 9600, NULL);
    init_gpio();
    init_pwm();
    init_dac();
    init_timer();
    sei();
    
    uint8_t b0 = eeprom_read_byte((uint8_t*) EEPROM_ADDR + 0);
    if (b0 != 0xFF) {
        mode = (int) b0 - '0';
        mode_frozen = (int) eeprom_read_byte((uint8_t*) EEPROM_ADDR + 1) - '0';
        mode_sweep = (int) eeprom_read_byte((uint8_t*) EEPROM_ADDR + 2) - '0';
    }
    
    while (1) {
        duty_cycle = (avg_voltage / 3.3) * 100;
        if (print_timer_ms >= PRINT_PERIOD_TIME_MS) {
            printf("Voltage = %.2f V, Duty = %.2f, Mode = ", avg_voltage, ((float) duty_cycle) / 100.0);
            if (mode_frozen) {printf("frozen\n");}
            else {printf("unfrozen\n");}
            print_timer_ms = 0;
        }
        if (print2_timer_ms >= PRINT2_PERIOD_TIME_MS) {
            printf("Vpot = %.2f V, A = %.2f V, f = %d Hz, Mode = ", avg_voltage, avg_voltage, freq_hz);
            if (mode_sweep) {printf("sweep\n");}
            else {printf("fixed\n");}
            print2_timer_ms = 0;
        }
        TCA0.SINGLE.CMP1BUF = (TCA0.SINGLE.PERBUF * duty_cycle) / 100;
        if (!mode_frozen) {mode = ((int) (avg_voltage * 10)) % 4;}
        if (mode == 0) {
            VPORTD.OUT = 0;
            _delay_ms(125);
        }
        else if (mode == 1) {
            for (int i = 0; i < 6; ++i) {
                VPORTD.OUT = 1 << i;
                _delay_ms(125);
            }
        }
        else if (mode == 2) {
            if (VPORTD.OUT == 0) {VPORTD.OUT = 0b00111111;}
            else {VPORTD.OUT = 0;}
            _delay_ms(125);
        }
        else {
            TCA0.SINGLE.CMP1BUF = (TCA0.SINGLE.PERBUF * duty_cycle) / 100;
            _delay_ms(100);
        }
    }
}