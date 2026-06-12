#ifndef PHYSMEM_H
#define PHYSMEM_H

uint64_t get_usable_ram_size(struct limine_memmap_response *memmap);
uint64_t get_ram_size(struct limine_memmap_response *memmap);
uint64_t* place_bitmap(struct limine_memmap_response *memmap, uint64_t hhdm_offset);
void setup_bitmap(struct limine_memmap_response *memmap, uint64_t* bitmap, uint64_t hhdm_offset);

#endif