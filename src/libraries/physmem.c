#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "physmem.h"


uint64_t get_usable_ram_size(struct limine_memmap_response *memmap) {
    uint64_t usable_ram = 0;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        if (memmap->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            usable_ram += memmap->entries[i]->length;
        }
    }
    return usable_ram;
}


uint64_t get_ram_size(struct limine_memmap_response *memmap) {
    uint64_t highest_base = memmap->entries[0]->base;
    uint64_t length_with_highest_base_or_some_shit_lol = memmap->entries[0]->length;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        if (memmap->entries[i]->base > highest_base) {
            highest_base = memmap->entries[i]->base;
            length_with_highest_base_or_some_shit_lol = memmap->entries[i]->length;
        }
    }
    return highest_base + length_with_highest_base_or_some_shit_lol;
}
