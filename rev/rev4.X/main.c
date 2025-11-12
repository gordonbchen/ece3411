#include "my_util.h"
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "i2c.h"
#include "uart.h"


int main() {
    clock_init();
    uart_init(3, 9600, NULL);
    ADC_init();

    TWI_init(&TWI0);  // temp sensor.

    TWI_init(&TWI1);  // lcd.
    LCD_init();
    Backlight_init();

    char str[64];
    float voltage;
    int n_stars;
    int str_idx;
    while (1) {
        str[0] = '\0';
        voltage = read_voltage_adc();
        str_idx = sprintf(str, "%.2f V  %d C\n", voltage, read_temp_i2c());

        n_stars = (voltage / 3.3) * 16;
        for (int i = 0; i < n_stars; ++i) {
            str[str_idx] = '*';
            ++str_idx;
        }
        str[str_idx] = '\0';
        printf("%s\n", str);

        LCD_clear();
        _delay_ms(2);
        LCD_print(str);
        _delay_ms(500);
    }
}
