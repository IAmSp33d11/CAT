#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "physmem.h"
#include "virtmem.h"

uint64_t get_cr3(void) {
    uint64_t temp;
    __asm__ volatile(
        "mov %%cr3, %0;"
        : "=r" (temp)
        :
        :
    );
    return temp;
}

void set_cr3(uint64_t* val) {
    __asm__ volatile(
        "mov %0, %%cr3"
        :
        : "r" (val)
        :
    );
}

uint64_t* clone_pd(uint64_t hhdm_offset) {
    uint64_t* new_pd = alloc_page(); // It takes 4096 bytes for one page table, also known as 1 page.
    uint64_t* old_pd = (uint64_t*) (get_cr3() + hhdm_offset);
    memcpy(new_pd, old_pd, 4096); // memcpy takes number of bytes it wants
    return new_pd;
}

void switch_pd(uint64_t* new_pd, uint64_t hhdm_offset) {
    uint64_t* phys_val = (uint64_t*) (((uint64_t) new_pd) - hhdm_offset);
    set_cr3(phys_val);
}