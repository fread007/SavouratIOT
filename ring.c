#include "ring.h"



bool_t ring_empty(ring_t *ring) {
    return (ring->head==ring->tail);
}

bool_t ring_full(ring_t *ring) {
    int next = (ring->head + 1) % MAX_CHARS;
    return (next==ring->tail);
}

void ring_put(ring_t *ring, uint8_t bits) {
    //kprintf("put %c in ring\n", bits);
    uint32_t next = (ring->head + 1) % MAX_CHARS;
    ring->buffer[ring->head] = bits;
    ring->head = next;
}

uint8_t ring_get(ring_t *ring) {
    uint8_t bits;
    uint32_t next = (ring->tail + 1) % MAX_CHARS;
    bits = ring->buffer[ring->tail];
    ring->tail = next;
    return bits;
}
void ring_put_int(ring_t *r, int n) {
    char buf[10];
    int i = 0;

    if (n == 0) {
        ring_put(r, '0');
        return;
    }

    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }

    while (i > 0) {
        ring_put(r, buf[--i]);
    }
}