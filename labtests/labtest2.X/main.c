#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"


#define PRINT_TIME_MS 2000
volatile int print_timer_ms = 0;

volatile int curr_time_hours = 0;
volatile int curr_time_mins = 0;
volatile int curr_time_sec = 0;

volatile int countdown_set = 0;
volatile int countdown_mins = 0;
volatile int countdown_sec = 0;

volatile int countdown_paused = 0;
volatile int print_pause_status = 0;

volatile int countdown_done = 0;
#define BLINK_TIME_MS 50
volatile int blink_timer = 0;
volatile int print_countdown_done = 0;
volatile int blinks = 100;

volatile int show_hours = 1;
#define HOUR_MIN_TIME_SEC 5
volatile int hour_min_timer = 0;

void my_delay_ms(int delay_ms) {
    for (int i = 0; i < delay_ms; ++i) {
         _delay_ms(1);
    }
}

void init() {
    VPORTD.DIR = 0b11111111;
    VPORTD.OUT = 0;

    VPORTB.DIR = 1 << 3;
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;

    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
    
    // T = N * (1 + PER) / f_clock
    //   = 256 * (1 + 624) / 16 MHz = 10 ms.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.PERBUF = 624;
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV256_gc | TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    
    RTC.CLKSEL = RTC_CLKSEL_OSC1K_gc;
    while (RTC.STATUS & RTC_PERBUSY_bm) {}
    RTC.PER = 1000 / 128;
    RTC.INTCTRL = RTC_OVF_bm;
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV128_gc;

    uart_init(3, 9600, NULL);
    USART3.CTRLA = USART_RXCIE_bm;
}

ISR(TCA0_OVF_vect) {
    if (print_timer_ms < PRINT_TIME_MS) {
        print_timer_ms += 10;
    }
    if (countdown_done && (blink_timer < BLINK_TIME_MS)) {
        blink_timer += 10;
    }
    TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

ISR(RTC_CNT_vect) {
    ++curr_time_sec;
    if (curr_time_sec == 60) {
        curr_time_sec = 0;
        ++curr_time_mins;
        if (curr_time_mins == 60) {
            curr_time_mins = 0;
            ++curr_time_hours;
        }
    }
    
    if (countdown_set && (!countdown_paused) && (!countdown_done)) {
        if ((countdown_sec <= 0) && (countdown_mins <= 0)) {
            countdown_done = 1;
            print_countdown_done = 1;
        }
        else {
            --countdown_sec;
            if (countdown_sec < 0) {
                countdown_sec = 59;
                --countdown_mins;
            }
        }
    }
    
    if (hour_min_timer < HOUR_MIN_TIME_SEC) {
        ++hour_min_timer;
    }
    
    RTC.INTFLAGS = RTC_OVF_bm;
}

ISR(PORTB_PORT_vect) {
    if (!countdown_set) {
        countdown_sec = curr_time_mins;
        countdown_mins = curr_time_hours;
        countdown_set = 1;
    }
    PORTB.INTFLAGS = PIN5_bm;
}

ISR(USART3_RXC_vect) {
    char c = USART3.RXDATAL;
    if (!countdown_set) {
        return;
    }
    if (c == 'p') {
        countdown_paused = 1;
        print_pause_status = 1;
    }
    else if (c == 'r') {
        countdown_paused = 0;
        print_pause_status = 1;
    }
}

int main(void) {
    init();
    getchar();
    
    while (1) {
        printf("Hours: ");
        scanf("%d", &curr_time_hours);
        getchar();
        if ((curr_time_hours < 1) || (curr_time_hours > 12)) {
            printf("%d is not a valid hour.\n", curr_time_hours);
            continue;
        }
        break;
    }
    while (1) {   
        printf("Minutes: ");
        scanf("%d", &curr_time_mins);
        getchar();
        if ((curr_time_mins < 0) || (curr_time_mins > 59)) {
            printf("%d is not a valid minute.\n", curr_time_mins);
            continue;
        }
        break;
    }
    curr_time_sec = 0;

    sei();
    while (1) {
        if (print_timer_ms >= PRINT_TIME_MS) {
            print_timer_ms = 0;
            printf("\nCurrent time: %02d:%02d:%02d\n", curr_time_hours, curr_time_mins, curr_time_sec);
            printf("Countdown: %02d:%02d\n", countdown_mins, countdown_sec);
        }
        
        if (print_pause_status) {
            if (countdown_paused) {
                printf("Countdown paused.\n");
            }
            else {
                printf("Countdown resumed.\n");
            }
            print_pause_status = 0;
        }
        
        if (print_countdown_done) {
            print_countdown_done = 0;
            printf("\nCountdown finished!");
        }
        
        if (countdown_done && (blink_timer >= BLINK_TIME_MS) && (blinks > 0)) {
            VPORTD.OUT = ~VPORTD.OUT;
            blink_timer = 0;
            --blinks;
        }
        else if (hour_min_timer >= HOUR_MIN_TIME_SEC) {
            if (show_hours) {
                VPORTD.OUT = (char) curr_time_hours;
                VPORTB.OUT = 0;
            }
            else {
                VPORTD.OUT = (char) curr_time_mins;
                VPORTB.OUT = 1 << 3;
            }
            hour_min_timer = 0;
            show_hours = !show_hours;
        }
    }
}
