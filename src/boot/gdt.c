#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"

uint64_t gdt_address[5];
uint8_t* gdt_pointer = (uint8_t*) gdt_address;

// Taken from my old OS
struct GDT_entry {
    uint64_t base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
};

// Taken from https://osdev.wiki/wiki/GDT_Tutorial
void encodeGdtEntry(uint8_t *target, struct GDT_entry source)
{
    // Check the limit to make sure that it can be encoded
    if (source.limit > 0xFFFFF) {return; }
    
    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;
    
    // Encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;
    
    // Encode the access byte
    target[5] = source.access_byte;
    
    // Encode the flags
    target[6] |= (source.flags << 4);
}

extern void setGdt(uint16_t limit, uint64_t base);
void setup_gdt() {
    struct GDT_entry entry;

    // Null descriptor
    entry.base = 0;
    entry.limit = 0;
    entry.access_byte = 0;
    entry.flags = 0;
    encodeGdtEntry(gdt_pointer, entry);

    // Kernel Mode Code Segment
    entry.base = 0;
    entry.limit = 0xFFFFF;
    entry.access_byte = 0x9A;
    entry.flags = 0xA;
    encodeGdtEntry(gdt_pointer + 0x08, entry);

    // Kernel Mode Data Segment
    entry.base = 0;
    entry.limit = 0xFFFFF;
    entry.access_byte = 0x92;
    entry.flags = 0xC;
    encodeGdtEntry(gdt_pointer + 0x10, entry);
    
    setGdt(0x18, (uint64_t) gdt_pointer);
}
