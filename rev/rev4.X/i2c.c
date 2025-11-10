#include "i2c.h"

void TWI_init() {
    // Pull up SDA (PA2) and SCLK (PA3).
    PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;

    // SCLK = 80 kHz, rise_time = 10ns.
    // BAUD = (F_CPU / (2 * SCLK)) - 5 - (F_CPU * rise_time / 2)
    TWI0.MBAUD = 95;
    TWI0.MCTRLA |= TWI_ENABLE_bm;         // enable I2C.
    TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc; // start in idle state.
}

void TWI_stop() {
    TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

void TWI_addr(uint8_t addr, uint8_t mode) {
    while (1) {
        // Send addr and mode.
        uint8_t addr_byte = (addr << 1) | mode;
        TWI0.MADDR = addr_byte;

        // Wait until R/W interrupt flag is set.
        uint8_t iflag = (mode == TWI_WRITE_MODE) ? TWI_WIF_bm : TWI_RIF_bm;
        while (!(TWI0.MSTATUS & iflag));

        // Stop and try again if client sends NACK.
        if (TWI0.MSTATUS & TWI_RXACK_bm) {
            TWI_stop();
        }

        // Break if no errors.
        if (!(TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm))) {
            break;
        }
    }
}

int TWI_transmit(uint8_t data) {
    TWI0.MDATA = data;
    while (!(TWI0.MSTATUS, TWI_WIF_bm));  // Wait for Write Interrupt Flag (WIF).

    // Return -1 for error, 0 for success.
    if (TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {
        return -1;
    }
    return 0;
}

uint8_t TWI_receive() {
    while (!(TWI0.MSTATUS, TWI_RIF_bm));  // Wait for Read Interrupt Flag (RIF).
    uint8_t data = TWI0.MDATA;
    TWI0.MCTRLB |= TWI_ACKACT_bm;  // Send NACK.
    return data;
}

void TWI_write(uint8_t addr, uint8_t cmd, uint8_t data) {
    TWI_addr(addr, TWI_WRITE_MODE);
    TWI_transmit(cmd);
    TWI_transmit(data);
    TWI_stop();
}

uint8_t TWI_read(uint8_t addr, uint8_t cmd) {
    TWI_addr(addr, TWI_WRITE_MODE);
    TWI_transmit(cmd);
    TWI_addr(addr, TWI_READ_MODE);
    uint8_t data = TWI_receive();
    TWI_stop();
    return data;
}
