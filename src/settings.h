#ifndef SETTINGS_H
#define	SETTINGS_H

#define SETTING_DEFAULT_I2C_ADDRESS         0x3C
#define SETTING_DEFAULT_I2C_SPEED_INDEX     1
#define SETTING_DEFAULT_DISPLAY_HEIGHT      64
#define SETTING_DEFAULT_DISPLAY_BRIGHTNESS  0xCF
#define SETTING_DEFAULT_DISPLAY_INVERSE     0


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

#endif	/* SETTINGS_H */
