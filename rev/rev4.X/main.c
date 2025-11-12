#include "my_util.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>
#include "i2c.h"
#include "uart.h"

void GPIO_init() {
    VPORTD.DIR = 0b11111111;
}

void PWM_init(int pwm_freq_hz) {
    // single slope PWM output on D0.
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm;

    TCA0.SINGLE.PERBUF = calc_per(pwm_freq_hz, 64);
    TCA0.SINGLE.CMP0BUF = 0;
}

void PWM_set_voltage(float voltage) {
    TCA0.SINGLE.CMP0BUF = TCA0.SINGLE.PERBUF * (voltage / 3.3);
}

#define WAVEFORM_TIMER_FREQ 10
void Timer_init() {
    TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA1.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    TCA1.SINGLE.PERBUF = calc_per(WAVEFORM_TIMER_FREQ, 64);
    TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

volatile int rising = 0;
volatile int falling = 0;

#define RISE_FALL_COUNTER_MAX (2 * WAVEFORM_TIMER_FREQ)
volatile int rise_fall_counter = 0;
volatile float pwm_voltage = 0.0;

volatile int temp_thresh = 24;
volatile int temp = 0;
ISR(TCA1_OVF_vect) {
    temp = read_temp_i2c();
    temp_thresh = 22 + (7 * (read_voltage_adc() / 3.3));

    if (rising) {
        if (rise_fall_counter >= RISE_FALL_COUNTER_MAX) {
            rising = 0;
            rise_fall_counter = 0;
            PWM_set_voltage(2.0);
        }
        else {
            pwm_voltage = 2.0 * ((float) rise_fall_counter / (float) RISE_FALL_COUNTER_MAX);
            PWM_set_voltage(pwm_voltage);
            ++rise_fall_counter;
        }
    }
    else if (falling) {
        if (rise_fall_counter >= RISE_FALL_COUNTER_MAX) {
            falling = 0;
            rise_fall_counter = 0;
            PWM_set_voltage(0.0);
        }
        else {
            pwm_voltage = 2.0 - (2.0 * ((float) rise_fall_counter / (float) RISE_FALL_COUNTER_MAX));
            PWM_set_voltage(pwm_voltage);
            ++rise_fall_counter;
        }
    }
    else {
        rising = (temp > temp_thresh) && (pwm_voltage < 1);
        falling = (temp < temp_thresh) && (pwm_voltage > 1);
    }
    TCA1.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

void init() {
    clock_init();
    uart_init(3, 9600, NULL);

    TWI_init(&TWI0);  // temp sensor.

    TWI_init(&TWI1);  // lcd.
    LCD_init();
    Backlight_init();

    GPIO_init();
    ADC_init();
    PWM_init(100);
    Timer_init();
    sei();
}


int main() {
    init();

    char str[64];
    while (1) {
        str[0] = '\0';
        sprintf(str, "TC74: %d C\nTemp Thresh: %d C", temp, temp_thresh);
        printf(str);

        LCD_clear();
        _delay_ms(2);
        LCD_print(str);
        _delay_ms(200);
    }
}
