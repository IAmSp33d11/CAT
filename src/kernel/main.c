#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.


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

static volatile struct limine_tsc_frequency_request tsc_request = {
    .id = LIMINE_TSC_FREQUENCY_REQUEST_ID,
    .revision = 0
};


// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;



__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
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

    // Make sure we got our TSC Frequency
    if (tsc_request.response == NULL) {
        panic(framebuffer);
    }
    uint64_t tsc_hz = tsc_request.response->frequency;

    uint64_t ticks_per_frame = tsc_hz / 30;


    font_buffer = NULL;
    if (module_request.response != NULL) {
        for (uint64_t i = 0; i < module_request.response->module_count; i++) {
            struct limine_file *file = module_request.response->modules[i];
            
            if (file->string != NULL && memcmp(file->string, "FONT", 4) == 0) {
                font_buffer = (uint8_t *)file->address;
                break;
            }
        }
    }

    if (font_buffer == NULL) {
        panic(framebuffer);
    }

    clear_screen(framebuffer);
    draw_trans_flag(framebuffer);
    // We're done, just hang...
    hcf();
}
