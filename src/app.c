#include <stdint.h>
#include "Microchip/usb.h"
#include "Microchip/usb_device.h"
#include "Microchip/usb_device_cdc.h"
#include "app.h"
#include "font.h"
#include "hardware.h"
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

    while(true) {
        led_activity_toggle();
        ssd1306_init(0x3C);
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
        wait_short();
    }

    ready();

    
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

        if (USBGetDeviceState() < CONFIGURED_STATE) { continue; }
        if (USBIsDeviceSuspended() == true) { continue; }


        // USB receive
        uint8_t usbCount = getsUSBUSART(UsbReadBuffer, USB_READ_BUFFER_MAX); //until the buffer is free.
        if (usbCount > 0) {
            led_activity_on();
            LedTimeout = LED_TIMEOUT;

            //copy buffer
            for (int i=0; i<usbCount; i++) {
                InputBuffer[InputBufferEnd] = UsbReadBuffer[i];
                if (InputBufferCount < INPUT_BUFFER_MAX) {
                    InputBufferEnd = (InputBufferEnd + 1) % INPUT_BUFFER_MAX;
                    InputBufferCount++;
                }
            }
        }

        //TODO
        
        // USB send reply
        if(InputBufferCount > 0) {
            led_activity_on();
            LedTimeout = LED_TIMEOUT;

            uint8_t usbCount = 0;
            for (int i=0; i<USB_WRITE_BUFFER_MAX; i++) {
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

        CDCTxService();
    }
}


void interrupt SYS_Interrupt(void) {
    USBDeviceTasks();
}
