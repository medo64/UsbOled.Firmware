#include <stdint.h>
#include <xc.h>
#include "hardware.h"
#include "i2c_master.h"
#include "ssd1306.h"

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
#define SSD1306_SET_COM_OUTPUT_SCAN_DIRECTION        0xC8
#define SSD1306_SET_DISPLAY_OFFSET                   0xD3
#define SSD1306_SET_DISPLAY_CLOCK_DIVIDE_RATIO       0xD5
#define SSD1306_SET_PRECHARGE_PERIOD                 0xD9
#define SSD1306_SET_COM_PINS_HARDWARE_CONFIGURATION  0xDA
#define SSD1306_SET_VCOMH_DESELECT_LEVEL             0xDB

void ssd1306_writeRawCommandByte(uint8_t value);
void ssd1306_writeRawCommandTwoBytes(uint8_t value1, uint8_t value2);


uint8_t displayAddress;
uint8_t displayWidth;
uint8_t displayHeight;

void ssd1306_init(uint8_t address, uint8_t width, uint8_t height) {
    displayAddress = address;
    displayWidth = width;
    displayHeight = height;

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

    i2c_master_init();

    ssd1306_writeRawCommandByte(SSD1306_SET_DISPLAY_OFF);                               // Set Display Off
    ssd1306_writeRawCommandTwoBytes(SSD1306_SET_DISPLAY_CLOCK_DIVIDE_RATIO, 0x80);      // Set Display Clock Divide Ratio/Oscillator Frequency
    ssd1306_writeRawCommandTwoBytes(SSD1306_SET_MULTIPLEX_RATIO, height - 1);           // Set Multiplex Ratio (line count - 1)
    ssd1306_writeRawCommandTwoBytes(SSD1306_SET_DISPLAY_OFFSET, 0x00);                  // Set Display Offset
    ssd1306_writeRawCommandByte(SSD1306_SET_DISPLAY_START_LINE);                        // Set Display Start Line 
    ssd1306_writeRawCommandTwoBytes(SSD1306_SET_CHARGE_PUMP, 0x14);                     // Set Charge Pump (0x10 external vcc, 0x14 internal vcc)
    ssd1306_writeRawCommandByte(SSD1306_SET_SEGMENT_REMAP);                             // Set Segment Re-Map
    ssd1306_writeRawCommandByte(SSD1306_SET_COM_OUTPUT_SCAN_DIRECTION);                 // Set COM Output Scan Direction
    switch (height) {
        case 32:
            ssd1306_writeRawCommandTwoBytes(SSD1306_SET_COM_PINS_HARDWARE_CONFIGURATION, 0x02);  // Set COM Pins Hardware Configuration (0x02 128x32, 0x12 128x64)
            ssd1306_writeRawCommandTwoBytes(SSD1306_SET_CONTRAST_CONTROL, 0x8F);                 // Set Contrast Control (0x8F 128x32; 0x9F 128x64 external vcc; 0xCF 128x64 internal vcc)
            break;
        case 64:
            ssd1306_writeRawCommandTwoBytes(SSD1306_SET_COM_PINS_HARDWARE_CONFIGURATION, 0x12);  // Set COM Pins Hardware Configuration (0x02 128x32, 0x12 128x64)
            ssd1306_writeRawCommandTwoBytes(SSD1306_SET_CONTRAST_CONTROL, 0xCF);                 // Set Contrast Control (0x8F 128x32; 0x9F 128x64 external vcc; 0xCF 128x64 internal vcc)
            break;
    }
    ssd1306_writeRawCommandTwoBytes(SSD1306_SET_PRECHARGE_PERIOD, 0xF1);                // Set Pre-Charge Period (0x22 external vcc; 0xF1 internal vcc)
    ssd1306_writeRawCommandTwoBytes(SSD1306_SET_VCOMH_DESELECT_LEVEL, 0x40);            // Set VCOMH Deselect Level
    ssd1306_writeRawCommandByte(SSD1306_ENTIRE_DISPLAY_ON_FORCED);                      // Set Entire Display On/Off
    ssd1306_writeRawCommandByte(SSD1306_SET_NORMAL_DISPLAY);                            // Set Normal/Inverse Display
    ssd1306_writeRawCommandByte(SSD1306_SET_DISPLAY_ON);                                // Set Display On
}


void ssd1306_displayOff() {
    ssd1306_writeRawCommandByte(SSD1306_SET_DISPLAY_OFF);
}

void ssd1306_displayOn() {
    ssd1306_writeRawCommandByte(SSD1306_SET_DISPLAY_ON);
}


void ssd1306_writeRawCommandByte(uint8_t value) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x00);
   i2c_master_writeByte(value);
   i2c_master_stop();
}

void ssd1306_writeRawCommandTwoBytes(uint8_t value1, uint8_t value2) {
   i2c_master_startWrite(displayAddress);
   i2c_master_writeByte(0x00);
   i2c_master_writeByte(value1);
   i2c_master_writeByte(value2);
   i2c_master_stop();
}
