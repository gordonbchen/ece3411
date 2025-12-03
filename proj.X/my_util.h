#ifndef MYUTIL_H
#define MYUTIL_H

#define F_CPU 16000000UL
#include <avr/io.h>

void clock_init();

void DAC_init(DAC_t* dac);
void DAC_out(DAC_t* dac, float voltage);
float char_to_voltage(char c);

void timer_init(TCA_t* timer);

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} Coord3D;

#define LIS3DH_ADDR 0x18
void LIS3DH_init();
void LIS3DH_read_xyz(Coord3D* coord);


#define LED_COUNT 8

void SPI1_init(void);
void APA102_init(void);

void APA102_set_all(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b);
void APA102_show(uint8_t ledCount, uint8_t (*leds)[4]);

#endif