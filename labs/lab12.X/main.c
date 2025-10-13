#define F_CPU 16000000UL

#include "uart.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>


static inline void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}


static inline void init_ports() {
    VPORTB.DIR = ~PIN5_bm;
    VPORTB.OUT = 1 << 3;
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}

volatile int verifying = 1;
volatile int input_new_pass = 0;
volatile int ask_question = 1;

ISR(PORTB_PORT_vect) {
    if ((!verifying) && (!input_new_pass)) {
        ask_question = 1;
        input_new_pass = 1;
    }
    PORTB.INTFLAGS = PIN5_bm;
}


static inline void init_usart() {
    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
}

#define IN_BUF_MAXLEN 10
volatile char in_buf[IN_BUF_MAXLEN];
volatile int buf_head = 0;
volatile int buf_tail = 0;
volatile int buf_print_tail = 0;

ISR(USART3_RXC_vect) {
    in_buf[buf_tail] = USART3.RXDATAL;
    buf_tail = (buf_tail + 1) % IN_BUF_MAXLEN;
}

#define PASSWORD_LEN 4
#define PASSWORD_LEN_PLUS1 5

volatile int done = 0;

static int correct_password(char password[]) {
    for (int i = 0; i < PASSWORD_LEN; ++i) {
        if (password[i] != in_buf[(buf_head + i) % IN_BUF_MAXLEN]) {
            return 0;
        }
    }
    return 1;
}

static void handle_input(char password[], char new_password[]) {
    // print user input chars.
    while (buf_print_tail != buf_tail) {
        if (in_buf[buf_print_tail] == '\r') putchar('\n');
        putchar(in_buf[buf_print_tail]);
        buf_print_tail = (buf_print_tail + 1) % IN_BUF_MAXLEN;
    }

    // return if user didn't hit enter yet.
    int prev = (buf_tail - 1 + IN_BUF_MAXLEN) % IN_BUF_MAXLEN;
    if (in_buf[prev] != '\r') return;

    int n_chars = ((buf_tail - buf_head + IN_BUF_MAXLEN) % IN_BUF_MAXLEN) - 1;
    int correct_len = n_chars == PASSWORD_LEN;
    if (verifying) {
        if (correct_len && correct_password(password)) {
            VPORTB.OUT = 0;
            printf("Access granted. Waiting for PB5.\n");
            verifying = 0;
        }
        else {
            printf("Access denied: incorrect password.\n");
            for (int i = 0; i < 3; ++i) {
                VPORTB.OUT = 0;
                _delay_ms(250);
                VPORTB.OUT = 1 << 3;
                _delay_ms(250);
            }
        }
    }
    else if (input_new_pass && correct_len) {
        for (int i = 0; i < PASSWORD_LEN; ++i) {
            new_password[i] = in_buf[(buf_head + i) % IN_BUF_MAXLEN];
        }
        done = 1;
    }

    buf_head = buf_tail;
    ask_question = 1;
}


#define EEPROM_ADDR 0x00

void write_str_eeprom(char *str, int strlen) {
    for (int i = 0; i < strlen; ++i) {
        eeprom_write_byte((uint8_t *)EEPROM_ADDR + i, str[i]);
        _delay_ms(10);
    }
}

void read_str_eeprom(char *str, int strlen) {
    for (int i = 0; i < strlen; ++i) {
        str[i] = eeprom_read_byte((uint8_t *)EEPROM_ADDR + i);
    }
}


int main(void) {
    init_clock();
    _delay_ms(100);
    init_usart();
    init_ports();

    for (int i = 0; i < 10; ++i) {
        _delay_ms(500);
        printf("%d ", i);
    }
    printf("\n");

    sei();

    // set / read init password.
    int prev_pass = eeprom_read_byte((uint8_t)EEPROM_ADDR) != 0xFF;
    if (!prev_pass) {
        write_str_eeprom("init", PASSWORD_LEN);
    }

    char password[PASSWORD_LEN_PLUS1];
    password[PASSWORD_LEN] = 0;
    read_str_eeprom(password, PASSWORD_LEN);

    char new_password[PASSWORD_LEN_PLUS1];
    new_password[PASSWORD_LEN] = 0;

    while (!done) {
        if (buf_print_tail != buf_tail) {
            cli();
            handle_input(password, new_password);
            sei();
        }

        if (ask_question) {
            ask_question = 0;
            if (verifying) {
                printf("\nPassword: %s\n", password);
                printf("Verify password: ");
            }
            if (input_new_pass && (!done)) printf("\nNew password: ");
        }
    }

    // write the new password.
    write_str_eeprom(new_password, PASSWORD_LEN);
    printf("Wrote new password: %s\n", new_password);

    return 0;
}
