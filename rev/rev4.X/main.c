#define F_CPU 16000000UL

#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "i2c.h"
#include "uart.h"

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

#define TC74_ADDR 0x48
#define TC74_READ_TEMP_CMD 0x00
int get_temp() {
    return TWI_read(TC74_ADDR, TC74_READ_TEMP_CMD);
}

int main() {
    clock_init();
    uart_init(3, 9600, NULL);
    ADC_init();
    TWI_init();

    while (1) {
        printf("%.2f V  %d C\n", get_voltage(), get_temp());
        _delay_ms(250);
    }
}
