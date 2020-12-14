#include <xc.h>
#include <stdint.h>
#include "hardware.h"

void init(void) {
    interrupt_disable();

    // Oscillator
    OSCCONbits.IRCF = 0b1111;  // 16 MHz or 48 MHz HF
    OSCCONbits.SPLLMULT = 1;   // 3x PLL is enabled
    OSCCONbits.SPLLEN = 1;     // PLL is enabled
    ACTCONbits.ACTSRC = 1;     // The HFINTOSC oscillator is tuned using Fll-speed USB events
    ACTCONbits.ACTEN = 1;      // ACT is enabled, updates to OSCTUNE are exclusive to the ACT
    
    // USB Init
    //LATA0 = 0;   // D+
    //LATA1 = 0;   // D-
    //TRISA0 = 0;  // D+
    //TRISA1 = 0;  // D-
}

void ready() {
    interrupt_enable();
}


void wait_1ms() {
    __delay_ms(1);
}

void wait_10ms() {
    __delay_ms(10);
}

void wait_short(void) {
    __delay_ms(150);
}
