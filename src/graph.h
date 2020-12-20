#ifndef GRAPH_H
#define	GRAPH_H

#include <stdbool.h>
#include <stdint.h>

/** Pushes a new value. */
void graph_push(const uint8_t value);

/** Draws graph data starting at the current location. */
bool graph_draw(uint8_t width, bool isLarge);

#endif	/* GRAPH_H */
