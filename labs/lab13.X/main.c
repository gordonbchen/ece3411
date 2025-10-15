#include <avr/io.h>
#include <avr/cpufunc.h>
#include "src/rtc.h"
#include "src/adc.h"
#include "src/dac.h"
#include "src/uart.h"

void clock_init(void);
void GPIO_init(void);
void OPAMP0_init(void);

int main(void) {
    clock_init();
    GPIO_init();
    LED0_init();
    RTC_init(); /*1s interrupt timer*/
    USART_init(); /*Streams data into Data Visualizer*/

    DAC0_init(); /*Creates and outputs the sine waveform*/
    ADC0_init(); /*Acquire Signal from OPAMP0 output*/

    /*Start the timer controlling the sine waveform generator
    and the timer controlling the acquisition*/
    DAC0_SineWaveTimer_enable();
    ADC0_SampleTimer_enable();

    OPAMP0_init();

    RTC_enable();

    sei(); /*Enable global interrupts*/

    while (1) {}
}

#define OPAMP_TIMEBASE_US (ceil(F_CPU / 1e6) - 1)
#define OPAMP_MAX_SETTLE (0x7F)

void OPAMP0_init(void) {
    /* Configure the Timebase */
    OPAMP.TIMEBASE = OPAMP_TIMEBASE_US;

    /* Configure the Op Amp n Control A */
    OPAMP.OP0CTRLA = OPAMP_OP0CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;

    /* Configure the Op Amp n Input Multiplexer */
    OPAMP.OP0INMUX = OPAMP_OP0INMUX_MUXNEG_OUT_gc | OPAMP_OP0INMUX_MUXPOS_DAC_gc;

    /* Configure the Op Amp n Resistor Wiper Multiplexer */
    OPAMP.OP0RESMUX = OPAMP_OP0RESMUX_MUXBOT_OFF_gc |
                      OPAMP_OP0RESMUX_MUXWIP_WIP0_gc |
                      OPAMP_OP0RESMUX_MUXTOP_OFF_gc;

    /* Configure the Op Amp n Settle Time*/
    OPAMP.OP0SETTLE = OPAMP_MAX_SETTLE;

    /* Enable OPAMP peripheral */
    OPAMP.CTRLA = OPAMP_ENABLE_bm;

    /* Wait for the operational amplifiers to settle */
    while (!(OPAMP.OP0STATUS & OPAMP_SETTLED_bm)) {}
}

void GPIO_init(void) {
    /* Disable digital input buffer*/
    PORTD.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

void clock_init() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
    _delay_ms(100);
}
