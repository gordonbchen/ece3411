#define F_CPU 4000000UL

#include <avr/io.h>
#include <util/delay.h>


void my_delay_ms(int ms) {
    for (int i = 0; i < ms; ++i) {
        _delay_ms(1);
    }
}

int seq_detected(int buttons[], int button_idx_start, int button_seq[]) {
    for (int i = 0; i < 4; ++i) {
        int button_idx = (button_idx_start + i) % 4;
        if (*(buttons + button_idx) != button_seq[i]) {
            return 0;
        }
    }
    return 1;
}

int main() {
    VPORTD.DIR = 0b11111111;
    VPORTC.DIR |= 0b11000000;
    
    int d_iters = 5;
    int c_iters = 2;
    
    int d_pin = 6;
    
    
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm;
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    
    int dir = 1;
    int pb2_pressed = 0;
    int pb5_pressed = 0;
    
    int button_seq[4] = {5, 2, 5, 2};
    int buttons[4] = {0, 0, 0, 0};
    int button_idx = 0;
    int n_buttons = 2;
    

    while (1) {
        for (int i = 0; i < 40; ++i) {
            if (i % d_iters == 0) {
                if (VPORTD.OUT & (1 << d_pin)) {
                    VPORTD.OUT = 0;
                }
                else {
                    VPORTD.OUT = 0;
                    for (int j = 0; j < n_buttons; ++j) {
                        VPORTD.OUT |= 1 << ((d_pin - j + 8) % 8);
                    }
                }
            }
            
            if (i % c_iters == 0) {
                if (VPORTC.OUT & PIN6_bm) {
                    VPORTC.OUT = 0;
                }
                else {
                    VPORTC.OUT = 0b11000000;
                }
            }
            
            if (!(VPORTB.IN & PIN2_bm)) {
                if (!pb2_pressed) {
                    buttons[button_idx] = 2;
                    button_idx = (button_idx + 1) % 4;
                    pb2_pressed = 1;
                }
            }
            else {
                pb2_pressed = 0;
            }
            
            if (!(VPORTB.IN & PIN5_bm)) {
                if (!pb5_pressed) {
                    dir = -dir;
                    buttons[button_idx] = 5;
                    button_idx = (button_idx + 1) % 4;
                    pb5_pressed = 1;
                }
            }
            else {
                pb5_pressed = 0;
            }
            
            if ((i == 20) & (!pb2_pressed)) {
                d_pin = (d_pin + dir + 8) % 8;
            }
            
            if (seq_detected(buttons, button_idx, button_seq)) {
                if (n_buttons == 2) {
                    n_buttons = 4;
                }
                else {
                    n_buttons = 2;
                }
                d_pin = 7;
                buttons[3] = 0;
            }
            
            my_delay_ms(50);
        }
        
        // Swap freqs.
        int tmp = d_iters;
        d_iters = c_iters;
        c_iters = tmp;
    }
}