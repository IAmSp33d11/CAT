#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "physmem.h"
#include "virtmem.h"
#include "asm.h"
#include "acpi.h"

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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_file_request kernel_request = {
    .id = LIMINE_EXECUTABLE_FILE_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request addr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};



// I dunno why I need these.
__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;



__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

extern void setup_gdt(void);
extern void idt_init(void);
extern void enable_SSE(void);
// Hey future me! If you rename this function, change it in linker.lds in ENTRY() too!
void startup(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    enable_SSE();


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

    if (kernel_request.response == NULL || addr_request.response == NULL) {
        panic_but_msg(framebuffer, "FATAL ERROR: FAILED TO GET KERNEL SIZE AND ADDRESS!");
    }
    uint64_t kernel_size = kernel_request.response->executable_file->size;
    uint64_t* kernel_phys_addr = (uint64_t*) addr_request.response->physical_base; // Where in RAM we actually are
    uint64_t* kernel_virt_addr = (uint64_t*) addr_request.response->virtual_base; // Where the bootloader loaded us

    struct limine_memmap_response *memmap;
    if (memmap_request.response == NULL) {
        panic_but_msg(framebuffer, "FATAL ERROR: FAILED TO FETCH MEMMAP FROM LIMINE BOOTLOADER!");
    }
    memmap = memmap_request.response;

    if (mp_request.response == NULL) {
        panic_but_msg(framebuffer, "FATAL ERROR: LIMINE BOOTLOADER FAILED TO PROVIDE MULTIPROCESSOR RESPONSE!");
    }
    if (rsdp_request.response == NULL) {
        panic_but_msg(framebuffer, "FATAL ERROR: LIMINE BOOTLOADER FUCKING HATES US AND REFUSES TO PROVIDE RSDP RESPONSE!");
    }

    void* rsdp = rsdp_request.response->address;



    if (tsc_hz == 0) {
        panic_but_msg(framebuffer, "FATAL ERROR: FAILED TO SYNCHRONIZE TSC!");
    }




    print("Successfully booted into the kernel!\n");
    char buffer[128];
    dtoa(bytes_to_mib(get_usable_ram_size(memmap)), 2, buffer);
    print("We have ");
    print(buffer);
    print(" MiB of usable RAM!\n");


    dtoa(bytes_to_mib(get_ram_size(memmap)), 2, buffer);
    print("We have ");
    print(buffer);
    print(" MiB of total RAM!\n");


    uint64_t* bitmap = place_bitmap(memmap, hhdm);
    itoa_hex(((uint64_t) bitmap) - hhdm, buffer);
    print("The bitmap is located at : 0x");
    print(buffer);
    print("\n");


    setup_bitmap(memmap, bitmap, hhdm);

    print("Our bitmap is done!\n");

    dtoa(bytes_to_mib(get_free_ram_size(bitmap, memmap)), 2, buffer);
    print("We have ");
    print(buffer);
    print(" MiB of free RAM!\n");

    dtoa(bytes_to_mib(get_used_ram_size(bitmap, memmap)), 2, buffer);
    print("We are using ");
    print(buffer);
    print(" MiB of RAM!\n");

    print("Memory Allocation Test\n");
    dtoa(bytes_to_mib(get_used_ram_size(bitmap, memmap)), 7, buffer);
    print("Before: ");
    print(buffer);
    print("\n");

    void* temp[100];
    for (int i = 0; i < 100; i++) {
        temp[i] = alloc_page();
    }
    dtoa(bytes_to_mib(get_used_ram_size(bitmap, memmap)), 7, buffer);
    print("During: ");
    print(buffer);
    print("\n");
    for (int i = 0; i < 100; i++) {
        free_page(temp[i]);
    }
    dtoa(bytes_to_mib(get_used_ram_size(bitmap, memmap)), 7, buffer);
    print("After: ");
    print(buffer);
    print("\n");



    print("The CR3 register is pointing to: 0x");
    itoa_hex(get_cr3(), buffer);
    print(buffer);
    print("\n");

    uint64_t* new_pd = clone_pd(hhdm);
    print("Cloned the page directory!\nIt is located at: 0x");
    itoa_hex((uint64_t) new_pd - hhdm, buffer);
    print(buffer);
    print("\n");

   


    switch_pd(new_pd, hhdm);
    print("Successfully switched page directory to a clone!\n");
    
    print("The CR3 register is pointing to: 0x");
    itoa_hex(get_cr3(), buffer);
    print(buffer);
    print("\n");

    itoa_hex((uint64_t) kernel_phys_addr, buffer);
    print("The kernel is loaded at : 0x");
    print(buffer);
    print("\n");

    itoa(kernel_size, buffer);
    print("And is: ");
    print(buffer);
    print(" bytes large!\n");


    new_pd = make_pd(kernel_virt_addr, (uint64_t) kernel_phys_addr, kernel_size, hhdm, memmap);
    print("We have a new Page Directory at: 0x");
    itoa_hex((uint64_t) new_pd - hhdm, buffer);
    print(buffer);
    print("\n");

    print("Attempting to switch to new page directory\n");
    switch_pd(new_pd, hhdm);
    print("We somehow didn't die!\n");

    print("The CR3 register is pointing to: 0x");
    itoa_hex(get_cr3(), buffer);
    print(buffer);
    print("\n");
    
    validate_rsdp(rsdp);
    print("The RSDP is valid!\n");

    void* madt = find_MADT(rsdp, hhdm);
    print("We found da MADT!\n");







    print("We are done!\n");
    // We're done, just hang...
    hcf();
}
