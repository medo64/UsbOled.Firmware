#include <xc.h>

void io_init(void) {
    LATA4 = 0;  // active is always off (open-drain))
    TRISA4 = 1; // keep it on by default

    LATC4 = 0;  // turn on by default (just for compatibility with I2C boards, pullup is always present on Oled FEC)
    TRISC4 = 0;
}
