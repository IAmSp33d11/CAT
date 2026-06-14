#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "physmem.h"
#include "virtmem.h"

// This code is full of misnomers. Referencing the PML4 as a PD. So just know when you read
// This code that its full of misnomers. I'm too lazy to change it lol.

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


uint64_t get_phys_addr_mask(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0x80000008));

    uint8_t phys_bits = eax & 0xFF;

    uint64_t mask = ((1ULL << phys_bits) - 1) & ~4095ULL;

    return mask;
}

uint64_t get_pml4_index(void* virtual_address) {
    return ((uint64_t)virtual_address >> 39) & 0x1FF;
}

void map_page(uint64_t* pml4_virt, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags, uint64_t hhdm_offset) {
    uint64_t addr_mask = get_phys_addr_mask();

    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    if (!(pml4_virt[pml4_idx] & 0x1)) { 
        uint64_t* new_table_virt = (uint64_t*)alloc_page();
        memset(new_table_virt, 0, 4096);
        
        uint64_t new_table_phys = (uint64_t)new_table_virt - hhdm_offset;
        pml4_virt[pml4_idx] = (new_table_phys & addr_mask) | 0x7; 
    }
    uint64_t* pdpt_virt = (uint64_t*)( (pml4_virt[pml4_idx] & addr_mask) + hhdm_offset );

    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    if (!(pdpt_virt[pdpt_idx] & 0x1)) {
        uint64_t* new_table_virt = (uint64_t*)alloc_page();
        memset(new_table_virt, 0, 4096);
        
        uint64_t new_table_phys = (uint64_t)new_table_virt - hhdm_offset;
        pdpt_virt[pdpt_idx] = (new_table_phys & addr_mask) | 0x7;
    }
    uint64_t* pd_virt = (uint64_t*)( (pdpt_virt[pdpt_idx] & addr_mask) + hhdm_offset );

    uint64_t pd_idx = (virt_addr >> 21) & 0x1FF;
    if (!(pd_virt[pd_idx] & 0x1)) {
        uint64_t* new_table_virt = (uint64_t*)alloc_page();
        memset(new_table_virt, 0, 4096);
        
        uint64_t new_table_phys = (uint64_t)new_table_virt - hhdm_offset;
        pd_virt[pd_idx] = (new_table_phys & addr_mask) | 0x7;
    }
    uint64_t* pt_virt = (uint64_t*)( (pd_virt[pd_idx] & addr_mask) + hhdm_offset );

    uint64_t pt_idx = (virt_addr >> 12) & 0x1FF;
    pt_virt[pt_idx] = (phys_addr & addr_mask) | flags;
}


// FUCK YOU INTEL!
// FUCK YOU AMD!
uint64_t* make_pd(uint64_t* kernel_virt_addr, uint64_t kernel_phys_addr, uint64_t kernel_size, uint64_t hhdm_offset, struct limine_memmap_response *memmap) {
    uint64_t* new_pml4 = alloc_page();
    memset(new_pml4, 0, 4096);

    uint64_t kernel_size_in_pages = (kernel_size + 4095) >> 12;
    uint64_t phys_addr = kernel_phys_addr;
    uint64_t virt_addr = (uint64_t) kernel_virt_addr;
    for (int i = 0; i < kernel_size_in_pages; i++) {
        map_page(new_pml4, virt_addr, phys_addr, 0x3, hhdm_offset);
        phys_addr += 4096;
        virt_addr += 4096;
    }

    uint64_t total_memory_limit = get_mem_size(memmap);
    for (uint64_t identity = 0; identity < total_memory_limit; identity += 4096) {
        map_page(new_pml4, hhdm_offset + identity, identity, 0x3, hhdm_offset);
    }

    return new_pml4;
}