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

// Returns it in page tables
uint64_t get_bitmap_size(struct limine_memmap_response *memmap) {
    uint64_t total_bytes = get_ram_size(memmap);
    uint64_t total_pages = total_bytes / 4096;
    uint64_t bitmap_bytes = (total_pages + 7) / 8;
    return (bitmap_bytes + 4095) / 4096;
}

uint64_t* place_bitmap(struct limine_memmap_response *memmap, uint64_t hhdm_offset) {
    uint64_t bitmap_size = get_bitmap_size(memmap) * 4096;
    int best_fit_idx = -1; // It must fit the best.
    uint64_t best_fit_len = -1;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        if (memmap->entries[i]->length >= bitmap_size && best_fit_len > memmap->entries[i]->length && memmap->entries[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t misalignment = memmap->entries[i]->base % 4096;
            uint64_t padding = 0;
            
            if (misalignment != 0) {
                padding = 4096 - misalignment;
            }

            if (memmap->entries[i]->length < padding || (memmap->entries[i]->length - padding) < bitmap_size) {
                continue;
            }

            best_fit_idx = i;
            best_fit_len = memmap->entries[i]->length;
        }
    }

    // How did this even happen???
    if (best_fit_len == -1) {
        kernel_panic("What? How did we not find viable memory for the bitmap?\nThis has to be a bug in our bootloader.");
    }

    uint64_t base = (memmap->entries[best_fit_idx]->base + 4096) & ~4095;
    uint8_t* bitmap_virtual_ptr = (uint8_t*)(base + hhdm_offset);

    for (size_t i = 0; i < bitmap_size; i++) {
        bitmap_virtual_ptr[i] = -1;
    }

    return (uint64_t*)bitmap_virtual_ptr;
}

void setup_bitmap(struct limine_memmap_response *memmap, uint64_t* bitmap, uint64_t hhdm_offset) {
    uint8_t* easier_bitmap = (uint8_t*) bitmap;

    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = entry->base / 4096;
            uint64_t total_pages = entry->length / 4096;
            uint64_t end_page = start_page + total_pages;

            for (uint64_t page = start_page; page < end_page; page++) {
                uint64_t byte_idx = page / 8;
                uint64_t bit_idx = page % 8;


                easier_bitmap[byte_idx] &= ~(1 << bit_idx);
            }
        }
    }
    // S.O.B.
    // SAVE OUR BITMAP
    uint64_t base = ((uint64_t)bitmap) - hhdm_offset;
    uint64_t bitmap_size = get_bitmap_size(memmap) * 4096;
    uint64_t bitmap_start_page = base / 4096;
    uint64_t bitmap_page_count = (bitmap_size + 4095) / 4096; 
    uint64_t bitmap_end_page = bitmap_start_page + bitmap_page_count;

    for (uint64_t page = bitmap_start_page; page < bitmap_end_page; page++) {
        uint64_t byte_idx = page / 8;
        uint64_t bit_idx = page % 8;
        easier_bitmap[byte_idx] |= (1 << bit_idx);
    }
}



// # Most useless comment lol
//
// Put on your war paint
// Cross walks and cross hearts hope-to-dies
// Silver clouds with grey linings
// One maniac at a time we will take it back
// You know time crawls on when your waiting for the song to start
// So dance along to the beat of your heart
// Hey, young blood!
// Doesn't it feel like our time is running out?
// I'm gonna change you
// Like a phoenix
// Wearing our vintage misery
// NOPE, I think it looked a little better on me
// I'm gonna change you
// Like a remix
// Then I'll raise you
// Like a phoenix!