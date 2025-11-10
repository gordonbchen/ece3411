#include "my_util.h"
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "i2c.h"
#include "uart.h"

#define TC74_ADDR 0x48
#define TC74_READ_TEMP_CMD 0x00
int get_temp() {
    return TWI_read(TC74_ADDR, TC74_READ_TEMP_CMD);
}


void TWI1_init() {
    // TWI1: SDA (PF2), SCL (PF3).
    PORTF.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN3CTRL = PORT_PULLUPEN_bm;
    PORTF.DIRSET = PIN2_bm | PIN3_bm;

    TWI1.MBAUD = (F_CPU / (2 * 100000)) - 5;  // 100kHz.
    TWI1.MCTRLA = TWI_ENABLE_bm;
    TWI1.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

void LCD_Backlight_write(uint8_t addr, uint8_t v1, uint8_t v2) {
    TWI1.MADDR = addr << 1;  // Write mode = 0.
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    TWI1.MDATA = v1;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    TWI1.MDATA = v2;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    TWI1.MCTRLB = TWI_MCMD_STOP_gc;
}

#define LCD_ADDR 0x3E
#define LCD_CMD_MODE 0x00
#define LCD_DATA_MODE 0x40
void LCD_write(uint8_t mode, uint8_t data) {
    LCD_Backlight_write(LCD_ADDR, mode, data);
}

#define BACKLIGHT_ADDR 0x6B
void Backlight_write(uint8_t reg, uint8_t data) {
    LCD_Backlight_write(BACKLIGHT_ADDR, reg, data);
}


void Backlight_init(void) {
    Backlight_write(0x2F, 0x00);  // Reset to default.
    Backlight_write(0x04, 0xFF);  // Set max brightness.
    Backlight_write(0x00, 0x20);  // Set software shutdown.
    Backlight_write(0x07, 0x00);  // Update registers.
}


void LCD_init() {
    _delay_ms(50);                  // Wait for LCD reset
    LCD_write(LCD_CMD_MODE, 0x38);  // Function set: 2 lines, 5x8 font
    LCD_write(LCD_CMD_MODE, 0x0C);  // Display ON, cursor OFF
    LCD_write(LCD_CMD_MODE, 0x01);  // Clear display
    LCD_write(LCD_CMD_MODE, 0x06);  // Entry mode: increment
    _delay_ms(2);
}

void LCD_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            LCD_write(LCD_CMD_MODE, 0xC0);
        }
        else {
            LCD_write(LCD_DATA_MODE, *str);
        }
        ++str;
    }
}

void LCD_clear() {
    LCD_write(LCD_CMD_MODE, 0x01);  // clear screen.
    LCD_write(LCD_CMD_MODE, 0x02);  // return home.
}

int main() {
    clock_init();
    uart_init(3, 9600, NULL);
    ADC_init();
    TWI_init();

    TWI1_init();
    LCD_init();
    Backlight_init();

    char str[64];
    float voltage;
    int n_stars;
    int str_idx;
    while (1) {
        str[0] = '\0';
        voltage = get_voltage();
        str_idx = sprintf(str, "%.2f V  %d C\n", voltage, get_temp());

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
