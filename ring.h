#include <stdint.h>
#include "main.h"

#define MAX_CHARS 512

bool_t ring_empty();
bool_t ring_full();
void ring_put(uint8_t bits);
uint8_t ring_get();