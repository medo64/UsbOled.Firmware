#ifndef SSD1306_H
#define	SSD1306_H

#include <stdint.h>

void ssd1306_init(uint8_t address);

void ssd1306_displayOff();
void ssd1306_displayOn();

#endif
