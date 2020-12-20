#ifndef SETTINGS_H
#define	SETTINGS_H

/** Initializes settings. */
void settings_init();

/** Saves settings to EEPROM. */
void settings_save();


/** Gets OLED's I2C address. */
uint8_t settings_getOledI2CAddress();

/** Sets OLED's I2C address. */
void settings_setOledI2CAddress(const uint8_t value);

/** Gets screen height. Only 64 (A-type) and 32 (B-type) are supported. */
uint8_t settings_getDisplayHeight();

/** Sets screen height. */
void settings_setDisplayHeight(const uint8_t value);

#endif	/* SETTINGS_H */
