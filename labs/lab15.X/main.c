#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <stdio.h>
#include "uart.h"
#include "i2c_lib_S25.h"

void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

#define TC74_ADDR 0x48  // I2C address.
void init_twi() {
    // Set baud rate and enable I2C.
    TWI_Host_Initialize();
   
    // Pull up SDA and SCLK.
    PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;
}

int main() {
    init_clock();
    uart_init(3, 9600, NULL);
    init_twi();
    sei();
    
    uint8_t temp;
    while (1) {
        temp = TWI_Host_Read(TC74_ADDR, 0x00);
        printf("temp = %d\n", temp);
        _delay_ms(500);
    }
};
