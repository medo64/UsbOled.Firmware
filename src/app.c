#include <stdint.h>
#include "Microchip/usb.h"
#include "Microchip/usb_device.h"
#include "Microchip/usb_device_cdc.h"
#include "buffer.h"
#include "settings.h"
#include "system.h"
#include "led.h"
#include "ssd1306.h"

bool processText(uint8_t* data, const uint8_t count, const bool useLargeFont);
bool processCommand(uint8_t* data, const uint8_t count);
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
                    uint8_t firstChar = InputBuffer[offset];

                    if (firstChar == 0x09) {  // HT: command mode
                        uint8_t dataOffset = offset + 1;
                        uint8_t dataCount = i - offset - 1 - (potentialCrLf ? 1 : 0);
                        bool wasOk = processCommand(&InputBuffer[dataOffset], dataCount);
                        if (!wasOk) { OutputBufferAppend('!'); }
                        if (potentialCrLf) { OutputBufferAppend(0x0D); }  // CR was seen before LF
                        OutputBufferAppend(0x0A);
                    } else {
                        uint8_t dataOffset;
                        uint8_t dataCount;
                        bool useLarge;
                        bool moveIfEmpty;
                        switch (firstChar) {
                            case 0x07:  // BEL: clear screen
                                ssd1306_clearAll();
                                dataOffset = offset + 1;
                                dataCount = i - offset - 1 - (potentialCrLf ? 1 : 0);
                                useLarge = false;
                                moveIfEmpty = false;
                                break;
                            case 0x08:  // BS: move to origin
                                ssd1306_moveTo(1, 1);
                                dataOffset = offset + 1;
                                dataCount = i - offset - 1 - (potentialCrLf ? 1 : 0);
                                useLarge = false;
                                moveIfEmpty = false;
                                break;
                            case 0x0B:  // VT: reserved
                                dataOffset = offset + 1;
                                dataCount = i - offset - 1 - (potentialCrLf ? 1 : 0);
                                useLarge = false;
                                moveIfEmpty = false;
                                break;
                            case 0x0C:  // FF: changes to double-size font
                                dataOffset = offset + 1;
                                dataCount = i - offset - 1 - (potentialCrLf ? 1 : 0);
                                useLarge = true;
                                moveIfEmpty = true;
                                break;
                            default:  // text
                                dataOffset = offset;
                                dataCount = i - offset - (potentialCrLf ? 1 : 0);
                                useLarge = false;
                                moveIfEmpty = true;
                                break;
                        }

                        bool wasOk = processText(&InputBuffer[dataOffset], dataCount, useLarge);
                        if ((dataCount > 0) || moveIfEmpty) {
                            ssd1306_moveToNextRow();
                            if (useLarge) { ssd1306_moveToNextRow(); }  //extra move for large font
                        }

                        if (!wasOk) { OutputBufferAppend('!'); }
                        if (potentialCrLf) { OutputBufferAppend(0x0D); }  // CR was seen before LF
                        OutputBufferAppend(0x0A);
                    }

                    offset = i + 1;  // set the next start

                }

                potentialCrLf = (value == 0x0D);  // checked when LF is matched
            }

            InputBufferCount =- offset;
            buffer_copy(&InputBuffer[0], &InputBuffer[offset], InputBufferCount);  // move unused portion of buffer to the start
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

    ssd1306_writeLargeTextAt("USB OLED", 1, 5);
    ssd1306_writeTextAt("medo64.com", 3, 4);
    ssd1306_moveToNextRow();
}


bool processText(uint8_t* data, const uint8_t count, const bool useLargeFont) {
    bool ok = true;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t value = *data;
        if (value >= 32) {
            if (useLargeFont) {
                ok = ok && ssd1306_writeLargeCharacter(value);
            } else {
                ok = ok && ssd1306_writeCharacter(value);
            }
        } else {
            ok = false;
        }
        data++;
    }
    return ok;
}

bool processCommand(uint8_t* data, const uint8_t count) {
    switch (*data) {
        case '~': {  // defaults
            return false;
        }

        case '@': {  // select address
            if (count == 3) {
                uint8_t address = 0;
                if (!hexToNibble(*++data, &address)) { return false; }
                if (!hexToNibble(*++data, &address)) { return false; }
                settings_setOledI2CAddress(address);
                settings_save();
                initOled();
                return true;
            }
        }

        case '#': {  // set screen size
            if (count == 2) {
                switch(*++data) {
                    case 'A': settings_setDisplayHeight(64); break;
                    case 'B': settings_setDisplayHeight(32); break;
                    default: return false;
                }
                settings_save();
                initOled();
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

        case 'm': {
            return false;
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