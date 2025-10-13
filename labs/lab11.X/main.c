#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "uart.h"

void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

void init_timer() {
    // T = N * (1 + PER) / f_clock
    //   = 64 * (1 + 249) / 16 MHz = 1 ms.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.PERBUF = 249;
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

#define TIMER_MS 1000
volatile int timer = 0;

ISR(TCA0_OVF_vect) {
    if (timer < TIMER_MS) {
        ++timer;
    }
    TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

void init_adc() {
    ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;    // input from PE0
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;     // CLK_ADC = 1 MHz
    ADC0.CTRLD = ADC_INITDLY_DLY16_gc;   // delay 16 CLK_ADC cycles (16 us)
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;   // use VDD as reference voltage
    ADC0.CTRLA = ADC_ENABLE_bm;          // enable ADC
    ADC0.COMMAND = ADC_STCONV_bm;        // start AD conversion.
}

float get_voltage() {
    while (ADC0.COMMAND & ADC_STCONV_bm);  // wait until conversion done.
    // (Ain / max 12-bit value) * 3.3V
    float voltage = ((float) ADC0.RES / 4096.0) * 3.3;
    ADC0.COMMAND = ADC_STCONV_bm;  // start next conversion.
    return voltage;
}

int main() {
    init_clock();
    uart_init(3, 9600, NULL);
    init_timer();
    init_adc();
    sei();

    float voltage;
//    char vbuf[16];
    while (1) {
        if (timer == TIMER_MS) {
            timer = 0;
            voltage = get_voltage();
//            dtostrf(voltage, 4, 2, vbuf);  // min 4 chars, 2 decimals.
//            printf("%s\n", vbuf);
            printf("%d.%d%d\n", (int)voltage, (int) (voltage / 0.1) % 10, (int) (voltage / 0.01) % 10);
        }
    }
};
