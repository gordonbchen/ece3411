#include "my_util.h"
#include "uart.h"
#include <avr/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void init() {
    clock_init();
    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;  // Enable usart interrupt.
    VPORTD.DIR = 0b11111111;
    DAC_init(&DAC0);  // DAC output at PD6.
    timer_init(&TCA0);
    sei();
}

volatile char last_char = 'x';
#define MAX_KEY_TIME_MS 300  // time to press key or else will register as stop.
volatile int key_timer = 0;
ISR(USART3_RXC_vect) {
    last_char = USART3_RXDATAL;
    key_timer = 0;
}

#define PRINT_TIME_MS 100
volatile int print_timer = 0;
volatile int should_print = 0;
ISR(TCA0_OVF_vect) {
    if (key_timer >= MAX_KEY_TIME_MS) {
        last_char = 'x';  // user did not press key. stop.
        key_timer = 0;
    }
    else {
        ++key_timer;
    }

    if (print_timer >= PRINT_TIME_MS) {
        print_timer = 0;
        should_print = 1;
    }
    else {
        ++print_timer;
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}


int main() {
    init();

    printf("HELLO: WASD Controls:\n");
    while (1) {
        if (should_print) {
            printf("%c: %.2f\n", last_char, char_to_voltage(last_char));
            should_print = 0;
        }
        DAC_out(&DAC0, char_to_voltage(last_char));
    }
}

// Accel.
// LIS3DH_init();
// Coord3D coord;
// while (1) {
//     LIS3DH_read_xyz(&coord);
//     printf("x=%d, y=%d, z=%d\n", coord.x, coord.y, coord.z);
//     _delay_ms(250);
// }