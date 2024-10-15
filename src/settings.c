#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "settings.h"
#include "Microchip/usb_device.h"


void settings_init(void) {
    uint8_t* settingsPtr = (uint8_t*)&Settings;
    for (uint8_t i = 0; i < sizeof(Settings); i++) {
        *settingsPtr = _SETTINGS_PROGRAM[i];
        settingsPtr++;
    }
}

void settings_save(void) {
    bool hadInterruptsEnabled = (INTCONbits.GIE != 0);  // save if interrupts enabled
    INTCONbits.GIE = 0;  // disable interrupts
    PMCON1bits.WREN = 1;  // enable writes

    uint16_t address = _SETTINGS_FLASH_LOCATION;
    uint8_t* settingsPtr = (uint8_t*)&Settings;

    // erase
    PMADR = address;         // set location
    PMCON1bits.CFGS = 0;     // program space
    PMCON1bits.FREE = 1;     // erase
    PMCON2 = 0x55;           // unlock
    PMCON2 = 0xAA;           // unlock
    PMCON1bits.WR = 1;       // begin erase
    asm("NOP"); asm("NOP");  // forced

    // write
    for (uint8_t i = 1; i <= sizeof(Settings); i++) {
        unsigned latched = (i == sizeof(Settings)) ? 0 : 1;  // latch load is done for all except last
        PMADR = address;            // set location
        PMDATH = 0x3F;              // same as when erased
        PMDATL = *settingsPtr;      // load data
        PMCON1bits.CFGS = 0;        // program space
        PMCON1bits.LWLO = (uint8_t)latched;  // load write latches
        PMCON2 = 0x55;              // unlock
        PMCON2 = 0xAA;              // unlock
        PMCON1bits.WR = 1;          // begin write
        asm("NOP"); asm("NOP");     // forced
        address++;                  // move write address
        settingsPtr++;              // move data pointer
    }

    PMCON1bits.WREN = 0;  // disable writes
    if (hadInterruptsEnabled) { INTCONbits.GIE = 1; }  // restore interrupts
}


uint8_t settings_getI2CAddress(void) {
    uint8_t value = Settings.I2CAddress;
    return (value > 0) ? value : SETTING_DEFAULT_I2C_ADDRESS;
}

void settings_setI2CAddress(const uint8_t value) {
    Settings.I2CAddress = value;
}


uint8_t settings_getI2CSpeedIndex(void) {
    uint8_t value = Settings.I2CSpeedIndex;
    if ((value == 0) || (value > 10)) { value = SETTING_DEFAULT_I2C_SPEED_INDEX; }
    return value;
}

void settings_setI2CSpeedIndex(const uint8_t value) {
    Settings.I2CSpeedIndex = value;
}


uint8_t settings_getDisplayHeight(void) {
    if (Settings.DisplayHeight == 32) {
        return 32;
    } else if (Settings.DisplayHeight == 128) {
        return 128;
    } else {
        return 64;
    }
}

void settings_setDisplayHeight(const uint8_t value) {
    Settings.DisplayHeight = value;
}


uint8_t settings_getDisplayBrightness(void) {
    return Settings.DisplayBrightness;
}

void settings_setDisplayBrightness(const uint8_t value) {
    Settings.DisplayBrightness = value;
}


bool settings_getDisplayInverse(void) {
    return (Settings.DisplayInverse != 0);
}

void settings_setDisplayInverse(const bool value) {
    Settings.DisplayInverse = value ? 1 : 0;
}


bool settings_getDisplayFlip(void) {
    return (Settings.DisplayFlip != 0);
}

void settings_setDisplayFlip(const bool value) {
    Settings.DisplayFlip = value ? 1 : 0;
}
