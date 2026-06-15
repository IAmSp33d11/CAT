#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "physmem.h"

struct __attribute__((packed)) TSS_entry {
    uint32_t reserved0;
    uint64_t rsp0; // Kernel Stack
    uint64_t rsp1; // Don't care
    uint64_t rsp2; // Don't care
    uint64_t reserved1;
    uint64_t ist1; // Interrupt Stack Tables
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7; 
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base; 
};


uint64_t gdt_address[7];
uint8_t* gdt_pointer = (uint8_t*) gdt_address;

// Taken from my old OS
struct GDT_entry {
    uint64_t base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
};

struct TSS_entry tss __attribute__((aligned(16))) = {0};


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

void encodeTssEntry(uint8_t *target, uint64_t tss_address)
{
    uint32_t limit = 104 - 1;
    
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] = (limit >> 16) & 0x0F;
    
    target[2] = tss_address & 0xFF;
    target[3] = (tss_address >> 8) & 0xFF;
    target[4] = (tss_address >> 16) & 0xFF;
    target[7] = (tss_address >> 24) & 0xFF;
    
    target[5] = 0x89;
    target[6] |= (0x0 << 4);
    
    target[8] = (tss_address >> 32) & 0xFF;
    target[9] = (tss_address >> 40) & 0xFF;
    target[10] = (tss_address >> 48) & 0xFF;
    target[11] = (tss_address >> 56) & 0xFF;
    
    target[12] = 0;
    target[13] = 0;
    target[14] = 0;
    target[15] = 0;
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

    // User Mode Code Segment
    entry.base = 0;
    entry.limit = 0xFFFFF;
    entry.access_byte = 0xFA;
    entry.flags = 0xA;
    encodeGdtEntry(gdt_pointer + 0x18, entry);

    // User Mode Data Segment
    entry.base = 0;
    entry.limit = 0xFFFFF;
    entry.access_byte = 0xF2;
    entry.flags = 0xC;
    encodeGdtEntry(gdt_pointer + 0x20, entry);

    // Stupid TSS

    memset(&tss, 0, sizeof(struct TSS_entry));

    uint64_t stack = (uint64_t) alloc_page();
    memset((void*)stack, 0, 4096);

    tss.rsp0 = stack + 4096;

    tss.iomap_base = sizeof(struct TSS_entry);

    encodeTssEntry(gdt_pointer + 0x28, (uint64_t)&tss);
    
    setGdt(0x38, (uint64_t) gdt_pointer);

    __asm__ volatile("ltr %%ax" :: "a"(0x28));
}