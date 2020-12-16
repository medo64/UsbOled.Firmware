#ifndef SSD1306_H
#define	SSD1306_H

#include <stdint.h>

/** Initializes Display. */
void ssd1306_init(uint8_t address, uint8_t width, uint8_t height);


/** Turns display off. */
void ssd1306_displayOff();

/** Turns display on. */
void ssd1306_displayOn();

/** Sets contrast value. */
void ssd1306_setContrast(uint8_t value);


/** Clear display content. */
void ssd1306_clearAll();

/** Clear display content of a single row. */
void ssd1306_clearRow(uint8_t y);

/** Writes character at the current position */
void ssd1306_writeCharacter(const uint8_t value);

/** Writes character at the current position */
void ssd1306_writeText(const uint8_t* value);

#endif
