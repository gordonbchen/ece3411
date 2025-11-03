#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/delay.h>
#include <stdio.h>
#include "uart.h"
#include "i2c_lib_S25.h"

void init_clock() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}

#define LCD_ADDR 0x3E  // 0x7C >> 1ine
#define BACKLIGHT_ADDR 0x6B
#define LCD_CMD_MODE 0x00  // Control byte: Co=0, RS=0
#define LCD_DATA_MODE 0x40  // Control byte: Co=0, RS=1

void TWI1_init() {
    PORTF.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN3CTRL = PORT_PULLUPEN_bm;
    PORTF.DIRSET = PIN2_bm | PIN3_bm;
    
    TWI1.MBAUD = (F_CPU / (2 * 100000)) - 5;  // 100kHz.
    TWI1.MCTRLA = TWI_ENABLE_bm;
    TWI1.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

void LCD_sendCommand(uint8_t cmd) {
    TWI1.MADDR = (LCD_ADDR << 1); // Write mode
    while (!(TWI1.MSTATUS & TWI_WIF_bm));
    
    TWI1.MDATA = LCD_CMD_MODE;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));
    
    TWI1.MDATA = cmd;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));
    
    TWI1.MCTRLB = TWI_MCMD_STOP_gc;
}

void Backlight_writeReg(uint8_t reg, uint8_t value) {
    // Start + address (write)
    TWI1.MADDR = (BACKLIGHT_ADDR << 1);   // 7-bit address -> write
    while (!(TWI1.MSTATUS & TWI_WIF_bm)); // wait for write

    // Register address
    TWI1.MDATA = reg;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    // Data
    TWI1.MDATA = value;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    // STOP
    TWI1.MCTRLB = TWI_MCMD_STOP_gc;
}

void Backlight_init(void) {
    // Reset to default.
    Backlight_writeReg(0x2F, 0x00);

    // Set max brightness.
    Backlight_writeReg(0x04, 0xFF);
    
    // Set software shutdown.
    Backlight_writeReg(0x00, 0x20);
    
    // Update registers.
    Backlight_writeReg(0x07, 0x00);
}


void LCD_init() {
    _delay_ms(50);  // Wait for LCD reset
    LCD_sendCommand(0x38);  // Function set: 2 lines, 5x8 font
    LCD_sendCommand(0x0C);  // Display ON, cursor OFF
    LCD_sendCommand(0x01);  // Clear display
    LCD_sendCommand(0x06);  // Entry mode: increment
    _delay_ms(2);
}

void LCD_sendData(uint8_t data) {
    TWI1.MADDR = (LCD_ADDR << 1);
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    TWI1.MDATA = LCD_DATA_MODE;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    TWI1.MDATA = data;
    while (!(TWI1.MSTATUS & TWI_WIF_bm));

    TWI1.MCTRLB = TWI_MCMD_STOP_gc;
}

void LCD_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            LCD_sendCommand(0xC0);
            str++;
        }
        else {
            LCD_sendData(*str++);
        }
    }
}

void ADC_init() {
    ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;    // input from PE0
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;     // CLK_ADC = 1 MHz
    ADC0.CTRLD = ADC_INITDLY_DLY16_gc;   // delay 16 CLK_ADC cycles (16 us)
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;   // use VDD as reference voltage
    ADC0.CTRLA = ADC_ENABLE_bm;          // enable ADC
    ADC0.COMMAND = ADC_STCONV_bm;        // start AD conversion.
}

float get_voltage() {
    while (ADC0.COMMAND & ADC_STCONV_bm);  // wait until conversion done.
    // (Ain / max 12-bit value) * 3.3V
    float voltage = ((float) ADC0.RES / 4096.0) * 3.3;
    ADC0.COMMAND = ADC_STCONV_bm;  // start next conversion.
    return voltage;
}

int main() {
    init_clock();
    uart_init(3, 9600, NULL);
    TWI1_init();
    LCD_init();
    ADC_init();
    Backlight_init();
    
    char str[64];
    
    float voltage;
    int n_stars;
    int idx;
    while (1) {
        LCD_sendCommand(0x01);  // clear screen.
        LCD_sendCommand(0x02);  // return home.
        
        voltage = get_voltage();
        str[0] = '\0';
        idx = sprintf(str, "Voltage: %.2f\n", voltage);

        n_stars = (voltage / 3.3) * 16;
        for (int i = 0; i < n_stars; ++i) {
            str[idx + i] = '*';
        }
        str[idx + n_stars] = '\0';
        
        LCD_print(str);

        _delay_ms(500);
    }
};
