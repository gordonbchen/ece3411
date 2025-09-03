#define F_CPU 4000000UL  // Clock freq.
#include <avr/io.h>
#include <util/delay.h>


void my_delay_ms(int ms) {
    for (int i = 0; i < ms; ++i) {
        _delay_ms(1);
    }
}

int main(void) {
    PORTD.DIRSET = 0b11111111;  // Set all pins of Port D to output.
    
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;  // Set Port B pin 2 to pull up.
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm;  // Set Port B pin 5 to pull up.
    
    unsigned char pin = 3;
    char dir = 1;
    unsigned int hz = 3;
    
    unsigned char pb2_pressed = 0;
    unsigned char pb5_pressed = 0;
    unsigned char just_pressed = 0;
    
    while (1) {
        if (!(PORTB.IN & PIN2_bm)) {
            if (!pb2_pressed) {
                pb2_pressed = 1;
                hz++;
                just_pressed++;
            }
        }
        else {
            pb2_pressed = 0;
        }
        
        if (!(PORTB.IN & PIN5_bm)) {
            if (!pb5_pressed) {
                pb5_pressed = 1;
                if (hz > 1) {
                    hz--;
                }
                just_pressed++;
            }
        }
        else {
            pb5_pressed = 0;
        }
        
        if (just_pressed == 2) {
            if (pin == 7) {
                dir = -1;
            }
            else if (pin == 0) {
                dir = 1;
            }
            pin = pin + dir;
        }
        just_pressed = 0;

        char pin_bm = 1 << pin;
        
        int delay_ms = (1000 / hz) / 2;
        PORTD.OUTSET = pin_bm;  // Set Port D pin 3 to high.
        my_delay_ms(delay_ms);
        PORTD.OUTCLR = pin_bm;  // Clear Port D pin 3 (set to low).
        my_delay_ms(delay_ms);
    }
    return (0);
}