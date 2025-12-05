#include "my_util.h"
#include "i2c.h"
#include <avr/io.h>

void clock_init() {
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
}


void DAC_init(DAC_t* dac) {
    dac->CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;
}

// RTC init, set to roughly 100 ms.
void RTC_init(void) {
    // 1. Select internal 32.768kHz oscillator.
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    // 2. Set overflow period: 32768 ticks = 1 second.
    RTC.PER = 32768 / 10;

    // 3. Enable overflow interrupt.
    RTC.INTCTRL = RTC_OVF_bm;

    // 4. Enable RTC with no prescaler.
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;
}

// Output to DAC desired voltage so it can be read by micro.
void DAC_out(DAC_t* dac, float voltage) {
    uint16_t x = (voltage / 3.3) * 1023;
    // 10-bit DAC value. upper 8 in DATAH, lower 2 in DATAL[7:6].
    dac->DATAH = (uint8_t) ((x >> 2) & 0xFF);
    dac->DATAL = (uint8_t) ((x & 0x03) << 6);
}

// Used for better looking code, simply turning character command into desired voltage.
float char_to_voltage(char c) {
    if (c == 'w') { return 0.3; }
    if (c == 'a') { return 0.9; }
    if (c == 's') { return 2.1; }
    if (c == 'd') { return 1.5; }
    return 3.3;
}


// Func only support 1ms timer.
void timer_init(TCA_t* timer) {
    timer->SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    // T = (PER + 1) * DIV / 16MHz = 250 * 64 / 16M = 1ms.
    timer->SINGLE.PERBUF = 249;
    timer->SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
    timer->SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
}

// Inititalize the accelerometer.
void LIS3DH_init() {
    TWI_init(&TWI0);

    // CTRL_REG1 (0x20)
    // Enable XYZ axes, 100Hz sample rate = 0x57
    TWI_write(&TWI0, LIS3DH_ADDR, 0x20, 0x57);

    // CTRL_REG4 (0x23)
    // High resolution mode, ±2g = 0x08
    TWI_write(&TWI0, LIS3DH_ADDR, 0x23, 0x08);
}

// Read the accelerometer, using our struct to store the results.
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

// Spi write function.
void SPI1_write(uint8_t data) {
    SPI1.DATA = data;
    while (!(SPI1.INTFLAGS & SPI_IF_bm));
}

void SPI1_init(void) {
    PORTMUX.SPIROUTEA = PORTMUX_SPI1_ALT1_gc; // PC4=MOSI, PC6=SCK
    SPI1.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV16_gc;
    SPI1.CTRLB = 0;
    PORTC.DIRSET = PIN4_bm | PIN6_bm;
}

// LED specific write function.
void APA102_sendByte(uint8_t b) {
    SPI1_write(b);
}

void APA102_startFrame(void) {
    APA102_sendByte(0x00);
    APA102_sendByte(0x00);
    APA102_sendByte(0x00);
    APA102_sendByte(0x00);
}

void APA102_endFrame(uint8_t ledCount) {
    uint8_t n = (ledCount + 15) / 16;
    for (uint8_t i = 0; i < n; i++)
        APA102_sendByte(0xFF);
}

// Use other LED functions to instruct specific brightness and color.
void APA102_sendLED(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b) {
    brightness &= 0x1F;
    APA102_sendByte(0xE0 | brightness);
    APA102_sendByte(b);
    APA102_sendByte(g);
    APA102_sendByte(r);
}

void APA102_show(uint8_t ledCount, uint8_t (*leds)[4]) {
    APA102_startFrame();
    for (uint8_t i = 0; i < ledCount; i++)
        APA102_sendLED(leds[i][0], leds[i][1], leds[i][2], leds[i][3]);
    APA102_endFrame(ledCount);
}

void APA102_init(void) {
    SPI1_init();

    // Clear LEDs on startup
    uint8_t leds[LED_COUNT][4];
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds[i][0] = 0;
        leds[i][1] = 0;
        leds[i][2] = 0;
        leds[i][3] = 0;
    }
    APA102_show(LED_COUNT, leds);
}

// Setting all LEDs based on total used (8).
void APA102_set_all(uint8_t brightness, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t leds[LED_COUNT][4];
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        leds[i][0] = brightness;
        leds[i][1] = r;
        leds[i][2] = g;
        leds[i][3] = b;
    }
    APA102_show(LED_COUNT, leds);
}
