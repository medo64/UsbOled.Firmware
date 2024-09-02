#include <stdint.h>
#include <xc.h>
#include "font.h"
#include "system.h"
#include "i2c_master.h"
#include "ssd1306.h"

#define SSD1306_SET_LOWER_START_COLUMN_ADDRESS       0x00
#define SSD1306_SET_UPPER_START_COLUMN_ADDRESS       0x10
#define SSD1306_SET_MEMORY_ADDRESSING_MODE           0x20
#define SSD1306_SET_COLUMN_ADDRESS                   0x21
#define SSD1306_SET_PAGE_ADDRESS                     0x22
#define SSD1306_SET_DISPLAY_START_LINE               0x40
#define SSD1306_SET_CONTRAST_CONTROL                 0x81
#define SSD1306_SET_CHARGE_PUMP                      0x8D
#define SSD1306_SET_SEGMENT_REMAP                    0xA1
#define SSD1306_ENTIRE_DISPLAY_ON                    0xA4
#define SSD1306_ENTIRE_DISPLAY_ON_FORCED             0xA5
#define SSD1306_SET_NORMAL_DISPLAY                   0xA6
#define SSD1306_SET_INVERSE_DISPLAY                  0xA7
#define SSD1306_SET_DISPLAY_OFF                      0xAE
#define SSD1306_SET_DISPLAY_ON                       0xAF
#define SSD1306_SET_MULTIPLEX_RATIO                  0xA8
#define SSD1306_SET_PAGE_START_ADDRESS               0xB0
#define SSD1306_SET_COM_OUTPUT_SCAN_DIRECTION        0xC8
#define SSD1306_SET_DISPLAY_OFFSET                   0xD3
#define SSD1306_SET_DISPLAY_CLOCK_DIVIDE_RATIO       0xD5
#define SSD1306_SET_PRECHARGE_PERIOD                 0xD9
#define SSD1306_SET_COM_PINS_HARDWARE_CONFIGURATION  0xDA
#define SSD1306_SET_VCOMH_DESELECT_LEVEL             0xDB

void ssd1306_writeRawCommand1(const uint8_t value);
void ssd1306_writeRawCommand2(const uint8_t value1, const uint8_t value2);
void ssd1306_writeRawCommand3(const uint8_t value1, const uint8_t value2, const uint8_t value3);
void ssd1306_writeRawData(const uint8_t* value, const uint8_t count);
void ssd1306_writeRawDataZeros(const uint8_t count);

uint8_t displayAddress;
uint8_t displayWidth;
uint8_t displayHeight;
uint8_t displayColumns;
uint8_t displayRows;

uint8_t currentRow;
uint8_t currentColumn;

void ssd1306_init(const uint8_t address, const uint8_t baudRateCounter, const uint8_t width, const uint8_t height) {
    displayAddress = address;
    displayWidth = width;
    displayHeight = height;
    displayColumns = width / 8;
    displayRows = height / 8;

    //reset I2C bus
    LATC0 = 0;                                // set clock low
    LATC1 = 0;                                // set data low
    TRISC0 = 1;                               // clock pin configured as output
    TRISC1 = 1;                               // data pin configured as input
    for (unsigned char j=0; j<3; j++) {       // repeat 3 times with alternating data
        for (unsigned char i=0; i<10; i++) {  // 9 cycles + 1 extra
            TRISC0 = 0; __delay_us(50);       // force clock low
            TRISC0 = 1; __delay_us(50);       // release clock high
        }
        TRISC1 = !TRISC1;                     // toggle data line
    }

    i2c_master_init(baudRateCounter);

    ssd1306_writeRawCommand1(SSD1306_SET_DISPLAY_OFF);                                    // Set Display Off
    ssd1306_writeRawCommand2(SSD1306_SET_DISPLAY_CLOCK_DIVIDE_RATIO, 0x80);               // Set Display Clock Divide Ratio/Oscillator Frequency
    ssd1306_writeRawCommand2(SSD1306_SET_MULTIPLEX_RATIO, height - 1);                    // Set Multiplex Ratio (line count - 1)
    ssd1306_writeRawCommand2(SSD1306_SET_DISPLAY_OFFSET, 0x00);                           // Set Display Offset
    ssd1306_writeRawCommand1(SSD1306_SET_DISPLAY_START_LINE);                             // Set Display Start Line 
    ssd1306_writeRawCommand2(SSD1306_SET_CHARGE_PUMP, 0x14);                              // Set Charge Pump (0x10 external vcc, 0x14 internal vcc)
    ssd1306_writeRawCommand1(SSD1306_SET_SEGMENT_REMAP);                                  // Set Segment Re-Map
    ssd1306_writeRawCommand1(SSD1306_SET_COM_OUTPUT_SCAN_DIRECTION);                      // Set COM Output Scan Direction
    switch (height) {
        case 32:
            ssd1306_writeRawCommand2(SSD1306_SET_COM_PINS_HARDWARE_CONFIGURATION, 0x02);  // Set COM Pins Hardware Configuration (0x02 128x32, 0x12 128x64)
            ssd1306_writeRawCommand2(SSD1306_SET_CONTRAST_CONTROL, 0x8F);                 // Set Contrast Control (0x8F 128x32; 0x9F 128x64 external vcc; 0xCF 128x64 internal vcc)
            break;
        case 64:
            ssd1306_writeRawCommand2(SSD1306_SET_COM_PINS_HARDWARE_CONFIGURATION, 0x12);  // Set COM Pins Hardware Configuration (0x02 128x32, 0x12 128x64)
            ssd1306_writeRawCommand2(SSD1306_SET_CONTRAST_CONTROL, 0xCF);                 // Set Contrast Control (0x8F 128x32; 0x9F 128x64 external vcc; 0xCF 128x64 internal vcc)
            break;
    }
    ssd1306_writeRawCommand2(SSD1306_SET_PRECHARGE_PERIOD, 0xF1);                         // Set Pre-Charge Period (0x22 external vcc; 0xF1 internal vcc)
    ssd1306_writeRawCommand2(SSD1306_SET_VCOMH_DESELECT_LEVEL, 0x40);                     // Set VCOMH Deselect Level
    ssd1306_writeRawCommand1(SSD1306_ENTIRE_DISPLAY_ON);                                  // Set Entire Display On/Off
    ssd1306_writeRawCommand1(SSD1306_SET_NORMAL_DISPLAY);                                 // Set Normal Display

    ssd1306_writeRawCommand2(SSD1306_SET_MEMORY_ADDRESSING_MODE, 0b10);                  // Set Page addressing mode

    ssd1306_writeRawCommand1(SSD1306_SET_DISPLAY_ON);                                     // Set Display On
}


void ssd1306_displayOff() {
    ssd1306_writeRawCommand1(SSD1306_SET_DISPLAY_OFF);
}

void ssd1306_displayOn() {
    ssd1306_writeRawCommand1(SSD1306_SET_DISPLAY_ON);
}

void ssd1306_displayInvert() {
    ssd1306_writeRawCommand1(SSD1306_SET_INVERSE_DISPLAY);
}

void ssd1306_displayNormal() {
    ssd1306_writeRawCommand1(SSD1306_SET_NORMAL_DISPLAY);
}

void ssd1306_setContrast(const uint8_t value) {
    ssd1306_writeRawCommand2(SSD1306_SET_CONTRAST_CONTROL, value);
}


void ssd1306_clearAll() {
    for (uint8_t i = 0; i < displayRows; i++) {
        ssd1306_writeRawCommand1(SSD1306_SET_PAGE_START_ADDRESS | i);
        ssd1306_writeRawDataZeros(displayWidth);
    }
    ssd1306_moveTo(1, 1);
}

void ssd1306_clearRemaining(const bool isLarge) {
    uint8_t columnCount = (uint8_t)((displayColumns - currentColumn) << 3);
    uint8_t nextColumn = currentColumn + 1;
    uint8_t nextRow = currentRow + 1;
    ssd1306_writeRawDataZeros(columnCount);
    if (isLarge) {
        ssd1306_moveTo(nextRow + 1, nextColumn);
        ssd1306_writeRawDataZeros(columnCount);
    }
    ssd1306_moveTo(nextRow, nextColumn);
}

bool ssd1306_clearRow(const uint8_t row) {
    if (ssd1306_moveTo(row, 1)) {
        ssd1306_writeRawDataZeros(displayWidth);
        return true;
    }
    return false;
}


bool ssd1306_moveTo(const uint8_t row, const uint8_t column) {
    if ((row <= displayRows) && (column <= displayColumns)) {
        uint8_t newRow = (row == 0) ? currentRow : row - 1;
        uint8_t newColumn = (column == 0) ? currentColumn : column - 1;
        uint8_t newColumnL = (newColumn << 3) & 0x0F;
        uint8_t newColumnH = (newColumn >> 1) & 0x0F;
        ssd1306_writeRawCommand1(SSD1306_SET_PAGE_START_ADDRESS | newRow);
        ssd1306_writeRawCommand1(SSD1306_SET_LOWER_START_COLUMN_ADDRESS | newColumnL);
        ssd1306_writeRawCommand1(SSD1306_SET_UPPER_START_COLUMN_ADDRESS | newColumnH);
        currentRow = newRow;
        currentColumn = newColumn;
        return true;
    }
    return false;
}

bool ssd1306_moveToNextRow(void) {
    if (currentRow >= displayRows - 1) { return false; }
    uint8_t newRow = currentRow + 1;
    ssd1306_writeRawCommand1(SSD1306_SET_PAGE_START_ADDRESS | newRow);
    ssd1306_writeRawCommand1(SSD1306_SET_LOWER_START_COLUMN_ADDRESS);
    ssd1306_writeRawCommand1(SSD1306_SET_UPPER_START_COLUMN_ADDRESS);
    currentRow = newRow;
    currentColumn = 0;
    return true;
}


bool ssd1306_drawCharacter(const uint8_t* data, const uint8_t count) {
    if ((count != 8) && (count != 16)) { return false; }  // must be 8x8 or 8x16 pixels - 8/16 bytes
    if (currentColumn >= displayColumns) { return false; }

    if (count >= 16) {
        ssd1306_writeRawCommand1(SSD1306_SET_PAGE_START_ADDRESS | (currentRow + 1));
        ssd1306_writeRawData(&data[8], 8);

        uint8_t currentColumnLow = (currentColumn << 3) & 0x0F;
        uint8_t currentColumnHigh = (currentColumn >> 1) & 0x0F;
        ssd1306_writeRawCommand1(SSD1306_SET_PAGE_START_ADDRESS | currentRow);
        ssd1306_writeRawCommand1(SSD1306_SET_LOWER_START_COLUMN_ADDRESS | currentColumnLow);
        ssd1306_writeRawCommand1(SSD1306_SET_UPPER_START_COLUMN_ADDRESS | currentColumnHigh);
        ssd1306_writeRawData(&data[0], 8);
    } else {
        ssd1306_writeRawData(data, 8);
    }
    currentColumn++;

    return true;
}

bool ssd1306_writeCharacter(const char value, const bool isLarge) {
    uint8_t data[16];
    if ((value <= 32) || (value >= 127)) {
        for (uint8_t i = 0; i < sizeof(data); i++) { data[i] = 0; }
    } else {
        if (isLarge) {
            uint16_t offset = (uint16_t)((value - 33) << 4);  // *16
            for (uint8_t i = 0; i < 16; i++) {
                data[i] = font_8x16[offset + i];
            }
        } else {
            uint16_t offset = (uint16_t)((value - 33) << 3);  // *8
            for (uint8_t i = 0; i < 8; i++) {
                data[i] = font_8x8[offset + i];
            }
        }
    }
    return ssd1306_drawCharacter(data, isLarge ? 16 : 8);
}

bool ssd1306_writeText(const char* text, const bool isLarge) {
    bool ok = true;
    while (*text != 0) {
        ok &= ssd1306_writeCharacter(*text, isLarge);
        text++;
    }
    return ok;
}


void ssd1306_writeRawCommand1(const uint8_t value1) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x00);
   i2c_master_writeByte(value1);
   i2c_master_stop();
}

void ssd1306_writeRawCommand2(const uint8_t value1, const uint8_t value2) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x00);
   i2c_master_writeByte(value1);
   i2c_master_writeByte(value2);
   i2c_master_stop();
}

void ssd1306_writeRawCommand3(const uint8_t value1, const uint8_t value2, const uint8_t value3) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x00);
   i2c_master_writeByte(value1);
   i2c_master_writeByte(value2);
   i2c_master_writeByte(value3);
   i2c_master_stop();
}

void ssd1306_writeRawData(const uint8_t *value, const uint8_t count) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x40);
   for (uint8_t i = 0; i < count; i++) {
       i2c_master_writeByte(*value);
       value++;
   }
   i2c_master_stop();
}

void ssd1306_writeRawDataZeros(const uint8_t count) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x40);
   for (uint8_t i = 0; i < count; i++) {
       i2c_master_writeByte(0x00);
   }
   i2c_master_stop();
}
