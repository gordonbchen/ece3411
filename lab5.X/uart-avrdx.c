/*
 * AVRDx specific UART code
 */

/* CPU frequency */
#define F_CPU 16000000UL

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

void* usart_init(uint8_t usartnum, uint32_t baud_rate)
{
    USART_t* usart;

    if (usartnum == 0) {
        usart = &USART0;
        // enable USART0 TX pin
        PORTA.DIRSET = PIN0_bm;
    }
    else if (usartnum == 1) {
        usart = &USART1;
        // enable USART1 TX pin
        PORTC.DIRSET = PIN0_bm;
    }
    else if (usartnum == 2) {
        usart = &USART2;
        // enable USART2 TX pin
        PORTF.DIRSET = PIN0_bm;
    }
    else if (usartnum == 3) {
        usart = &USART3;
        // enable USART3 TX pin
        PORTB.DIRSET = PIN0_bm;
    } 
    else {
        usart = NULL;
    }

    // set BAUD and CTRLB registers
    uint16_t baud_setting = (F_CPU * 64) / (16 * baud_rate);
    usart->BAUD = baud_setting;

    usart->CTRLB = USART_TXEN_bm | USART_RXEN_bm;

    return usart;
}

void usart_transmit_data(void* ptr, char c)
{
    USART_t* usart = (USART_t*)ptr;
    
    // Send data.
    usart->TXDATAL = c;
}

void usart_wait_until_transmit_ready(void *ptr)
{
    USART_t* usart = (USART_t*)ptr;
    
    // Wait until ready to transmit.
    while (!(usart->STATUS & USART_DREIF_bm));
}

int usart_receive_data(void* ptr)
{
    USART_t* usart = (USART_t*)ptr;
    
    // Wait until data has arrived and then return the data
    while (!(usart->STATUS & USART_RXCIF_bm));
    return usart->RXDATAL;
}
