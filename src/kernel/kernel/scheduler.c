#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "helpful.h"
#include "physmem.h"
#include "scheduler.h"

extern struct TSS_entry tss;


static uint64_t pid_counter; // This is almost certain to never wrap around
// If it wraps around you've got bigger fish to fry.


static struct prgm_page *first_page;
static struct prgm_page *last_page;

static struct prgm_page* current_program_page;
static uint8_t current_program_offset;



void init_prgm_handler() {
    pid_counter = 0;
    first_page = alloc_page();
    current_program_page = first_page;
    last_page = first_page;
    memset(first_page, 0, 4096);
    first_page->prev = first_page;
    first_page->next = first_page;
    first_page->task_allocation_mask = TASKS_PAGE_BITMAP;
}



struct prgm_page* add_prgm_page() {
    struct prgm_page* temp_page = alloc_page();
    memset(temp_page, 0, 4096);
    current_program_page = temp_page;
    current_program_offset = 0;
    last_page->next = temp_page;
    first_page->prev = temp_page;
    temp_page->next = first_page;
    temp_page->prev = last_page;
    last_page = temp_page;
    temp_page->task_allocation_mask = TASKS_PAGE_BITMAP;
    return temp_page;
}


struct prgm* add_prgm(uint64_t cr3, uint64_t rip, uint64_t rsp) {
    struct prgm_page* temp_page = first_page;
    uint32_t i = 0;
    size_t j = 0;
    if (temp_page == NULL) {
        return NULL; // We gotta run init_prgm_handler first!
    }
    while (true) {
        if (temp_page == first_page && ++i == 0) {
            // Fuck
            temp_page = add_prgm_page();
        } else if (temp_page->task_allocation_mask == -1) {
            temp_page = temp_page->next;
            continue;
        } else if (temp_page->task_allocation_mask == 0) { // Something broke, we can't trust it.
            // OUR LIFE IS A LIE! RUN FOR YOUR LIVES!
            kernel_panic("KERNEL PANIC: MEMORY CORRUPTION WHILE TRYING TO CREATE PROGRAM!");
        } else {
            for (; j < TASKS_PER_PAGE; j++) {
                if (((temp_page->task_allocation_mask >> j) & 1) == 0 && (temp_page->slots[j].status == STATUS_UNKNOWN)) {
                    break;
                }
            }
            break;
        }
    }
    struct prgm* temp_prgm = &temp_page->slots[j];
    memset(temp_prgm, 0, sizeof(struct prgm));
    temp_prgm->registers.cr3 = cr3;
    temp_prgm->registers.rip = rip;
    temp_prgm->registers.rsp = rsp;
    temp_prgm->status = STATUS_ALIVE;
    temp_prgm->pid = pid_counter++;
    return temp_prgm;
}



struct prgm* schedule() {
    static uint8_t times_ran = 0;
    if (times_ran++ == 0) {
        current_program_page = first_page;
        current_program_offset = 0;
        return &current_program_page->slots[current_program_offset];
    }
    
    if (++current_program_offset >= TASKS_PER_PAGE) {
        current_program_page++;
        current_program_offset = 0;
    }
    if (pid_counter == -1) {
        kernel_panic("KERNEL PANIC: We ran outta unique Program ID's!\nHow did you manage to do this?!");
    }

    while (current_program_page->slots[current_program_offset].status == STATUS_UNKNOWN 
    || current_program_page->slots[current_program_offset].status == STATUS_DED) {
        current_program_offset = (current_program_offset + 1) % TASKS_PER_PAGE;
        if (((current_program_offset + 1) % TASKS_PER_PAGE) == 0) {
            current_program_page = current_program_page->next;
        }
    }
    
    return &current_program_page->slots[current_program_offset];
}

void run_prgm(struct prgm* next_prgm) {
    
}