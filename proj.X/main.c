#include "my_util.h"
#include "uart.h"
#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdio.h>

volatile char last_char = 'x';
#define MAX_KEY_TIME_MS 300
volatile int key_timer = 0;

// ISR for USART from bluetooth device, just grab character sent.
ISR(USART3_RXC_vect) {
    last_char = USART3_RXDATAL;
    key_timer = 0;
}

// Setting up variables for timer and RTC.
#define PRINT_TIME_MS 100
volatile int print_timer = 0;
volatile int should_print = 0;

volatile int should_read_speed = 0;

#define SPEED_THRESHOLD 1070.0f
#define SPEED_MIN 1000.0f

// RTC ISR: Activated 10 times/sec, read speed.
ISR(RTC_CNT_vect) {
    should_read_speed = 1;
    RTC.INTFLAGS = RTC_OVF_bm;
}

// Timer ISR: ensure to time out after a key is pressed, stop robot movement.
ISR(TCA0_OVF_vect) {
    if (key_timer >= MAX_KEY_TIME_MS) {
        last_char = 'x';
        key_timer = 0;
    } else key_timer++;

    if (print_timer >= PRINT_TIME_MS) {
        print_timer = 0;
        should_print = 1;
    } else print_timer++;

    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

// Calling init, most inits defined in my_util.
void init(void) {
    clock_init();
    DAC_init(&DAC0);
    timer_init(&TCA0);
    RTC_init();

    uart_init(3, 9600, NULL);
    USART3.CTRLA |= USART_RXCIE_bm;
    USART3.CTRLB = USART_TXEN_bm | USART_RXEN_bm;

    APA102_init();
    LIS3DH_init();
    sei();
}

int main(void) {
    init();
    printf("LED + Accelerometer system started.\n");

    Coord3D coord;
    float speed = 0.0;
    while (1) {
        // Print out on occassion.
        if (should_print) {
            printf("%c: %.2f\n", last_char, char_to_voltage(last_char));
            should_print = 0;
        }
        
        // Output DAC so microcontroller can read voltage for wheel control.
        DAC_out(&DAC0, char_to_voltage(last_char));
        
        // Check speed from accelerometer when ISR says to.
        if (should_read_speed) {
            should_read_speed = 0;
            LIS3DH_read_xyz(&coord);
            speed = sqrtf((float)coord.x*coord.x + (float)coord.y*coord.y + (float)coord.z*coord.z);
            printf("[SPEED] x=%d y=%d z=%d speed=%.2f\n", coord.x, coord.y, coord.z, speed);
        }
        
        // Setting LED lights: green for moving, red for not.
        if (speed >= SPEED_THRESHOLD || speed < SPEED_MIN) {
            APA102_set_all(15, 0, 255, 0);
        }
        else {
            APA102_set_all(15, 255, 0, 0);
        }
    }
    return 0;
}
