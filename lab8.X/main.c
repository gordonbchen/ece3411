#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "uart.h"

typedef struct Time {
    int hours;
    int minutes;
    int seconds;
} Time_t;

volatile Time_t curr_time = {.hours = 12, .minutes = 30, .seconds = 0};
volatile Time_t alarm_time = {.hours = 12, .minutes = 30, .seconds = 15};

void print_time(volatile Time_t* time) {
    printf("%02d:%02d:%02d\n", time->hours, time->minutes, time->seconds);
}

int ge_time(volatile Time_t* t1, volatile Time_t* t2) {
    int sec1 = (t1->hours * 3600) + (t1->minutes * 60) + t1->seconds;
    int sec2 = (t2->hours * 3600) + (t2->minutes * 60) + t2->seconds;
    return sec1 >= sec2;
}

void inc_time(volatile Time_t* time) {
    ++time->seconds;
    if (time->seconds == 60) {
        time->seconds = 0;
        ++time->minutes;
        
        if (time->minutes == 60) {
            time->minutes = 0;
            ++time->hours;
        }
    }
}

#define PRINT_TIME 5
volatile int print_timer = PRINT_TIME;

#define BLINK_TIME 1
volatile int blink_timer = BLINK_TIME;
volatile int alarm_triggered = 0;

void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;  
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

void init_RTC() {
    // use 32.768 kHz clock.
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
    
    // set overflow to 32768 ticks = 1sec.
    while (RTC.STATUS & RTC_PERBUSY_bm) {}
    RTC.PER = 32768;
    
    // enable overflow interrupt.
    RTC.INTCTRL = RTC_OVF_bm;
    
    // enable RTC, no prescaling.
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;
    
    // enable global interrupts.
    sei();
}

ISR(RTC_CNT_vect) {
    inc_time(&curr_time);

    if (print_timer > 0) {
        --print_timer;
    }
    
    if (alarm_triggered && (blink_timer > 0)) {
        --blink_timer;
    }

    // clear interrupt.
    RTC.INTFLAGS = RTC_OVF_bm;
}

int main() {
    init_clock();
    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
    VPORTD.DIR = 0b11111111;
    init_RTC();
 
    while (1) {
        if ((!alarm_triggered) && ge_time(&curr_time, &alarm_time)) {
            alarm_triggered = 1;
        }
        
        if (print_timer == 0) {
            printf("\nCurrent Time: ");
            print_time(&curr_time);
            
            printf("Alarm Time: ");
            print_time(&alarm_time);
            
            if (!alarm_triggered) {
                printf("Status: Waiting...\n");
            }
            
            print_timer = PRINT_TIME;
        }
        
        if (blink_timer == 0) {
            VPORTD.OUT = ~VPORTD.OUT;
            blink_timer = BLINK_TIME;
        }
    }
}