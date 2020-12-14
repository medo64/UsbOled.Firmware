#ifndef APP_H
#define	APP_H

// USB input commands ring buffer - max is maximum line length
#define INPUT_BUFFER_MAX  192
uint8_t InputBuffer[INPUT_BUFFER_MAX];
uint8_t InputBufferStart = 0;
uint8_t InputBufferEnd = 0;
uint8_t InputBufferCount = 0;


// USB read buffer
#define USB_READ_BUFFER_MAX  32
uint8_t UsbReadBuffer[USB_READ_BUFFER_MAX];

// USB write buffer
#define USB_WRITE_BUFFER_MAX  32
uint8_t UsbWriteBuffer[USB_WRITE_BUFFER_MAX];

#endif
