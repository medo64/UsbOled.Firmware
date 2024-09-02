#pragma once

#define SETTING_DEFAULT_I2C_ADDRESS         0x3C
#define SETTING_DEFAULT_I2C_SPEED_INDEX     1
#define SETTING_DEFAULT_DISPLAY_HEIGHT      64
#define SETTING_DEFAULT_DISPLAY_BRIGHTNESS  0xCF
#define SETTING_DEFAULT_DISPLAY_INVERSE     0

#define _SETTINGS_FLASH_RAW {                                                                \
                              SETTING_DEFAULT_I2C_ADDRESS,                                   \
                              SETTING_DEFAULT_I2C_SPEED_INDEX,                               \
                              SETTING_DEFAULT_DISPLAY_HEIGHT,                                \
                              SETTING_DEFAULT_DISPLAY_BRIGHTNESS,                            \
                              SETTING_DEFAULT_DISPLAY_INVERSE,                               \
                              0,                                                             \
                              26, 0x03,                                                      \
                              'E', 0, 'A', 0, '9', 0, 'E', 0,                                \
                              '1', 0, '9', 0, '7', 0, '9', 0,                                \
                              '2', 0, '8', 0, '0', 0, '1', 0                                 \
                            }  // reserving space because erase block is block 32-word (32-bytes as only low bytes are used)
#define _SETTINGS_FLASH_LOCATION 0x1FE0
const uint8_t _SETTINGS_PROGRAM[] __at(_SETTINGS_FLASH_LOCATION) = _SETTINGS_FLASH_RAW;

typedef struct {
    uint8_t I2CAddress;
    uint8_t I2CSpeedIndex;
    uint8_t DisplayHeight;
    uint8_t DisplayBrightness;
    uint8_t DisplayInverse;
    uint8_t Reserved;
    uint8_t UsbSerialLength;
    uint8_t UsbSerialType;
    uint8_t UsbSerialValue[24];
} SettingsRecord;

SettingsRecord Settings;


/** Initializes settings. */
void settings_init();

/** Saves settings to EEPROM. */
void settings_save();


/** Gets OLED's I2C address. */
uint8_t settings_getI2CAddress();

/** Sets OLED's I2C address. */
void settings_setI2CAddress(const uint8_t value);


/** Gets OLED's I2C speed (in 100kHz). */
uint8_t settings_getI2CSpeedIndex();

/** Sets OLED's I2C speed (in 100kHz). */
void settings_setI2CSpeedIndex(const uint8_t value);


/** Gets screen height. Only 64 (A-type) and 32 (B-type) are supported. */
uint8_t settings_getDisplayHeight();

/** Sets screen height. */
void settings_setDisplayHeight(const uint8_t value);


/** Gets OLED's brightness. */
uint8_t settings_getDisplayBrightness();

/** Sets OLED's brightness. */
void settings_setDisplayBrightness(const uint8_t value);


/** Gets if OLED's display is inverted. */
bool settings_getDisplayInverse();

/** Sets if OLED's display is inverted. */
void settings_setDisplayInverse(const bool value);
