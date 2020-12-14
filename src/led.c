#include <xc.h>

void led_init() {
    LATA4 = 1;  // start with Active off
    TRISA4 = 0; // enable Active
}
