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
    sei();
}

volatile char last_char = ' ';
ISR(USART3_RXC_vect) {
    last_char = USART3_RXDATAL;
}

int main() {
    init();

    getchar();
    printf("HELLO: WASD Controls:\n");
    while (1) {
        printf("%c: %f\n", last_char, char_to_voltage(last_char));
        DAC_out(&DAC0, char_to_voltage(last_char));
        _delay_ms(100);
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