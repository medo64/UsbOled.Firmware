#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "settings.h"
#include "Microchip/usb_device.h"

#define SETTING_DEFAULT_ADDRESS         0x3C
#define SETTING_DEFAULT_DISPLAY_HEIGHT  64

#define _SETTINGS_FLASH_RAW { SETTING_DEFAULT_ADDRESS, SETTING_DEFAULT_DISPLAY_HEIGHT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } //reserving space because erase block is block 32-word (32-bytes as only low bytes are used)
#define _SETTINGS_FLASH_LOCATION 0x1FE0
const uint8_t _SETTINGS_PROGRAM[] __at(_SETTINGS_FLASH_LOCATION) = _SETTINGS_FLASH_RAW;

typedef struct {
    uint8_t Address;
    uint8_t DisplayHeight;
} SettingsRecord;

SettingsRecord Settings;

void settings_init() {
    uint8_t* settingsPtr = (uint8_t*)&Settings;
    for (uint8_t i = 0; i < sizeof(Settings); i++) {
        *settingsPtr = _SETTINGS_PROGRAM[i];
        settingsPtr++;
    }
}

void settings_save() {
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
        PMCON1bits.LWLO = latched;  // load write latches
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


uint8_t settings_getOledI2CAddress() {
    uint8_t value = Settings.Address;
    return (value > 0) ? value : 0x3C;
}

void settings_setOledI2CAddress(uint8_t value) {
    Settings.Address = value;
}


uint8_t settings_getDisplayHeight() {
    switch (Settings.DisplayHeight) {
        case 32: return 32;
        default: return 64;
    }
}

void settings_setDisplayHeight(uint8_t height) {
    Settings.DisplayHeight = height;
}
