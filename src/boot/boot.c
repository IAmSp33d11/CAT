#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"


__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);


// We request stuff from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 1
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};


__attribute__((used, section(".limine_requests")))
static volatile struct limine_tsc_frequency_request tsc_request = {
    .id = LIMINE_TSC_FREQUENCY_REQUEST_ID,
    .revision = 0
};


__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST_ID,
    .revision = 0,
    .flags = 1
};


// I dunno why I need these.
__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;



__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

#define MAX_MEMMAP_SIZE     64 // I mean its almost certainly not gonna have over 64 entries.


extern void setup_gdt(void);
extern void idt_init(void);
// Hey future me! If you rename this function, change it in linker.lds in ENTRY() too!
void startup(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }


    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];

    // Make sure we got our Higher-Half Direct Map
    if (hhdm_request.response == NULL) {
        panic(framebuffer);
    }

    uint64_t hhdm = hhdm_request.response->offset;
    uint64_t tsc_hz;
    // Make sure we got our TSC Frequency
    if (tsc_request.response == NULL) {
        tsc_hz = calibrate_tsc();
    } else {
        tsc_hz = tsc_request.response->frequency;
    }

    font_buffer = NULL;
    if (module_request.response != NULL) {
        for (uint64_t i = 0; i < module_request.response->module_count; i++) {
            struct limine_file *file = module_request.response->modules[i];
            
            

            if (file->string != NULL && memcmp(file->string, "FONT", 4) == 0) {
                font_buffer = (uint8_t *)file->address;
                continue;
            }
        }
    } else {
        panic(framebuffer);
    }
    if (font_buffer == NULL) {
        panic(framebuffer);
    }

    uint64_t memmap_size = 0;
    struct limine_memmap_entry **memmap;
    if (memmap_request.response == NULL) {
        panic_but_msg(framebuffer, "FATAL ERROR: FAILED TO FETCH MEMMAP FROM LIMINE BOOTLOADER!");
    }
    memmap_size = memmap_request.response->entry_count;
    memmap = memmap_request.response->entries;

    if (mp_request.response == NULL) {
        panic_but_msg(framebuffer, "FATAL ERROR: LIMINE BOOTLOADER FAILED TO PROVIDE MULTIPROCESSOR RESPONSE!");
    }
    if (mp_request.response->flags & 1) {
        // x2APIC
    } else {
        // xAPIC
    }



    if (tsc_hz == 0) {
        panic_but_msg(framebuffer, "FATAL ERROR: FAILED TO SYNCHRONIZE TSC!");
    }
    print("Successfully booted into the kernel!\n");
    char buffer[128];
    itoa(memmap_size, buffer);
    print("We have ");
    print(buffer);
    print(" entries in the memory map!\n");
    print("The TSC's clock speed is ");
    itoa(tsc_hz, buffer);
    print(buffer);
    print("Hz!\n");

    setup_gdt();

    print("WE SETUP THE GDT!\n");

    idt_init();

    print("WE SETUP THE IDT!\n");

    __asm__ volatile ("int $0x00"); 
    

    // We're done, just hang...
    hcf();
}
