#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"


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

int main(void) {
    init_clock();
    
    VPORTD.DIR = 0b11111111;
    
    uart_init(3, 9600, NULL);
    
    int pin = 3;
    int freq_hz = 2;
    int delay_ms;
    int timer_ms = 0;
    
    char fp_char;
    
    while (1) {
        delay_ms = (1000 / freq_hz) / 2;
        
        VPORTD.OUT = 1 << pin;
        my_delay_ms(delay_ms);
        
        VPORTD.OUT = 0;
        my_delay_ms(delay_ms);
        
        timer_ms += 2 * delay_ms;
        if (timer_ms >= 5000) {
            timer_ms = 0;

            printf("Do you want to change the frequency or position? (F/P) ");
            scanf("%c", &fp_char);
            getchar();
            if (fp_char == 'F') {
                printf("Frequency: ");
                scanf("%d", &freq_hz);
                getchar();
            }
            else if (fp_char == 'P') {
                printf("Position: ");
                scanf("%d", &pin);
                getchar();
            }
        }
    }
}

