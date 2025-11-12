#include "my_util.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <util/delay.h>
#include "i2c.h"
#include "uart.h"

void GPIO_init() {
    VPORTE.DIR = 0;
    VPORTD.DIR = 0b11111111;
    VPORTA.DIR = 0b11111111;
}

void Button_init() {
    VPORTB.DIR = 0;
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}

volatile int TC74_standby_changed = 0;
volatile int TC74_standby = 0;
ISR(PORTB_PORT_vect) {
    TC74_standby = !TC74_standby;
    TC74_standby_changed = 1;
    PORTB.INTFLAGS = PIN2_bm;
}

void DAC_init() {
    // DAC0 outputs to PD6.
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;
}

#define DAC_FREQ_HZ 1000
int DAC_calc_per() {
    // NOTE: assumes DIV64.
    // (DIV / F_CPU) * PER = (1 / (freq_hz * N_SINES))
    // PER = F_CPU / (DIV * freq_hz * N_SINES)
    return (int) (((int32_t) F_CPU) / ((int32_t) DAC_FREQ_HZ * 64));
}

float LM335Z_read_temp() {
    return (read_voltage_adc() * 100.0) - 273.3;
}

void SPI_init() {
    // Route SPI0: MOSI = PA4, SCK = PA5, SS = PA7.
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_DEFAULT_gc;
    
    // Enable SPI0 in Master mode, set prescaler = 128.
    SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV128_gc;
    
    // Enable SS pin control.
    SPI0.CTRLB = SPI_SSD_bm;
}

void SPI0_write(uint8_t data) {
    SPI0.DATA = data;
    // Block until transfer complete.
    while (!(SPI0.INTFLAGS & SPI_IF_bm));
}

// MCP4131 Commands
#define MCP4131_WRITE_CMD 0x00  // Write command to potentiometer.
#define MCP4131_WIPER0 0x00     // Target wiper 0.

void MCP4131_set_wiper(uint8_t value) {
    PORTA.OUTCLR = PIN7_bm;  // CS low.
    SPI0_write(MCP4131_WRITE_CMD | MCP4131_WIPER0);  // Send command.
    SPI0_write(value);
    PORTA.OUTSET = PIN7_bm;  // CS high.
}

void Timer_init() {
    TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA1.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    TCA1.SINGLE.PERBUF = DAC_calc_per();
    TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

volatile int rising = 0;
volatile int falling = 0;

#define RISE_FALL_COUNTER_MAX (2 * DAC_FREQ_HZ)
volatile int rise_fall_counter = 0;
volatile float dac_out = 0.0;
volatile int vpeak = 0;
volatile int vfloor = 1;

volatile float LM335Z_temp = 0.0;
volatile float TC74A0_temp = 0.0;

ISR(TCA1_OVF_vect) {
    LM335Z_temp = LM335Z_read_temp();
    if (!TC74_standby) {TC74A0_temp = read_temp_i2c() - 1;}

    if (rising) {
        if (rise_fall_counter >= RISE_FALL_COUNTER_MAX) {
            rising = 0;
            rise_fall_counter = 0;
            vfloor = 0;
            vpeak = 1;
        }
        else {
            dac_out = 1023.0 * ((float)rise_fall_counter / 2000.0);
            ++rise_fall_counter;
            MCP4131_set_wiper((128.0 / 2000.0) * (float)rise_fall_counter);
        }
    }
    else if (falling) {
        if (rise_fall_counter >= RISE_FALL_COUNTER_MAX) {
            falling = 0;
            rise_fall_counter = 0;
            vfloor = 1;
            vpeak = 0;
        }
        else {
            dac_out = 1023.0 - (1023.0 * ((float)rise_fall_counter / 2000.0));
            ++rise_fall_counter;
            MCP4131_set_wiper(128.0 - ((128.0 / 2000.0) * (float)rise_fall_counter));
        }
    }
    else {
        if ((LM335Z_temp > TC74A0_temp) && vfloor) {
            rising = 1;
        }
        else if ((LM335Z_temp < TC74A0_temp) && vpeak) {
            falling = 1;
        }
    }

    uint16_t y = dac_out;
    // 10-bit DAC value.
    // upper 8 in DATAH, lower 2 in DATAL[7:6].
    DAC0.DATAL = (uint8_t) ((y & 0x03) << 6);
    DAC0.DATAH = (uint8_t) ((y >> 2) & 0xFF);

    TCA1.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

char get_status_sign() {
    if (rising) return 0b10110010;
    if (falling) return 0b11010011;
    if (vpeak) return 0b10111011;
    return 0b11010101;
}

void init() {
    clock_init();
    uart_init(3, 9600, NULL);

    TWI_init(&TWI0);  // temp sensor.

    TWI_init(&TWI1);  // lcd.
    LCD_init();
    Backlight_init();

    GPIO_init();
    Button_init();
    ADC_init();
    DAC_init();
    Timer_init();
    SPI_init();
    sei();
}


int main() {
    init();

    char str[64];
    while (1) {
        if (TC74_standby_changed) {
            cli();
            TWI_write(&TWI0, TC74_ADDR, 0x01, TC74_standby << 7);
            if (TC74_standby) {
                Backlight_write(0x04, 0x88);  // Set brightness.
                Backlight_write(0x07, 0x00);  // Update registers.
                _delay_ms(100);
            }
            else {
                Backlight_write(0x04, 0xFF);  // Set brightness.
                Backlight_write(0x07, 0x00);  // Update registers.
                _delay_ms(100);
            }
            TC74_standby_changed = 0;
            sei();
        }

        str[0] = '\0';
        sprintf(
            str, "T1:%.2fC T2:%dC\nWave Stats:%c %d",
            LM335Z_temp, (int)TC74A0_temp, get_status_sign(), TC74_standby
        );
        printf(str);

        LCD_clear();
        _delay_ms(2);
        LCD_print(str);
        _delay_ms(200);
    }
}
