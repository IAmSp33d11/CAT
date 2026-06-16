#ifndef APIC_H
#define APIC_H

typedef enum {
    APIC_MODE_XAPIC,
    APIC_MODE_X2APIC
} apic_mode_t;

static apic_mode_t current_apic_mode = APIC_MODE_XAPIC;
static void* xapic_base_vaddr = NULL;

void init_apic(void* madt_apic_base);
void apic_write(uint32_t reg, uint64_t value);
uint64_t apic_read(uint32_t reg);
uint32_t calibrate_lapic_timer(void);
#endif