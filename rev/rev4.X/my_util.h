#ifndef MYUTIL_H
#define MYUTIL_H

#define F_CPU 16000000UL
#include <avr/io.h>

void clock_init();

void ADC_init();
float read_voltage_adc();

#endif
