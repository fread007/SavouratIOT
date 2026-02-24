#include "isr.h"
#include "isr-mmio.h"
#include "main.h"
#include "uart.h"
#include <stddef.h>
#include "console.h"

// Declaration des fonctions d'assembleur
extern void _irqs_setup();
extern void _irqs_enable();
extern void _irqs_disable();
extern void _wfi();

// Structure d'un handler d'IRQ
typedef struct {
    void (*callback)(uint32_t, void*);
    void* cookie;
} irq_handler_t;

// Liste de tous les handlers d'IRQ
static irq_handler_t irq_handlers[NIRQS];

// appel des fonctions d'assembleur pour configurer et gérer les interruptions
void irqs_setup() {
    _irqs_setup();
}

void irqs_enable() {
    _irqs_enable();
}

void irqs_disable() {
    _irqs_disable();
}

void wfi(void) {
    _wfi();
}

// active l'interruption spécifiée par irq et associe le callback et cookie
void irq_enable(uint32_t irq, void(*callback)(uint32_t, void*), void* cookie) {
    if (irq < NIRQS) {
        irq_handlers[irq].callback = callback;
        irq_handlers[irq].cookie = cookie;
        mmio_write32((void*)VIC_BASE_ADDR, VICINTENABLE, (1 << irq)); // active l'interruption dans le VIC
    }
}

// désactive l'interruption spécifiée par irq et efface le callback et cookie associés
void irq_disable(uint32_t irq) {
    if (irq < NIRQS) {
        irq_handlers[irq].callback = NULL;
        irq_handlers[irq].cookie = NULL;
        mmio_write32((void*)VIC_BASE_ADDR, VICINTCLEAR, (1 << irq)); // désactive l'interruption dans le VIC
    }
}

void irq_handler(void) {
    //recupere le status des interruptions en cours
    uint32_t status = mmio_read32((void*)VIC_BASE_ADDR, 0x00);

    //test si il s'agit d'une interuption UART0
    if (status & (1 << UART0_IRQ)) {
        mmio_write32(UART0, 0x44, (1 << 4)); //clear l'interruption dans l'uart0
        if (irq_handlers[UART0_IRQ].callback != NULL) { //test si elle a etais activé
            irq_handlers[UART0_IRQ].callback(0, irq_handlers[UART0_IRQ].cookie); //appel le handler associé à l'interruption uart0
        }
        
    }

    //test si il s'agit d'une interuption timer0
    if(status & (1 << TIMER0_IRQ)) {
        mmio_write32(TIMER0, 0x0C, 1); //clear l'interruption dans le timer0
        if (irq_handlers[TIMER0_IRQ].callback != NULL) { //test si elle a etais activé
            irq_handlers[TIMER0_IRQ].callback(0, irq_handlers[TIMER0_IRQ].cookie);   //appel le handler associé à l'interruption timer0
        }
    }

    //clear l'interruption dans le VIC
    mmio_write32((void*)VIC_BASE_ADDR, 0xF00, 0);
}