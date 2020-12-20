#include <stdint.h>
#include "Microchip/usb.h"
#include "Microchip/usb_device.h"
#include "Microchip/usb_device_cdc.h"
#include "buffer.h"
#include "led.h"
#include "settings.h"
#include "ssd1306.h"
#include "system.h"

bool processText(const uint8_t* data, const uint8_t count, const bool useLargeFont);
bool processCommand(const uint8_t* data, const uint8_t count);
void initOled(void);
uint8_t nibbleToHex(const uint8_t value);
bool hexToNibble(const uint8_t hex, uint8_t* nibble);

#define LED_TIMEOUT       2000
#define LED_TIMEOUT_NONE  65535
uint16_t LedTimeout = LED_TIMEOUT_NONE;

void main(void) {
    init();
    led_init();

    led_activity_on();

    settings_init();
    initOled();
    ssd1306_setContrast(settings_getBrightness());

    led_activity_off();

#if defined(USB_INTERRUPT)
    interruptsEnable();
#endif
    
    USBDeviceInit();
    USBDeviceAttach();

    led_activity_off();
    
    while(true) {
        if (LedTimeout != LED_TIMEOUT_NONE) {
            if (LedTimeout == 0) {
                led_activity_off();
                LedTimeout = LED_TIMEOUT_NONE;
            } else {
                LedTimeout--;
            }
        }

#if defined(USB_POLLING)
        USBDeviceTasks();
#endif

        if (USBGetDeviceState() < CONFIGURED_STATE) { continue; }
        if (USBIsDeviceSuspended()) { continue; }

        CDCTxService();

        // USB receive
        uint8_t readCount = getsUSBUSART(UsbReadBuffer, USB_READ_BUFFER_MAX); //until the buffer is free.
        if (readCount > 0) {
            led_activity_on(); LedTimeout = LED_TIMEOUT;
            for (uint8_t i = 0; i < readCount; i++) {  // copy to buffer
                uint8_t value = UsbReadBuffer[i];
                if (InputBufferCount < INPUT_BUFFER_MAX) {
                    InputBuffer[InputBufferCount] = value;
                    InputBufferCount++;
                } else {
                    InputBufferCorrupted = true;  // no more buffer; darn it
                }
            }
        }

        // USB send
        if ((OutputBufferCount > 0) && USBUSARTIsTxTrfReady()) {  // send output if TX is ready
            led_activity_on(); LedTimeout = LED_TIMEOUT;
            uint8_t writeCount = 0;
            for (uint8_t i = 0; i < USB_WRITE_BUFFER_MAX; i++) {  // copy to output buffer
                if (i < OutputBufferCount) {
                    UsbWriteBuffer[i] = OutputBuffer[i];
                    writeCount++;
                } else {
                    break;
                }
            }
            if (writeCount > 0) {
                putUSBUSART(&UsbWriteBuffer[0], writeCount);  // send data
                OutputBufferCount -= writeCount;  // reduce buffer for the length used
                buffer_copy(&OutputBuffer[0], &OutputBuffer[writeCount], OutputBufferCount);  // move buffer content to beginning
            }
        }

        // Process line
        if (InputBufferCount > 0) {
            uint8_t offset = 0;
            bool potentialCrLf = false;

            for (uint8_t i = 0; i < InputBufferCount; i++) {  // find EOLs
                uint8_t value = InputBuffer[i];

                if (value == 0x0A) {  // start line processing
                    uint8_t* dataPtr = &InputBuffer[offset];
                    uint8_t dataCount = i - offset - (potentialCrLf ? 1 : 0);
                    bool wasOk;

                    if (*dataPtr == 0x09) {  // HT: command mode
                        dataPtr++;
                        dataCount--;
                        wasOk = processCommand(dataPtr, dataCount);
                    } else {
                        bool useLarge = false;
                        bool processingPrefixes = true;
                        while (processingPrefixes) {
                            switch (*dataPtr) {
                                case 0x07:  // BEL: clear screen
                                    ssd1306_clearAll();
                                    dataPtr++;
                                    dataCount--;
                                    break;

                                case 0x08:  // BS: move to origin
                                    ssd1306_moveTo(1, 1);
                                    dataPtr++;
                                    dataCount--;
                                    break;

                                case 0x0B:  // VT: double-size font
                                    dataPtr++;
                                    dataCount--;
                                    useLarge = true;
                                    break;

                                case 0x0C:  // FF: not used
                                    break;

                                default:
                                    processingPrefixes = false;  // special characters can only be at the beginning of line
                                    break;
                            }
                            if (dataCount == 0) { break; }
                        }

                        if (dataCount > 0) {
                            wasOk &= processText(dataPtr, dataCount, useLarge);
                        } else {
                            ssd1306_moveToNextRow();
                            if (useLarge) { ssd1306_moveToNextRow(); }  //extra move for large font
                        }
                    }

                    if (!wasOk) { OutputBufferAppend('!'); }
                    if (potentialCrLf) { OutputBufferAppend(0x0D); }  // CR was seen before LF
                    OutputBufferAppend(0x0A);

                    offset = i + 1;  // set the next start
                }

                potentialCrLf = (value == 0x0D);  // checked when LF is matched
            }

            if (offset > 0) {  // move unused portion of buffer to the start
                InputBufferCount -= offset;
                buffer_copy(&InputBuffer[0], &InputBuffer[offset], InputBufferCount);
            }
        }
    }
}


void __interrupt() SYS_InterruptHigh(void) {
    #if defined(USB_INTERRUPT)
        USBDeviceTasks();
    #endif
}


void initOled(void) {
    ssd1306_init(settings_getOledI2CAddress(), 128, settings_getDisplayHeight());
    ssd1306_clearAll();

    ssd1306_writeText("    USB OLED    ", true);
    ssd1306_moveToNextRow();
    ssd1306_moveToNextRow();
    ssd1306_writeText("   medo64.com   ", false);
    ssd1306_moveToNextRow();
}


bool processText(const uint8_t* data, const uint8_t count, const bool useLargeFont) {
    bool ok = true;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t value = *data;
        if (value >= 32) {
            if (useLargeFont) {
                ok &= ssd1306_writeCharacter(value, true);
            } else {
                ok &= ssd1306_writeCharacter(value, false);
            }
        } else {
            ok = false;
        }
        data++;
    }
    return ok;
}

bool processCommand(const uint8_t* data, const uint8_t count) {
    switch (*data) {

        case '#': {  // set screen size
            if (count == 2) {
                switch(*++data) {
                    case 'A': settings_setDisplayHeight(64); break;
                    case 'B': settings_setDisplayHeight(32); break;
                    default: return false;
                }
                initOled();
                return true;
            }
        }

        case '*': {  // set brightness
            if (count == 3) {
                uint8_t brightness;
                if (!hexToNibble(*++data, &brightness)) { return false; }
                if (!hexToNibble(*++data, &brightness)) { return false; }
                settings_setBrightness(brightness);
                ssd1306_setContrast(brightness);
                return true;
            }
        }

        case '?': {  // print settings
            if (count == 1) {
                uint8_t address = settings_getOledI2CAddress();
                uint8_t height = settings_getDisplayHeight();
                if (OutputBufferCount > OUTPUT_BUFFER_HIGH) { return false; }  // if we're high on usage, reject printing stuff out
                OutputBufferAppend('@');
                OutputBufferAppend(nibbleToHex(address >> 4));  // high nibble
                OutputBufferAppend(nibbleToHex(address));  // low nibble
                OutputBufferAppend(' ');
                OutputBufferAppend('#');
                switch (height) {
                    case 32: OutputBufferAppend('B'); break;
                    default: OutputBufferAppend('A'); break;
                }
                return true;
            }
        }

        case '@': {  // select address
            if (count == 3) {
                uint8_t address;
                if (!hexToNibble(*++data, &address)) { return false; }
                if (!hexToNibble(*++data, &address)) { return false; }
                settings_setOledI2CAddress(address);
                initOled();
                return true;
            }
        }

        case '^': {  // save permanently
            if (count == 1) {
                settings_save();
                return true;
            }
        }

        case '~': {  // defaults
            if (count == 1) {
                settings_setOledI2CAddress(SETTING_DEFAULT_ADDRESS);
                settings_setDisplayHeight(SETTING_DEFAULT_DISPLAY_HEIGHT);
                settings_setBrightness(SETTING_DEFAULT_BRIGHTNESS);
                settings_save();
                return true;
            }
        }

        case 'c': {
            if (count == 17) {
                uint8_t customCharData[8];
                for (uint8_t i = 0; i < 8; i++) {
                    if (!hexToNibble(*++data, &customCharData[i])) { return false; }
                    if (!hexToNibble(*++data, &customCharData[i])) { return false; }
                }
                return ssd1306_drawCharacter(&customCharData[0], 8, false);
            }
        }

        case 'C': {
            if (count == 33) {
                uint8_t customCharData[16];
                for (uint8_t i = 0; i < 16; i++) {
                    if (!hexToNibble(*++data, &customCharData[i])) { return false; }
                    if (!hexToNibble(*++data, &customCharData[i])) { return false; }
                }
                return ssd1306_drawCharacter(&customCharData[0], 16, true);
            }
        }

        case 'm': {
            if (count == 3) {
                uint8_t row = 0;
                if (!hexToNibble(*++data, &row)) { return false; }
                if (!hexToNibble(*++data, &row)) { return false; }
                return ssd1306_moveTo(row, 1);
            } else if (count == 5) {
                uint8_t row, column;
                if (!hexToNibble(*++data, &row)) { return false; }
                if (!hexToNibble(*++data, &row)) { return false; }
                if (!hexToNibble(*++data, &column)) { return false; }
                if (!hexToNibble(*++data, &column)) { return false; }
                return ssd1306_moveTo(row, column);
            }
        }
    }

    return false;
}

uint8_t nibbleToHex(const uint8_t value) {
    uint8_t nibble = value & 0x0F; //just do it on lowest 4 bits
    if (nibble < 10) {
        return 0x30 + nibble; //number (0-9)
    } else {
        return 0x37 + nibble; //character (A-F)
    }
}

bool hexToNibble(const uint8_t hex, uint8_t* nibble) {
    *nibble <<= 4;  // move nibble up
   if ((hex >= 0x30) && (hex <= 0x39)) {
        *nibble |= hex - 0x30;
    } else if ((hex >= 0x41) && (hex <= 0x46)) {
        *nibble |= hex - 0x37;
    } else if ((hex >= 0x61) && (hex <= 0x66)) {
        *nibble |= hex - 0x57;
    } else {
        return false;
    }
    return true;
}