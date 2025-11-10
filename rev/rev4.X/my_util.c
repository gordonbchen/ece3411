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

float get_voltage() {
    while (ADC0.COMMAND & ADC_STCONV_bm);
    // wait until conversion done.
    // (Ain / max 12-bit value) * 3.3V
    float voltage = ((float)ADC0.RES / 4096.0) * 3.3;
    ADC0.COMMAND = ADC_STCONV_bm; // start next conversion.
    return voltage;
}
