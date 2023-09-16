#include <xc.h>

void led_init() {
    LATA4 = 0;  // active is always off (open-drain))
    TRISA4 = 1; // keep it on by default
}
