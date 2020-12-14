#ifndef SSD1306_H
#define	SSD1306_H

#include <stdint.h>

void ssd1306_init(uint8_t address);

void ssd1306_writeRawCommandByte(uint8_t value);
void ssd1306_writeRawCommandTwoBytes(uint8_t value1, uint8_t value2);

#endif
