#include <stdint.h>
#include "main.h"

#define MAX_CHARS 512

typedef struct {
    volatile uint32_t tail;
    volatile uint8_t buffer[MAX_CHARS];
    volatile uint32_t head;
} ring_t;

bool_t ring_empty(ring_t *ring);
bool_t ring_full(ring_t *ring);
void ring_put(ring_t *ring, uint8_t bits);
uint8_t ring_get(ring_t *ring);
void ring_put_int(ring_t *r, int n);