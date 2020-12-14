#ifndef IO_H
#define IO_H

#include <xc.h>


void led_init();

#define led_activity_off()      LATA4 = 1
#define led_activity_on()       LATA4 = 0
#define led_activity_toggle()   LATA4 = !LATA4

#endif
