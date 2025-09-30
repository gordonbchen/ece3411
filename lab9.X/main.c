#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile char duty_cycle = 50;

volatile char button2_down = 0;
volatile char button5_down = 0;

void init() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;

    VPORTB.DIR = 0b00000000;
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
    PORTB.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;

    VPORTD.DIR = 0b11111111;
    VPORTD.OUT = 0;

    // PWM freq of 1000Hz, period = 1ms.
    // period = prescaler * (1 + PER) / F_CPU
    // PER = (period * F_CPU / prescaler) - 1
    // PER = (1/1000 * 16000000 / 64) - 1 = 16000 / 64 - 1 = 250 - 1 = 249.
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    // single slope PWM output on D2.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP2EN_bm;

    TCA0.SINGLE.PERBUF = 249;
    TCA0.SINGLE.CMP2BUF = TCA0.SINGLE.PERBUF * duty_cycle / 100;

    sei();
}

ISR(PORTB_PORT_vect) {
    if (PORTB.INTFLAGS & PIN2_bm) {
        button2_down = !(VPORTB.IN & PIN2_bm);
        PORTB.INTFLAGS = PIN2_bm;
    }
    if (PORTB.INTFLAGS & PIN5_bm) {
        button5_down = !(VPORTB.IN & PIN5_bm);
        PORTB.INTFLAGS = PIN5_bm;
    }
}

int main(void) {
    init();

    while (1) {
        _delay_ms(20);
        if (button2_down && (duty_cycle < 95)) {
            ++duty_cycle;
        }
        if (button5_down && (duty_cycle > 5)) {
            --duty_cycle;
        }
        TCA0.SINGLE.CMP2BUF = TCA0.SINGLE.PERBUF * duty_cycle / 100;
    }
}
