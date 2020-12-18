#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"

bool buffer_appendInput(uint8_t value) {
    if (InputBufferCount < INPUT_BUFFER_MAX) {
        InputBuffer[InputBufferCount] = value;
        InputBufferCount++;
        return true;
    } else {
        return false;
    }
}

bool buffer_appendOutput(uint8_t value) {
    if (OutputBufferCount < OUTPUT_BUFFER_MAX) {
        OutputBuffer[OutputBufferCount] = value;
        OutputBufferCount++;
        return true;
    } else {
        return false;
    }
}


void buffer_copy(uint8_t* destination, uint8_t* source, const uint8_t count) {
    for (uint8_t i = 0; i < count ; i++) {
        *destination = *source;
        destination++;
        source++;
    }
}
