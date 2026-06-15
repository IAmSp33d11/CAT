#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "apic.h"
#include "asm.h"

// Stupid PIT shit
#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND   0x43

// Attempt to setup the APIC timer
uint32_t calibrate_lapic_timer(void) {
    wrmsr(0x832, 0x10000);
    wrmsr(0x83E, 0x03);

    uint16_t pit_count = 11932;

    outb(PIT_COMMAND, 0x30);
    outb(PIT_CHANNEL_0, (uint8_t)pit_count);
    outb(PIT_CHANNEL_0, (uint8_t)(pit_count >> 8));
    

    wrmsr(0x838, 0xFFFFFFFF);
    while (1) {
        outb(PIT_COMMAND, 0xE2);
        if ((inb(PIT_CHANNEL_0) & 0x80) != 0) {
            break; // Okay the PIT is done with its 10ms!
        }
    }
    uint32_t current_ticks = (uint32_t)rdmsr(0x839);
    uint32_t elapsed_ticks = 0xFFFFFFFF - current_ticks;

    wrmsr(0x838, 0);

    return elapsed_ticks;
}