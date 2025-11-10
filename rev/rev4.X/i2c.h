#ifndef I2C_H
#define I2C_H

#include <avr/io.h>
#include <util/twi.h>

#define TWI_WRITE_MODE 0
#define TWI_READ_MODE 1

void TWI_init();

void TWI_stop();
void TWI_addr(uint8_t addr, uint8_t mode);

int TWI_transmit(uint8_t data);
uint8_t TWI_receive();

void TWI_write(uint8_t addr, uint8_t cmd, uint8_t data);
uint8_t TWI_read(uint8_t addr, uint8_t cmd);

#endif
