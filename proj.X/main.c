#include "my_util.h"
#include "uart.h"
#include <avr/delay.h>

void init() {
    clock_init();
    uart_init(3, 9600, NULL);

    // GPIO.
    VPORTD.DIR = 0b11111111;

    LIS3DH_init();
}

int main() {
    init();

    Coord3D coord;
    while (1) {
        LIS3DH_read_xyz(&coord);

        printf("x=%d, y=%d, z=%d\n", coord.x, coord.y, coord.z);
        VPORTD.OUT = ~VPORTD.OUT;
        _delay_ms(250);
    }
}

