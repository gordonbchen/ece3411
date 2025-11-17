#include "my_util.h"
#include "uart.h"
#include <avr/delay.h>

void init() {
    clock_init();
    uart_init(3, 9600, NULL);

    // GPIO.
    VPORTD.DIR = 0b11111111;
}

int main() {
    init();
    while (1) {
        printf("Hello world!\n");
        VPORTD.OUT = ~VPORTD.OUT;
        _delay_ms(250);
    }
}