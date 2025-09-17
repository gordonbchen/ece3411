#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "uart.h"


volatile int pin = 3;
volatile int freq_hz = 2;
volatile char asking_fp = 1;  // 1 for fp, 0 for num
volatile char ask = 0;
volatile char asking_forp = 'F';
volatile char user_ans = 0;

void my_delay_ms(int ms) {
    for (int i = 0; i < ms; ++i) {
        _delay_ms(1);
    }
}

void init_clock() {
    // Unlock protected io registers. Needed for next clock control.
    CPU_CCP = CCP_IOREG_gc;

    // Enable external clock (16Mhz range).
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;  

    CPU_CCP = CCP_IOREG_gc;
    // Use external clock source.
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

ISR(USART3_RXC_vect) {
    ask = 1;
    user_ans = USART3.RXDATAL;

    if (asking_fp) {
        asking_forp = user_ans;
        asking_fp = 0;
        return;
    }
    
    if (asking_forp == 'F') {
        freq_hz = user_ans - '0';
    }
    else {
        pin = user_ans - '0';
    }
    asking_fp = 1;
}

int main(void) {
    init_clock();
    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
    sei();
    
    VPORTD.DIR = 0b11111111;
    
    int delay_ms;
    
    while (1) {
        if (user_ans != 0) {
            putchar(user_ans);
            user_ans = 0;
        }
        
        if (ask) {
            if (asking_fp) {
                printf("\nDo you want to change the frequency or position? (F/P) ");
            }
            else {
                if (asking_forp == 'F') {
                    printf("\nFrequency: ");
                }
                else {
                    printf("\nPosition: ");
                }
            }
            ask = 0;
        }
        
        delay_ms = (1000 / freq_hz) / 2;
        VPORTD.OUT = 1 << pin;
        my_delay_ms(delay_ms);
        VPORTD.OUT = 0;
        my_delay_ms(delay_ms);
    }
}

