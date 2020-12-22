#include <stdint.h>
#include "Microchip/usb.h"
#include "Microchip/usb_device.h"
#include "Microchip/usb_device_cdc.h"
#include "buffer.h"
#include "led.h"
#include "settings.h"
#include "ssd1306.h"
#include "system.h"

bool processInput(const uint8_t* data, const uint8_t count, bool* out_LastUseLarge);
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
    ssd1306_setContrast(settings_getDisplayBrightness());

    led_activity_off();

#if defined(USB_INTERRUPT)
    interruptsEnable();
#endif
    
    USBDeviceInit();
    USBDeviceAttach();

    led_activity_off();
    
    bool lastUseLarge = false;
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
            for (uint8_t i = 0; i < InputBufferCount; i++) {  // find EOLs
                uint8_t eolChar = InputBuffer[i];  // this'll be EOL eventually

                if ((eolChar == 0x0A) || (eolChar == 0x0D)) {  // start line processing on either CR or LF
                    if (!processInput(&InputBuffer[offset], i - offset, &lastUseLarge)) {
                        OutputBufferAppend('!');  // if there's any error, return exclamation point
                    }
                    OutputBufferAppend(eolChar);

                    offset = i + 1;  // set the next start
                }
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
    ssd1306_init(settings_getI2CAddress(), settings_getI2CSpeed(), 128, settings_getDisplayHeight());
    ssd1306_clearAll();

    ssd1306_writeText("    USB OLED    ", true);
    ssd1306_moveToNextRow();
    ssd1306_moveToNextRow();
    ssd1306_writeText("   medo64.com   ", false);
    ssd1306_moveToNextRow();
}


bool processInput(const uint8_t* data, const uint8_t count, bool* out_LastUseLarge) {
    if (count == 0) {  // if line is empty, process it more
        ssd1306_moveToNextRow();
        if (*out_LastUseLarge) {  // extra move for large font
            ssd1306_moveToNextRow();
            *out_LastUseLarge = false;
        }
        return true;
    }

    if (*data == 0x09) {  // HT: command mode
        return processCommand(++data, count - 1);
    } else {
        bool wasOk = true;
        bool useLarge = false;

        for (uint8_t i = 0; i < count; i++) {
            switch (*data) {
                case 0x07:  // BEL: clear screen
                    ssd1306_clearAll();
                    break;

                case 0x08:  // BS: move to origin
                    ssd1306_moveTo(1, 1);
                    break;

                case 0x0B:  // VT: double-size font
                    useLarge = !useLarge;
                    break;

                case 0x0C:  // FF: clear remaining
                    ssd1306_clearRemaining(useLarge);
                    break;

                default:
                    if ((*data >= 32) && (*data <= 126)) {  // ignore ASCII control characters
                        wasOk &= ssd1306_writeCharacter(*data, useLarge);
                    }
                    break;
            }

            data++;
        }

        *out_LastUseLarge = useLarge;
        return wasOk;
    }
}


bool processText(const uint8_t* data, const uint8_t count, const bool useLargeFont) {
    bool ok = true;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t value = *data;
        if (value >= 32) {
        } else {
            ok = false;
        }
        data++;
    }
    return ok;
}

bool processCommand(const uint8_t* data, const uint8_t count) {
    switch (*data) {

        case '#': {  // screen size
            if (count == 1) {  // get screen size
                if (settings_getDisplayHeight() == 32) {
                    OutputBufferAppend('B');
                } else {
                    OutputBufferAppend('A');
                }
                return true;
            } else if (count == 2) {  // set screen size
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

        case '%': {  // reset
            if (count == 1) {  // reboot
                reset();
                return true;
            }
        }

        case '*': {  // brightness
            if (count == 1) {  // get brightness
                uint8_t brightness = settings_getDisplayBrightness();
                OutputBufferAppend(nibbleToHex(brightness >> 4));  // high nibble
                OutputBufferAppend(nibbleToHex(brightness));  // low nibble
                return true;
            } else if (count == 3) {  // set brightness
                uint8_t brightness;
                if (!hexToNibble(*++data, &brightness)) { return false; }
                if (!hexToNibble(*++data, &brightness)) { return false; }
                settings_setDisplayBrightness(brightness);
                settings_save();
                ssd1306_setContrast(brightness);
                return true;
            }
        }

        case '@': {  // I2C address
            if (count == 1) {  // get I2C address
                uint8_t address = settings_getI2CAddress();
                OutputBufferAppend(nibbleToHex(address >> 4));  // high nibble
                OutputBufferAppend(nibbleToHex(address));  // low nibble
                return true;
            } else if (count == 3) {  // set I2C address
                uint8_t address;
                if (!hexToNibble(*++data, &address)) { return false; }
                if (!hexToNibble(*++data, &address)) { return false; }
                settings_setI2CAddress(address);
                settings_save();
                initOled();
                return true;
            }
        }

        case '^': {  // I2C speed
            if (count == 1) {  // get I2C speed
                uint16_t speed = settings_getI2CSpeed();
                OutputBufferAppend(nibbleToHex(speed >> 12));  // upper high nibble
                OutputBufferAppend(nibbleToHex(speed >> 8));   // upper low nibble
                OutputBufferAppend(nibbleToHex(speed >> 4));   // lower high nibble
                OutputBufferAppend(nibbleToHex(speed));        // lower low nibble
                return true;
            } else if ((count == 3) || (count == 5)) {  // set I2C speed
                uint16_t_VAL speed;
                if (count == 5) {
                    if (!hexToNibble(*++data, &speed.byte.HB)) { return false; }
                    if (!hexToNibble(*++data, &speed.byte.HB)) { return false; }
                } else {
                    speed.byte.HB = 0;
                }
                if (!hexToNibble(*++data, &speed.byte.LB)) { return false; }
                if (!hexToNibble(*++data, &speed.byte.LB)) { return false; }
                settings_setI2CSpeed(speed.Val);
                settings_save();
                initOled();
                return true;
            }
        }

        case '~': {  // defaults
            if (count == 1) {
                settings_setI2CAddress(SETTING_DEFAULT_I2C_ADDRESS);
                settings_setDisplayHeight(SETTING_DEFAULT_DISPLAY_HEIGHT);
                settings_setDisplayBrightness(SETTING_DEFAULT_DISPLAY_BRIGHTNESS);
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