#define F_CPU 4000000UL

#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    VPORTD.DIR = 0b11111111;
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    
    while (1) {
        if (PORTB.IN & PIN5_bm) {
            VPORTD.OUT = 0b00000000;
            continue;
        }
        VPORTD.OUT = PIN3_bm;
    }
    return 0;
}
