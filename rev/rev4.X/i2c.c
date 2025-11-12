#include "i2c.h"

// Only supports TWI0 and TWI1.
TWI_t* TWI_get(int n) {
    if (n == 0) return &TWI0;
    return &TWI1;
}

void TWI_init(TWI_t* twi) {
    // Pull up SDA and SCLK.
    if (twi == &TWI0) {
        // SDA (PA2), SCLK (PA3).
        PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
        PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;
    }
    else if (twi == &TWI1) {
        // SDA (PF2), SCLK (PF3).
        PORTF.PIN2CTRL |= PORT_PULLUPEN_bm;
        PORTF.PIN3CTRL |= PORT_PULLUPEN_bm;
    }

    // SCLK = 80 kHz, rise_time = 10ns.
    // BAUD = (F_CPU / (2 * SCLK)) - 5 - (F_CPU * rise_time / 2)
    twi->MBAUD = 95;
    twi->MCTRLA |= TWI_ENABLE_bm;         // enable I2C.
    twi->MSTATUS |= TWI_BUSSTATE_IDLE_gc; // start in idle state.
}

void TWI_stop(TWI_t* twi) {
    twi->MCTRLB |= TWI_MCMD_STOP_gc;
}

void TWI_addr(TWI_t* twi, uint8_t addr, uint8_t mode) {
    while (1) {
        // Send addr and mode.
        twi->MADDR = (addr << 1) | mode;

        // Wait until R/W interrupt flag is set.
        uint8_t iflag = (mode == TWI_WRITE_MODE) ? TWI_WIF_bm : TWI_RIF_bm;
        while (!(twi->MSTATUS & iflag));

        // Stop and try again if client sends NACK.
        if (twi->MSTATUS & TWI_RXACK_bm) {
            TWI_stop(twi);
        }

        // Break if no errors.
        if (!(twi->MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm))) {
            break;
        }
    }
}

int TWI_transmit(TWI_t* twi, uint8_t data) {
    twi->MDATA = data;
    while (!(twi->MSTATUS & TWI_WIF_bm));  // Wait for Write Interrupt Flag (WIF).

    // Return -1 for error, 0 for success.
    if (twi->MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {
        return -1;
    }
    return 0;
}

uint8_t TWI_receive(TWI_t* twi) {
    while (!(twi->MSTATUS & TWI_RIF_bm));  // Wait for Read Interrupt Flag (RIF).
    uint8_t data = twi->MDATA;
    twi->MCTRLB |= TWI_ACKACT_bm;  // Send NACK.
    return data;
}

void TWI_write(TWI_t* twi, uint8_t addr, uint8_t cmd, uint8_t data) {
    TWI_addr(twi, addr, TWI_WRITE_MODE);
    TWI_transmit(twi, cmd);
    TWI_transmit(twi, data);
    TWI_stop(twi);
}

uint8_t TWI_read(TWI_t* twi, uint8_t addr, uint8_t cmd) {
    TWI_addr(twi, addr, TWI_WRITE_MODE);
    TWI_transmit(twi, cmd);
    TWI_addr(twi, addr, TWI_READ_MODE);
    uint8_t data = TWI_receive(twi);
    TWI_stop(twi);
    return data;
}


// LCD.
// void LCD_Backlight_write(uint8_t addr, uint8_t v1, uint8_t v2) {
//     TWI1.MADDR = addr << 1;
//     while (!(TWI1.MSTATUS & TWI_WIF_bm));

//     TWI1.MDATA = v1;
//     while (!(TWI1.MSTATUS & TWI_WIF_bm));

//     TWI1.MDATA = v2;
//     while (!(TWI1.MSTATUS & TWI_WIF_bm));

//     TWI1.MCTRLB = TWI_MCMD_STOP_gc;
// }

void LCD_write(uint8_t mode, uint8_t data) {
    TWI_write(&TWI1, LCD_ADDR, mode, data);
//     LCD_Backlight_write(LCD_ADDR, mode, data);
}

void LCD_init() {
    _delay_ms(50);                          // Wait for LCD reset
    LCD_write(LCD_CMD_MODE, 0x38);  // Function set: 2 lines, 5x8 font
    LCD_write(LCD_CMD_MODE, 0x0C);  // Display ON, cursor OFF
    LCD_write(LCD_CMD_MODE, 0x01);  // Clear display
    LCD_write(LCD_CMD_MODE, 0x06);  // Entry mode: increment
    _delay_ms(50);
}

void LCD_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            LCD_write(LCD_CMD_MODE, 0xC0);
        }
        else {
            LCD_write(LCD_DATA_MODE, *str);
        }
        ++str;
    }
}

void LCD_clear() {
    LCD_write(LCD_CMD_MODE, 0x01);  // clear screen.
    LCD_write(LCD_CMD_MODE, 0x02);  // return home.
}


void Backlight_write(uint8_t reg, uint8_t data) {
    TWI_write(&TWI1, BACKLIGHT_ADDR, reg, data);
//     LCD_Backlight_write(BACKLIGHT_ADDR, reg, data);
}

void Backlight_init() {
    _delay_ms(50);
    Backlight_write(0x2F, 0x00);  // Reset to default.
    Backlight_write(0x04, 0xFF);  // Set max brightness.
    Backlight_write(0x00, 0x20);  // Set software shutdown.
    Backlight_write(0x07, 0x00);  // Update registers.
    _delay_ms(50);
}