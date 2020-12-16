#ifndef SSD1306_H
#define	SSD1306_H

#include <stdint.h>

void ssd1306_init(uint8_t address, uint8_t width, uint8_t height);

void ssd1306_displayOff();
void ssd1306_displayOn();

void ssd1306_setContrast(uint8_t value);

#endif
