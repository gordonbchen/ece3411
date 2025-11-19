#ifndef MYUTIL_H
#define MYUTIL_H

#define F_CPU 16000000UL
#include <avr/io.h>

void clock_init();


typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} Coord3D;

#define LIS3DH_ADDR 0x18
void LIS3DH_init();
void LIS3DH_read_xyz(Coord3D* coord);

#endif