#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// AVR128DB48 uses Harvard architecture
// program memory (instructions) separate from data memory (variables)

// sine table stored in Flash (program memory) to save RAM.
// 10-bit resolution: [0, 1023]
// centered at 512 (DC offset)
const uint16_t sine_table[64] PROGMEM = {
    512, 562, 612, 660, 707, 752, 794, 833, 868, 900, 927, 950, 968,
    982, 991, 995, 995, 991, 982, 968, 950, 927, 900, 868, 833, 794,
    752, 707, 660, 612, 562, 512, 461, 411, 363, 316, 271, 229, 190,
    155, 123, 95,  72,  53,  39,  30,  25,  24,  28,  37,  50,  68,
    90,  116, 146, 180, 218, 259, 303, 350, 399, 449, 500, 551};

void DAC_init(void) {
    // OUTEN -> buffered output at PD6
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
    // voltage = VDD * (value / 1024)
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;
}

int main(void) {
    DAC_init();

    uint8_t i = 0;
    while (1) {
        // read value from lookup table (in program memory)
        uint16_t sample = pgm_read_word(&sine_table[i]);

        // DATAL[7:6] = DACn.DATA[1:0]
        // DATAH = DACn.DATA[1:0]
        DAC0.DATAL = (sample & 0x03) << 6;
        DAC0.DATAH = (sample >> 2) & 0xFF;

        i = (i + 1) % 64;

        // delay_s = (1 / freq_hz) / 64
        _delay_us(5);
    }

    return 0;
}
