#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"


#define IN_BUF_MAXLEN 3
volatile char in_buf[IN_BUF_MAXLEN];
volatile int buf_head = 0;
volatile int buf_tail = 0;
volatile int buf_print_tail = 0;

volatile char ask_question = 1;
volatile char freq_mode = 1;

volatile int freq_hz = 4;
volatile int n_leds = 1;


void my_delay_ms(int delay_ms) {
    for (int i = 0; i < delay_ms; ++i) {
         _delay_ms(1);
    }
}

void init() {
    VPORTD.DIR = 0b11111111;
    VPORTD.OUT = 0;

    VPORTB.DIR = 1 << 3;
    VPORTB.OUT = 0;
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;

    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;

    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;

    sei();
}

ISR(PORTB_PORT_vect) {
    freq_mode = !freq_mode;
    VPORTB.OUT = (!freq_mode) << 3;
    ask_question = 1;
    PORTB.INTFLAGS = PIN2_bm;
}


ISR(USART3_RXC_vect) {
    in_buf[buf_tail] = USART3.RXDATAL;
    buf_tail = (buf_tail + 1) % IN_BUF_MAXLEN;
}

// read an int of max digits from the circular buffer.
// returns 0 on fail, 1 if success.
int read_int(int max_digits, int* num) {
    int success = 0;

    *num = 0;

    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    int idx = buf_head;

    for (int i = 0; i < max_digits; ++i) {
        if (idx == prev) {
            break;
        }

        int v = in_buf[idx] - '0';
        if ((v < 0) || (v > 9)) {
            break;
        }
        *num = (*num * 10) + v;

        success = 1;
        idx = (idx + 1) % IN_BUF_MAXLEN;
    }
    return success;
}

void handle_input() {
    while (buf_print_tail != buf_tail) {
        if (in_buf[buf_print_tail] == '\r') {
            putchar('\n');
        }
        putchar(in_buf[buf_print_tail]);
        buf_print_tail = (buf_print_tail + 1) % IN_BUF_MAXLEN;
    }

    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    if (in_buf[prev] != '\r') {
        return;
    }

    int new_num;
    if (freq_mode && read_int(2, &new_num)) {
        if (new_num > 0) {
            freq_hz = new_num;
        }
    }
    else if ((!freq_mode) && read_int(1, &new_num)) {
        if ((new_num > 0) && (new_num <= 8)) {
            n_leds = new_num;
        }
    }

    buf_head = buf_tail;
    ask_question = 1;
}

int main(void) {
    init();

    int blink_delay;
    while (1) {
        if (buf_print_tail != buf_tail) {
            cli();
            handle_input();
            sei();
        }

        if (ask_question) {
            ask_question = 0;
            if (freq_mode) {
                printf("\n%d Frequency: ", freq_hz);
            }
            else {
                printf("\n%d # Leds: ", n_leds);
            }
        }

        if (VPORTD.OUT == 0) {
            for (int i = 0; i < n_leds; ++i) {
                VPORTD.OUT |= 1 << i;
            }
        }
        else {
            VPORTD.OUT = 0;
        }
        blink_delay = 1000 / (freq_hz * 2);
        my_delay_ms(blink_delay);
    }
}
