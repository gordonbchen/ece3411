#define F_CPU 4000000UL

#include <avr/io.h>
#include <util/delay.h>


void my_delay_ms(int ms) {
    for (int i = 0; i < ms; ++i) {
        _delay_ms(1);
    }
}

int main() {
    VPORTD.DIR = 0b11111111;
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    
    char pin = 3;
    char button_down = 0;
    
    // 4Hz at 50% duty cycle.
    int period = 1000 / 4;
    int inv_duty_cycle = 2;
    int on_ms;
    
    while (1) {
        if (!(VPORTB.IN & PIN2_bm)) {
            if (!button_down) {
                button_down = 1;
                inv_duty_cycle = 4;
            }
        }
        else {
            if (button_down) {
                inv_duty_cycle = 2;
            }
            button_down = 0;
        }
        
        // Blink LED at D3
        on_ms = period / inv_duty_cycle;
        VPORTD.OUT = 1 << pin;
        my_delay_ms(on_ms);    
        VPORTD.OUT = 0;
        my_delay_ms(period - on_ms);
    }
}