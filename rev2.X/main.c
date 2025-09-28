#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "uart.h"
#include <stdio.h>


#define INPUT_BUFFER_MAXLEN 16
volatile char input_buffer[INPUT_BUFFER_MAXLEN];
volatile char input_buffer_head = 0;
volatile char input_buffer_tail = 0;
volatile char input_buffer_print_tail = 0;

volatile int blink_timer = 0;


void my_delay_ms(int delay_ms) {
    for (int i = 0; i < delay_ms; ++i) {
        _delay_ms(1);
    }
}

void init() {
    VPORTD.DIR = 0b11111111;
    
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;

    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
    
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
    while (RTC.STATUS & RTC_PERBUSY_bm) {}
    RTC.PER = 32768 / 1000;
    RTC.INTCTRL = RTC_OVF_bm;
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;
    
    sei();
}

ISR(USART3_RXC_vect) {
    // printf("\nISR: %d %d %d\n", input_buffer_head, input_buffer_tail, input_buffer_print_tail);
    // for (int i = 0; i < INPUT_BUFFER_MAXLEN; ++i) {
    //     printf("%d ", input_buffer[i]);
    // }
    // putchar('\n');
    // for (int i = input_buffer_head; i != input_buffer_tail; i = (i + 1) % INPUT_BUFFER_MAXLEN) {
    //     printf("%d ", input_buffer[i]);
    // }
    // putchar('\n');

    input_buffer[input_buffer_tail] = USART3.RXDATAL;
    input_buffer_tail = (input_buffer_tail + 1) % INPUT_BUFFER_MAXLEN;
}

ISR(RTC_CNT_vect) {
    if (blink_timer > 0) {
        --blink_timer;
    }
    RTC.INTFLAGS = RTC_OVF_bm;
}

// Read an int from the circular input buffer.
// Returns -1 if read fail (no digits read), else 0.
int read_int_from_input_buffer(int max_digits, int* return_int) {
    *return_int = 0;
    int read_success = -1;
    int num;
    int idx = input_buffer_head;
    for (int i = 0; i < max_digits; ++i) {
        num = input_buffer[idx] - '0';
        if ((num < 0) || (num > 9)) {
            break;
        }
        *return_int = (*return_int * 10) + num;
        read_success = 0;

        if (idx == input_buffer_tail) {
            break;
        }
        idx = (idx + 1) % INPUT_BUFFER_MAXLEN;
    }
    return read_success;
}

int main() {
    init();
    
    int freq_hz = 1;
    int n_pins = 1;

    int asking_freq_or_n_pins = 1;
    int asking_freq = 0;
    int user_ans;
    int ask_question = 1;

    int new_input;
    int delay_ms;
    int prev;
    
    my_delay_ms(2500);
    
    while (1) {
        new_input = input_buffer_print_tail != input_buffer_tail;
        if (new_input) {
            // This extra, seemingly unnecessary check on new_input fixes a race condition.
            while (input_buffer_print_tail != input_buffer_tail) {
                if (input_buffer[input_buffer_print_tail] == '\r') {
                    putchar('\n');
                }
                putchar(input_buffer[input_buffer_print_tail]);
                input_buffer_print_tail = (input_buffer_print_tail + 1) % INPUT_BUFFER_MAXLEN;
            }
        }

        prev = (input_buffer_tail - 1 + INPUT_BUFFER_MAXLEN) % INPUT_BUFFER_MAXLEN;
        if (new_input && (input_buffer[prev] == '\r')) {
            if (asking_freq_or_n_pins) {
                if (input_buffer[input_buffer_head] == 'F') {
                    asking_freq = 1;
                    asking_freq_or_n_pins = 0;
                }
                else if (input_buffer[input_buffer_head] == 'P') {
                    asking_freq = 0;
                    asking_freq_or_n_pins = 0;
                }
            }
            else if (asking_freq) {
                if (read_int_from_input_buffer(2, &user_ans) == 0) {
                    if (user_ans != 0) {
                        freq_hz = user_ans;
                        asking_freq_or_n_pins = 1;
                    }
                }
            }
            else {
                if (read_int_from_input_buffer(1, &user_ans) == 0) {
                    if ((user_ans > 0) && (user_ans <= 8)) {
                        n_pins = user_ans;
                        asking_freq_or_n_pins = 1;
                    }
                }
            }
            input_buffer_head = input_buffer_tail;
            ask_question = 1;
        }

        if (ask_question) {
            if (asking_freq_or_n_pins) {
                printf("(F)req or # of (P)ins: ");
            }
            else if (asking_freq) {
                printf("Freq: ");
            }
            else {
                printf("# Pins: ");
            }
            ask_question = 0;
        }
        
        if (blink_timer == 0) {
            blink_timer = 1000 / (freq_hz * 2);
            if (VPORTD.OUT == 0) {
                for (int i = 0; i < n_pins; ++i) {
                    VPORTD.OUT |= 1 << i;
                }
            }
            else {
                VPORTD.OUT = 0;
            }
        }
    }
}
