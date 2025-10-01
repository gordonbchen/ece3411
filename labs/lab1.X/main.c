// ------- Preamble -------- //
#define F_CPU 4000000UL /* Tells the Clock Freq to the Compiler. */
#include <avr/io.h> /* Defines pins, ports etc. */
#include <util/delay.h> /* Functions to waste time */
int main(void) {
    // -------- Inits --------- //
    /* Data Direction Register B: writing a one to the bit enables output. */
    PORTB.DIRSET = PIN3_bm;
    // ------ Event loop ------ //
    while (1) {
        PORTB.OUTSET = PIN3_bm; /* Turn on the LED bit/pin in PORTB */
        _delay_ms(1000); /* wait for 1 second */
        PORTB.OUTCLR = PIN3_bm; /* Turn off the LED bit/pin */
        _delay_ms(1000); /* wait for 1 second */
    } /* End event loop */
    return (0); /* This line is never reached */
}