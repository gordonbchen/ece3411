#define F_CPU 4000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


volatile int freq_hz = 4;

void my_delay_ms(int delay_ms) {
    for (int i = 0; i < delay_ms; ++i) {
        _delay_ms(1);
    }
}

void init_ext_int() {
    PORTB.DIRCLR = PIN2_bm | PIN5_bm;  // B2 and B5 input.
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm;

    PORTB.PIN2CTRL |= PORT_ISC_FALLING_gc; // Interrupt on falling (1 -> 0).
    PORTB.PIN5CTRL |= PORT_ISC_FALLING_gc;
    sei();  // Enable global interrupts.
}

ISR(PORTB_PORT_vect) {
    if (PORTB.INTFLAGS & PIN2_bm) {
        freq_hz = 8;
        PORTB.INTFLAGS = PIN2_bm;  // Clear B2 interrupt flag.
    }
    else if (PORTB.INTFLAGS & PIN5_bm) {
        freq_hz = 1;
        PORTB.INTFLAGS = PIN5_bm;
    }
}


int main(void) {    
    VPORTD.DIR = 0b11111111;

    init_ext_int();

    int pin = 0;
    while (1) {
        for (int i = 0; i < freq_hz; ++i) {
            VPORTD.OUT = 1 << pin;
            my_delay_ms(500 / freq_hz);

            VPORTD.OUT = 0;
            my_delay_ms(500 / freq_hz);
        }
        pin = (pin + 1) % 8;
    }
    
    return 0;
}
