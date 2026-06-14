#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "acpi.h"


void validate_rsdp(void* rsdp_pointer) {
    struct XSDP_t* rsdp = (struct XSDP_t*) rsdp_pointer;

    uint8_t* rsdp_bytes = (uint8_t*) rsdp_pointer;
    uint8_t temp = 0;
    for (size_t i = 0; i < 20; i++) {
        temp += rsdp_bytes[i];
    }
    if (temp != 0) {
        kernel_panic("KERNEL PANIC: THE RSDP CHECKSUM IS INVALID! YOUR KAPUTER IS BORKED! PLEASE FIX IT BEFORE WE CAN RUN ON DIS KAPUTER!");
    }

    if (rsdp->Revision == 0) {
        kernel_panic("KERNEL PANIC: THIS OPERATING SYSTEM DOES NOT YET SUPPORT YOUR HARDWARE!");
    }

    if (rsdp->Revision == 2) {
        for (size_t i = 0; i < 16; i++) {
            temp += rsdp_bytes[i + 20];
        }
        if (temp != 0) {
            kernel_panic("KERNEL PANIC: THE RSDP CHECKSUM IS INVALID! YOUR KAPUTER IS BORKED! PLEASE FIX IT BEFORE WE CAN RUN ON DIS KAPUTER!");
        }
    }

    // Okay we chill :3
}


void* find_MADT(void* rsdp_pointer, uint64_t hhdm_offset) {
    struct XSDP_t* rsdp = (struct XSDP_t*) rsdp_pointer;
    struct XSDT *xsdt;
    if (rsdp->Revision == 2) {
        xsdt = (struct XSDT *) (rsdp->XsdtAddress + hhdm_offset);
    }
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
        
    for (int i = 0; i < entries; i++)
    {
        struct ACPISDTHeader* h = (struct ACPISDTHeader*)((uintptr_t)xsdt->PointerToOtherSDT[i] + hhdm_offset); // I stole it from osdev.wiki lol
        if (!memcmp(h->Signature, "APIC", 4))
            return (void *) h;
    }

    kernel_panic("KERNEL PANIC: WE CANNOT FIND DA MADT! YOUR KAPUTER IS BORKED AGAIN!");
    return NULL;
}