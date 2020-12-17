#include <stdint.h>
#include "Microchip/usb.h"
#include "Microchip/usb_device.h"
#include "Microchip/usb_device_cdc.h"
#include "app.h"
#include "system.h"
#include "led.h"
#include "ssd1306.h"

#define LED_TIMEOUT       2000
#define LED_TIMEOUT_NONE  65535
uint16_t LedTimeout = LED_TIMEOUT_NONE;

void main(void) {
    init();
    led_init();

    led_activity_on();
    for (uint8_t i = 0; i < 3; i++) {
        wait_short();
        led_activity_off();
        wait_short();
        led_activity_on();
    }

    ssd1306_init(0x3C, 128, 64);
    ssd1306_clearAll();

    ssd1306_writeLargeText("USB OLED");
    ssd1306_writeTextAt("medo64.com", 2, 0);

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
        uint8_t usbCount = getsUSBUSART(UsbReadBuffer, USB_READ_BUFFER_MAX); //until the buffer is free.
        if (usbCount > 0) {
            led_activity_on();
            LedTimeout = LED_TIMEOUT;

            for (uint8_t i = 0; i < usbCount; i++) {  // copy to buffer
                InputBuffer[InputBufferEnd] = UsbReadBuffer[i];
                if (InputBufferCount < INPUT_BUFFER_MAX) {
                    InputBufferEnd = (InputBufferEnd + 1) % INPUT_BUFFER_MAX;
                    InputBufferCount++;
                }
            }
        }

        // TODO

        // USB send reply
        if ((InputBufferCount > 0) && USBUSARTIsTxTrfReady()) {
            led_activity_on();
            LedTimeout = LED_TIMEOUT;

            uint8_t usbCount = 0;
            for (uint8_t i = 0; i < USB_WRITE_BUFFER_MAX; i++) {  // copy to output buffer
                if (InputBufferCount > 0) {
                    UsbWriteBuffer[i] = InputBuffer[InputBufferStart];
                    InputBufferStart = (InputBufferStart + 1) % INPUT_BUFFER_MAX;
                    InputBufferCount--;
                    usbCount++;
                } else {
                    break;
                }
            }

            putUSBUSART(&UsbWriteBuffer[0], usbCount);
        }
    }
}


void __interrupt() SYS_InterruptHigh(void) {
    #if defined(USB_INTERRUPT)
        USBDeviceTasks();
    #endif
}
