#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "uart.h"


int print_time = 5000;
volatile int print_timer_ms = 0;

volatile int freq_hz = 3;

volatile int blink_timer_ms = 0;
volatile int print_hz = 1;
volatile int toggle_led = 1;

volatile int button_pressed = 0;


void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;  
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

void init_timer() {
    // T = N * (1 + PER) / f_clock
    //   = 64 * (1 + 249) / 16 MHz = 1000 us = 1 ms.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;  // normal mode.
    TCA0.SINGLE.PERBUF = 249;  // n ticks for period.
    
    // set prescaler = 64, each tick is 4us.
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    
    // enable overflow interrupt.
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

ISR(TCA0_OVF_vect) {
    if (print_timer_ms > print_time) {
        print_hz = 1;
        print_timer_ms = -1;
    }
    ++print_timer_ms;
    
    int blink_time = (1000 / freq_hz) / 2;
    if (blink_timer_ms > blink_time) {
        toggle_led = 1;
        blink_timer_ms = -1;
    }
    ++blink_timer_ms;
    
    // clear interrupt.
    TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

void init_buttons() {
    // set B2 and B5 to input.
    PORTB.DIRCLR = PIN2_bm | PIN5_bm;
    
    // enable pullup.
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm;
    
    // interrupt on falling (button pressed = low voltage).
    PORTB.PIN2CTRL |= PORT_ISC_FALLING_gc;
    PORTB.PIN5CTRL |= PORT_ISC_FALLING_gc;
}

ISR(PORTB_PORT_vect) {
    if (button_pressed) {
        return;
    }

    if (PORTB.INTFLAGS & PIN2_bm) {
        ++freq_hz;
        PORTB.INTFLAGS = PIN2_bm;
    }
    else if (PORTB.INTFLAGS & PIN5_bm) {
        if (freq_hz > 1) {
            --freq_hz;
        }
        PORTB.INTFLAGS = PIN5_bm;
    }
    button_pressed = 1;
}

int main(void) {
    // Init clock, uart, button interrupts, timer.
    init_clock();
    uart_init(3, 9600, NULL);
    init_buttons();
    init_timer();
    sei();
    
    VPORTD.DIR = 0b11111111;
    
    int led_state = 0;
    while (1) {
        if (print_hz) {
            printf("%d\n", freq_hz);
            print_hz = 0;
        }
        if (toggle_led) {
            VPORTD.OUT = led_state << 3;
            led_state = !led_state;
            toggle_led = 0;
        }
        
        if (button_pressed) {
            button_pressed = 0;
        }
    }
}

