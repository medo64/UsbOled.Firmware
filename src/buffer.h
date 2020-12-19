#ifndef BUFFER_H
#define	BUFFER_H

#include "Microchip/usb_config.h"

// USB read buffer
#define USB_READ_BUFFER_MAX  CDC_DATA_IN_EP_SIZE
uint8_t UsbReadBuffer[USB_READ_BUFFER_MAX];

// USB write buffer
#define USB_WRITE_BUFFER_MAX  CDC_DATA_OUT_EP_SIZE
uint8_t UsbWriteBuffer[USB_WRITE_BUFFER_MAX];


// Input buffer - max is maximum line length
#define INPUT_BUFFER_MAX  192
uint8_t InputBuffer[INPUT_BUFFER_MAX];
uint8_t InputBufferCount = 0;
bool InputBufferCorrupted = false;

// Output buffer - max needs to be on a large size to prevent running out of it
#define OUTPUT_BUFFER_MAX 224
#define OUTPUT_BUFFER_HIGH 192
uint8_t OutputBuffer[OUTPUT_BUFFER_MAX];
uint8_t OutputBufferCount = 0;

#define OutputBufferAppend(X)  if (OutputBufferCount < OUTPUT_BUFFER_MAX) { OutputBuffer[OutputBufferCount] = (X); OutputBufferCount++; }


void buffer_copy(uint8_t* destination, uint8_t* source, const uint8_t count);

#endif	/* BUFFER_H */

