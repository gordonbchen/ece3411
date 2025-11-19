#include "my_util.h"
#include "i2c.h"

void clock_init() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}


void LIS3DH_init() {
    TWI_init(&TWI0);

    // CTRL_REG1 (0x20)
    // Enable XYZ axes, 100Hz sample rate = 0x57
    TWI_write(&TWI0, LIS3DH_ADDR, 0x20, 0x57);

    // CTRL_REG4 (0x23)
    // High resolution mode, ±2g = 0x08
    TWI_write(&TWI0, LIS3DH_ADDR, 0x23, 0x08);
}

void LIS3DH_read_xyz(Coord3D* coord) {
    uint8_t xl = TWI_read(&TWI0, LIS3DH_ADDR, 0x28 | 0x80); // auto-increment
    uint8_t xh = TWI_read(&TWI0, LIS3DH_ADDR, 0x29);
    uint8_t yl = TWI_read(&TWI0, LIS3DH_ADDR, 0x2A);
    uint8_t yh = TWI_read(&TWI0, LIS3DH_ADDR, 0x2B);
    uint8_t zl = TWI_read(&TWI0, LIS3DH_ADDR, 0x2C);
    uint8_t zh = TWI_read(&TWI0, LIS3DH_ADDR, 0x2D);

    coord->x = (int16_t)(xh << 8 | xl) >> 4;   // 12-bit right-aligned
    coord->y = (int16_t)(yh << 8 | yl) >> 4;
    coord->z = (int16_t)(zh << 8 | zl) >> 4;
}