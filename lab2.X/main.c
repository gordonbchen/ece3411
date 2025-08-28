#define F_CPU 4000000UL  // Clock freq.
#include <avr/io.h>  // Pins, ports.
#include <util/delay.h>  // Delay funcs.

int main(void) {
    PORTD.DIRSET = 0b11111111;  // Set all to output.
    
    int pin = 0;
    while (1) {
        for (int i = 0; i < 2 * 2; ++i) {
            PORTD.OUTSET |= (1 << pin);
            _delay_ms(250);
            PORTD.OUTCLR |= (1 << pin);
            _delay_ms(250);
        }
        
        for (int i = 0; i < 2 * 4; ++i) {
            PORTD.OUTSET |= (1 << pin);
            _delay_ms(125);
            PORTD.OUTCLR |= (1 << pin);
            _delay_ms(125);
        }
        pin = (pin + 1) % 8;
    }
    return 0;
}