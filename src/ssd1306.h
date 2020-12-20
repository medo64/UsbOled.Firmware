#ifndef SSD1306_H
#define	SSD1306_H

#include <stdint.h>

/** Initializes Display. */
void ssd1306_init(const uint8_t address, const uint8_t width, const uint8_t height);


/** Turns display off. */
void ssd1306_displayOff();

/** Turns display on. */
void ssd1306_displayOn();

/** Sets contrast value. */
void ssd1306_setContrast(const uint8_t value);


/** Sets column and row to be used (at 8x8 resolution). */
bool ssd1306_moveTo(const uint8_t row, const uint8_t column);

/** Moves cursor to the first column of the next row. */
bool ssd1306_moveToNextRow();


/** Clear display content. */
void ssd1306_clearAll();

/** Clear display content of a single row. */
bool ssd1306_clearRow(const uint8_t row);


/** Writes custom 8x8 character at the current position */
bool ssd1306_drawCharacter(const uint8_t* data, const uint8_t count, bool isLarge);

/** Writes 8x8 character at the current position */
bool ssd1306_writeCharacter(const uint8_t value, bool isLarge);

/** Writes 8x8 text at the current position */
bool ssd1306_writeText(const uint8_t* text, bool isLarge);

#endif
