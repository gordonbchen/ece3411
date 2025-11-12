#include "my_util.h"

void clock_init() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

void ADC_init() {
    ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;  // input from PE0
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;   // CLK_ADC = 1 MHz
    ADC0.CTRLD = ADC_INITDLY_DLY16_gc; // delay 16 CLK_ADC cycles (16 us)
    VREF.ADC0REF = VREF_REFSEL_VDD_gc; // use VDD as reference voltage
    ADC0.CTRLA = ADC_ENABLE_bm;        // enable ADC
    ADC0.COMMAND = ADC_STCONV_bm;      // start AD conversion.
}

float read_voltage_adc() {
    while (ADC0.COMMAND & ADC_STCONV_bm);
    // wait until conversion done.
    // (Ain / max 12-bit value) * 3.3V
    float voltage = ((float)ADC0.RES / 4096.0) * 3.3;
    ADC0.COMMAND = ADC_STCONV_bm; // start next conversion.
    return voltage;
}

int calc_per(int freq_hz, int div) {
    // period = prescaler * (1 + PER) / F_CPU
    // PER = (period * F_CPU / prescaler) - 1
    // Ex: PWM freq of 1000Hz, period = 1ms.
    //     PER = (1/1000 * 16000000 / 64) - 1 = 16000 / 64 - 1 = 250 - 1 = 249.
    return ((F_CPU / freq_hz) / div) - 1;
}