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

#define LED_TIMEOUT       20
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
                if (InputBufferCorrupted && ((value == 0x0A) || (value == 0x0D))) {
                    InputBufferCount = 0;  // clear the whole buffer
                    InputBufferCorrupted = false;
                } else if (InputBufferCount < INPUT_BUFFER_MAX) {
                    InputBuffer[InputBufferCount] = value;
                    InputBufferCount++;
                } else {
                    InputBufferCorrupted = true;  // no more buffer; darn it
                    LedTimeout = LED_TIMEOUT_NONE;  // turn on LED permanently
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


#if defined(USB_INTERRUPT)
void __interrupt() SYS_InterruptHigh(void) {
    USBDeviceTasks();
}
#endif


void initOled(void) {
    uint8_t baudRateCounter;
    switch (settings_getI2CSpeedIndex()) {
        case 2: baudRateCounter = 59; break;    // 200 kHz @ 48 MHz
        case 3: baudRateCounter = 39; break;    // 300 kHz @ 48 MHz
        case 4: baudRateCounter = 29; break;    // 400 kHz @ 48 MHz
        case 5: baudRateCounter = 23; break;    // 500 kHz @ 48 MHz
        case 6: baudRateCounter = 19; break;    // 600 kHz @ 48 MHz
        case 7: baudRateCounter = 16; break;    // 700 kHz @ 48 MHz (~705 kHz)
        case 8: baudRateCounter = 14; break;    // 800 kHz @ 48 MHz
        case 9: baudRateCounter = 12; break;    // 900 kHz @ 48 MHz (~923 kHz)
        case 10: baudRateCounter = 11; break;   // 1000 kHz @ 48 MHz
        default: baudRateCounter = 119; break;  // 100 kHz @ 48 MHz
    }

    ssd1306_init(settings_getI2CAddress(), baudRateCounter, 128, settings_getDisplayHeight());
    ssd1306_setContrast(settings_getDisplayBrightness());
    if (settings_getDisplayInverse()) {
        ssd1306_displayInvert();
    } else {
        ssd1306_displayNormal();
    }
    ssd1306_displayFlip(settings_getDisplayFlip());
    ssd1306_clearAll();

    ssd1306_writeText16("    USB OLED    ");
    ssd1306_moveToNextRow16();
    ssd1306_writeText("   medo64.com   ");
    ssd1306_moveToNextRow();
}


bool processInput(const uint8_t* dataIn, const uint8_t count, bool* out_LastUseLarge) {
    if (count == 0) {  // if line is empty, process it more
        ssd1306_moveToNextRow();
        if (*out_LastUseLarge) {  // extra move for large font
            ssd1306_moveToNextRow();
            *out_LastUseLarge = false;
        }
        return true;
    }

    uint8_t* data = (uint8_t*)dataIn;
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

            case 0x09: {  // HT: command mode
                uint8_t* cmdData = data + 1;
                uint8_t cmdIndex = count;
                for (uint8_t j = i + 1; j < count; j++) {
                    if (*cmdData == 0) {
                        cmdIndex = j;
                        break;
                    }
                    cmdData++;
                }
                uint8_t cmdCount = cmdIndex - i - 1;
                if (cmdCount > 0) {
                    wasOk &= processCommand(++data, cmdCount);
                }
                data = cmdData;
                i = cmdIndex;
            } break;

            case 0x0B:  // VT: double-size font
                useLarge = !useLarge;
                break;

            case 0x0C:  // FF: clear remaining
                if (useLarge) {
                    ssd1306_clearRemaining16();
                } else {
                    ssd1306_clearRemaining();
                }
                break;

            default:
                if ((*data >= 32) && (*data <= 126)) {  // ignore ASCII control characters
                    if (useLarge) {
                        wasOk &= ssd1306_writeCharacter16(*data);
                    } else {
                        wasOk &= ssd1306_writeCharacter(*data);
                    }
                }
                break;
        }

        data++;
    }

    *out_LastUseLarge = useLarge;
    return wasOk;
}

bool processCommand(const uint8_t* data, const uint8_t count) {
    switch (*data) {

        case '#':  // screen size
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
            break;

        case '$':  // inverse
            if (count == 1) {  // get if display is inverted by default
                if (settings_getDisplayInverse()) {
                    OutputBufferAppend('I');
                } else {
                    OutputBufferAppend('N');
                }
                return true;
            } else if (count == 2) {  // set if display is inverted
                switch(*++data) {
                    case 'I': settings_setDisplayInverse(true); break;
                    case 'N': settings_setDisplayInverse(false); break;
                    default: return false;
                }
                settings_save();
                initOled();
                return true;
            }
            break;

        case '=':  // flip
            if (count == 1) {  // get if display is flipped by default
                if (settings_getDisplayFlip()) {
                    OutputBufferAppend('F');
                } else {
                    OutputBufferAppend('N');
                }
                return true;
            } else if (count == 2) {  // set if display is flipped
                switch(*++data) {
                    case 'F': settings_setDisplayFlip(true); break;
                    case 'N': settings_setDisplayFlip(false); break;
                    default: return false;
                }
                settings_save();
                initOled();
                return true;
            }
            break;

        case '%':  // reset
            if (count == 1) {  // reboot
                reset();
                return true;
            }
            break;

        case '*':  // brightness
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
            break;

        case '@':  // I2C address
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
            break;

        case '^':  // I2C speed
            if (count == 1) {  // get I2C speed index
                uint8_t speedIndex = settings_getI2CSpeedIndex();
                if (speedIndex == 10) {
                    OutputBufferAppend('0');
                } else {
                    OutputBufferAppend('0' + speedIndex);
                }
                return true;
            } else if (count == 2) {  // set I2C speed index
                uint8_t speedIndex = *++data;
                if (speedIndex == '0') {
                    settings_setI2CSpeedIndex(10);
                } else if ((speedIndex > '0') && (speedIndex <= '9')) {
                    settings_setI2CSpeedIndex(speedIndex - '0');
                } else {
                    return false;
                }
                settings_save();
                initOled();
                return true;
            }
            break;

        case '`':  // set serial number for USB
            if (count == 9) {
                uint8_t* serial = &Settings.UsbSerialValue[8];
                for (uint8_t i = 0; i < 8; i++) {
                    *serial = *++data;
                    serial += 2;
                }
                settings_save();
                reset();
            }
            break;

        case '~':  // defaults
            if (count == 1) {
                settings_setI2CAddress(SETTING_DEFAULT_I2C_ADDRESS);
                settings_setI2CSpeedIndex(SETTING_DEFAULT_I2C_SPEED_INDEX);
                settings_setDisplayHeight(SETTING_DEFAULT_DISPLAY_HEIGHT);
                settings_setDisplayBrightness(SETTING_DEFAULT_DISPLAY_BRIGHTNESS);
                settings_setDisplayInverse(SETTING_DEFAULT_DISPLAY_INVERSE);
                settings_save();
                return true;
            }
            break;

        case 'c':
        case 'C':
            if ((count == 17) || (count == 33)) {
                uint8_t dataCount = (count - 1) >> 1;
                uint8_t customCharData[16];
                for (uint8_t i = 0; i < dataCount; i++) {
                    if (!hexToNibble(*++data, &customCharData[i])) { return false; }
                    if (!hexToNibble(*++data, &customCharData[i])) { return false; }
                }
                return ssd1306_drawCharacter(&customCharData[0], dataCount);
            }
            break;

        case 'i':
            if (count == 1) {
                ssd1306_displayInvert();
                return true;
            }
            break;

        case 'I':
            if (count == 1) {
                ssd1306_displayNormal();
                return true;
            }
            break;

        case 'm':
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
            break;

        case 'V':  // Version
            if (count == 1) {  // get version
                OutputBufferAppend(0x30 + VERSION_MAJOR);
                OutputBufferAppend('.');
                OutputBufferAppend(0x30 + VERSION_MINOR);
                return true;
            }
            break;

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