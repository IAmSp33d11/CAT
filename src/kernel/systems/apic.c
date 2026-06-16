#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "apic.h"
#include "asm.h"

// Stupid PIT shit
#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND   0x43


void init_apic(void* madt_apic_base) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 21)) {
        current_apic_mode = APIC_MODE_X2APIC;
        uint64_t apic_base_msr = rdmsr(0x1B);
        apic_base_msr |= (1 << 10) | (1 << 11);
        wrmsr(0x1B, apic_base_msr);
    } else if (edx & (1 << 9)) {
        current_apic_mode = APIC_MODE_XAPIC;
        xapic_base_vaddr = madt_apic_base;

        uint64_t apic_base_msr = rdmsr(0x1B);
        apic_base_msr |= (1 << 11);
        wrmsr(0x1B, apic_base_msr);
    } else {
        kernel_panic("KERNEL PANIC: HOW OLD IS UR KAPUTER! IT DOESN'T SUPPORT APIC! HOW ARE U RUNNING DIS?!");
    }
}

// Use these two to fix any stupid issues of QEMU not supporting x2APIC by default.
// Our system must be running on any and all x86_64 CPUs
void apic_write(uint32_t reg, uint64_t value) {
    if (current_apic_mode == APIC_MODE_X2APIC) {
        wrmsr(reg, value);
    } else {
        uint32_t offset = (reg - 0x800) << 4;
        volatile uint32_t* target = (volatile uint32_t*)((uintptr_t)xapic_base_vaddr + offset);
        *target = (uint32_t)value;
    }
}

uint64_t apic_read(uint32_t reg) {
    if (current_apic_mode == APIC_MODE_X2APIC) {
        return rdmsr(reg);
    } else {
        uint32_t offset = (reg - 0x800) << 4;
        volatile uint32_t* target = (volatile uint32_t*)((uintptr_t)xapic_base_vaddr + offset);
        return *target;
    }
}

// Attempt to setup the APIC timer
uint32_t calibrate_lapic_timer(void) {
    
    apic_write(0x832, 0x10000);
    apic_write(0x83E, 0x03);

    uint16_t pit_count = 11932;

    outb(PIT_COMMAND, 0x30);
    outb(PIT_CHANNEL_0, (uint8_t)pit_count);
    outb(PIT_CHANNEL_0, (uint8_t)(pit_count >> 8));
    

    apic_write(0x838, 0xFFFFFFFF);
    while (1) {
        outb(PIT_COMMAND, 0xE2);
        if ((inb(PIT_CHANNEL_0) & 0x80) != 0) {
            break; // Okay the PIT is done with its 10ms!
        }
    }
    uint32_t current_ticks = (uint32_t)apic_read(0x839);
    uint32_t elapsed_ticks = 0xFFFFFFFF - current_ticks;

    apic_write(0x838, 0);

    return elapsed_ticks;
}