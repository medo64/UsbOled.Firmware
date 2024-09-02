#pragma once

#include <xc.h>


void led_init(void);

#define led_activity_off()      TRISA4 = 0
#define led_activity_on()       TRISA4 = 1
#define led_activity_toggle()   TRISA4 = !TRISA4
