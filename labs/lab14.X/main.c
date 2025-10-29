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

#define TIMER_MS 500
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

void init_SPI() {
    // Route SPI0: MOSI = PA4, SCK = PA5, SS = PA7.
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_DEFAULT_gc;
    
    // Enable SPI0 in Master mode, set prescaler = 128.
    SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV128_gc;
    
    // Enable SS pin control.
    SPI0.CTRLB = SPI_SSD_bm;
}

void SPI0_write(uint8_t data) {
    SPI0.DATA = data;
    // Block until transfer complete.
    while (!(SPI0.INTFLAGS & SPI_IF_bm));
}

// MCP4131 Commands
#define MCP4131_WRITE_CMD 0x00  // Write command to potentiometer.
#define MCP4131_WIPER0 0x00     // Target wiper 0.

void MCP4131_set_wiper(uint8_t value) {
    PORTA.OUTCLR = PIN7_bm;  // CS low.
    SPI0_write(MCP4131_WRITE_CMD | MCP4131_WIPER0);  // Send command.
    SPI0_write(value);
    PORTA.OUTSET = PIN7_bm;  // CS high.
}

int main() {
    init_clock();
    uart_init(3, 9600, NULL);
    init_timer();
    init_adc();
    sei();
    
    VPORTA.DIR = 0b11111111;
    init_SPI();

    float voltage;
    uint8_t v;
    while (1) {
        voltage = get_voltage();
        v = (uint8_t) ((voltage * 127.0) / 3.3);
        MCP4131_set_wiper(v);
        
        if (timer == TIMER_MS) {
            timer = 0;
            printf("%.2f\n", voltage);
            printf("%d\n", v);
        }
    }
};
