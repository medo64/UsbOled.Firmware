#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"

void buffer_copy(uint8_t* destination, const uint8_t* source, const uint8_t count) {
    for (uint8_t i = 0; i < count ; i++) {
        *destination = *source;
        destination++;
        source++;
    }
}
