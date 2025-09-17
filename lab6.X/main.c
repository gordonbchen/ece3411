#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "uart.h"


volatile int pin = 3;
volatile int freq_hz = 2;

volatile char asking_fp = 1;  // 1 for fp, 0 for num
volatile char print_question = 1;
volatile char forp = 'F';

volatile char user_ans = 0;
volatile char user_num;  // user input number for frequency / position.


void my_delay_ms(int ms) {
    for (int i = 0; i < ms; ++i) {
        _delay_ms(1);
    }
}

void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;  
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

ISR(USART3_RXC_vect) {
    print_question = 1;
    user_ans = USART3.RXDATAL;

    if (asking_fp) {
        if (!((user_ans == 'F') || (user_ans == 'P'))) {
            return;
        }
        forp = user_ans;
        asking_fp = 0;
        return;
    }
    
    user_num = user_ans - '0';
    if ((user_num < 0) || (user_num > 9)) {
        return;
    }
    if (forp == 'F') {
        if (user_num == 0) {
            return;
        }
        freq_hz = user_num;
    }
    else {
        if (user_num > 7) {
            return;
        }
        pin = user_num;
    }
    asking_fp = 1;
}

int main(void) {
    // Init UART and interrupts.
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
        
        if (print_question) {
            if (asking_fp) {
                printf("\nDo you want to change the frequency or position? (F/P) ");
            }
            else {
                if (forp == 'F') {
                    printf("\nFrequency: ");
                }
                else {
                    printf("\nPosition: ");
                }
            }
            print_question = 0;
        }
        
        // Blink.
        delay_ms = (1000 / freq_hz) / 2;
        VPORTD.OUT = 1 << pin;
        my_delay_ms(delay_ms);
        VPORTD.OUT = 0;
        my_delay_ms(delay_ms);
    }
}

