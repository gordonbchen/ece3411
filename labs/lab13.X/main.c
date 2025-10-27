#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "uart.h"

// ------------ OPAMP INIT -------------
void OPAMP_init(void) {
    OPAMP.TIMEBASE = ceil(F_CPU / 1e6) - 1;

    OPAMP.OP0CTRLA = OPAMP_OP0CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;
    OPAMP.OP1CTRLA = OPAMP_OP1CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;
    OPAMP.OP2CTRLA = OPAMP_OP2CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;

    OPAMP.OP0INMUX = OPAMP_OP0INMUX_MUXPOS_INP_gc | OPAMP_OP0INMUX_MUXNEG_OUT_gc;
    OPAMP.OP0RESMUX = OPAMP_OP0RESMUX_MUXBOT_GND_gc |
                      OPAMP_OP0RESMUX_MUXWIP_WIP3_gc |
                      OPAMP_OP0RESMUX_MUXTOP_OUT_gc;

    OPAMP.OP1INMUX = OPAMP_OP1INMUX_MUXPOS_INP_gc | OPAMP_OP1INMUX_MUXNEG_OUT_gc;
    OPAMP.OP1RESMUX = OPAMP_OP1RESMUX_MUXBOT_OFF_gc |
                      OPAMP_OP1RESMUX_MUXWIP_WIP0_gc |
                      OPAMP_OP1RESMUX_MUXTOP_OFF_gc;

    OPAMP.OP2INMUX = OPAMP_OP2INMUX_MUXPOS_LINKWIP_gc | OPAMP_OP2INMUX_MUXNEG_WIP_gc;
    OPAMP.OP2RESMUX = OPAMP_OP2RESMUX_MUXBOT_LINKOUT_gc |
                      OPAMP_OP2RESMUX_MUXWIP_WIP3_gc |
                      OPAMP_OP2RESMUX_MUXTOP_OUT_gc;

    // Settling time.
    OPAMP.OP0SETTLE = 0x7F;
    OPAMP.OP1SETTLE = 0x7F;
    OPAMP.OP2SETTLE = 0x7F;
    
    // Disable interrupts for opamp.
    // OP0.
    PORTD.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc
;    PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // Enable OPAMP.
    OPAMP.CTRLA = OPAMP_ENABLE_bm;

    // Wait for op ams to settle.
    while (!(OPAMP.OP0STATUS & OPAMP_SETTLED_bm));
    while (!(OPAMP.OP1STATUS & OPAMP_SETTLED_bm));
    while (!(OPAMP.OP2STATUS & OPAMP_SETTLED_bm));
}

// ------------ ADC INIT -------------
void ADC_init(void) {
    ADC0.MUXPOS = ADC_MUXPOS_AIN10_gc; // OPAMP2 output at AIN9.
    VREF.ADC0REF = VREF_REFSEL_VDD_gc; // Use VDD (3.3 V) as ref.
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;
    ADC0.CTRLA = ADC_ENABLE_bm;
    ADC0.COMMAND = ADC_STCONV_bm;
}

uint16_t ADC_read(void) {
    while (ADC0.COMMAND & ADC_STCONV_bm);
    uint16_t adc_val = ADC0.RES;
    ADC0.COMMAND = ADC_STCONV_bm;
    return adc_val;
}

// ------------ CURRENT CALC -------------
float read_current(void) {
    float adc_val = (float) ADC_read();
    printf("adc_val: %.3f\n", adc_val);
    float v_measured = (adc_val / 4096.0) * 3.3;  // 12-bit ADC.
    printf("v_measured: %.3f\n", v_measured);
    float v_shunt = v_measured;             // remove amplifier gain, gain = 1.0.
    return v_shunt / 330.0;                         // I = V/R, 0.1-ohm shunt resistor.
}

// ------------ CLOCK INIT -------------
void CLK_init(void) {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
    _delay_ms(100);
}

// ------------ MAIN LOOP -------------
int main(void) {
    CLK_init();
    OPAMP_init();
    ADC_init();
    uart_init(3, 9600, NULL);

    float current;
    while (1) {
        current = read_current();
        printf("Current: %.3f A\n\n", current);
        _delay_ms(500);
    }
}

