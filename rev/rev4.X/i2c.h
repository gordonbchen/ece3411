#ifndef I2C_H
#define I2C_H

#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>

#define TWI_WRITE_MODE 0
#define TWI_READ_MODE 1

TWI_t* TWI_get(int n);
void TWI_init(TWI_t* twi);

void TWI_stop(TWI_t* twi);
void TWI_addr(TWI_t* twi, uint8_t addr, uint8_t mode);

int TWI_transmit(TWI_t* twi, uint8_t data);
uint8_t TWI_receive(TWI_t* twi);

void TWI_write(TWI_t* twi, uint8_t addr, uint8_t cmd, uint8_t data);
uint8_t TWI_read(TWI_t* twi, uint8_t addr, uint8_t cmd);


#define TC74_ADDR 0x48
#define TC74_READ_TEMP_CMD 0x00
static inline int read_temp_i2c() {
    return TWI_read(&TWI0, TC74_ADDR, TC74_READ_TEMP_CMD);
}


#define LCD_ADDR 0x3E
#define LCD_CMD_MODE 0x00
#define LCD_DATA_MODE 0x40
// static inline void LCD_write(uint8_t mode, uint8_t data) {
//     TWI_write(&TWI1, LCD_ADDR, mode, data);
// }

#define BACKLIGHT_ADDR 0x6B
// static inline void Backlight_write(uint8_t reg, uint8_t data) {
//     TWI_write(&TWI1, BACKLIGHT_ADDR, reg, data);
// }


void Backlight_init();
void LCD_init();
void LCD_print(const char *str);
void LCD_clear();

#endif
