#ifndef SETTINGS_H
#define	SETTINGS_H

#define SETTING_DEFAULT_I2C_ADDRESS         0x3C
#define SETTING_DEFAULT_I2C_SPEED_LOW       100
#define SETTING_DEFAULT_I2C_SPEED_HIGH      0
#define SETTING_DEFAULT_DISPLAY_HEIGHT      64
#define SETTING_DEFAULT_DISPLAY_BRIGHTNESS  0xCF


/** Initializes settings. */
void settings_init();

/** Saves settings to EEPROM. */
void settings_save();


/** Gets OLED's I2C address. */
uint8_t settings_getI2CAddress();

/** Sets OLED's I2C address. */
void settings_setI2CAddress(const uint8_t value);

/** Gets OLED's I2C speed. */
uint16_t settings_getI2CSpeed();

/** Sets OLED's I2C speed. */
void settings_setI2CSpeed(const uint16_t value);

/** Gets screen height. Only 64 (A-type) and 32 (B-type) are supported. */
uint8_t settings_getDisplayHeight();

/** Sets screen height. */
void settings_setDisplayHeight(const uint8_t value);

/** Gets OLED's brightness. */
uint8_t settings_getDisplayBrightness();

/** Sets OLED's brightness. */
void settings_setDisplayBrightness(const uint8_t value);

#endif	/* SETTINGS_H */
