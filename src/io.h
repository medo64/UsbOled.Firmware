#pragma once

#include <xc.h>


void io_init(void);

#define io_led_activity_off()      TRISA4 = 0
#define io_led_activity_on()       TRISA4 = 1
#define io_led_activity_toggle()   TRISA4 = !TRISA4

#define io_pullup_on()             LC4 = 0
#define io_pullup_off()            LC4 = 1
