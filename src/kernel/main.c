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


#define VIDEO_WIDTH 160
#define VIDEO_HEIGHT 120
#define BYTES_PER_FRAME ((VIDEO_WIDTH * VIDEO_HEIGHT) / 8) // 2400 bytes

// Play a black and white video, for example, bad apple.
void play_bw_video(uint8_t *video_bin_ptr, uint32_t total_frames, uint64_t tsc_hz) {
    uint64_t ticks_per_frame = tsc_hz / 30;
    volatile uint32_t *fb_ptr = framebuffer->address;
    size_t pitch_pixels = framebuffer->pitch / 4;

    // Determine how many times we can scale 160x120 up to fit the screen height
    size_t scale = framebuffer->height / VIDEO_HEIGHT;
    if (scale < 1) scale = 1;

    size_t offset_x = (framebuffer->width - (VIDEO_WIDTH * scale)) / 2;
    size_t offset_y = (framebuffer->height - (VIDEO_HEIGHT * scale)) / 2;

    clear_screen();

    // Set up our initial baseline clock deadline
    uint64_t next_frame_deadline = rdtsc();

    for (uint32_t frame = 0; frame < total_frames; frame++) {
        // Point to the exact 2400-byte block for this frame
        uint8_t *frame_data = video_bin_ptr + (frame * BYTES_PER_FRAME);
        uint32_t byte_idx = 0;

        for (size_t y = 0; y < VIDEO_HEIGHT; y++) {
            for (size_t x = 0; x < VIDEO_WIDTH; x += 8) {
                // Fetch the packed 8 pixels
                uint8_t packed_byte = frame_data[byte_idx++];

                for (int bit = 0; bit < 8; bit++) {
                    // Check individual bits from Left to Right (MSB to LSB)
                    uint32_t color = (packed_byte & (0x80 >> bit)) ? 0x00FFFFFF : 0x00000000;
                    size_t pixel_x = x + bit;

                    // Draw a scaled block of pixels (nearest-neighbor scaling)
                    for (size_t sy = 0; sy < scale; sy++) {
                        for (size_t sx = 0; sx < scale; sx++) {
                            size_t scr_x = offset_x + (pixel_x * scale) + sx;
                            size_t scr_y = offset_y + (y * scale) + sy;

                            // Prevent out-of-bounds screen corruption
                            if (scr_x < framebuffer->width && scr_y < framebuffer->height) {
                                fb_ptr[scr_y * pitch_pixels + scr_x] = color;
                            }
                        }
                    }
                }
            }
        }

        next_frame_deadline += ticks_per_frame;

        // Wait until the timer says its been 1/30th of a second
        while (rdtsc() < next_frame_deadline) {
            __asm__ volatile("pause"); 
        }
    }
}

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
    uint64_t tsc_hz;
    // Make sure we got our TSC Frequency
    if (tsc_request.response == NULL) {
        tsc_hz = calibrate_tsc();
    } else {
        tsc_hz = tsc_request.response->frequency;
    }
    

    uint64_t ticks_per_frame = tsc_hz / 30;


    font_buffer = NULL;
    bad_apple = NULL;
    uint64_t bad_apple_size = 0;
    if (module_request.response != NULL) {
        for (uint64_t i = 0; i < module_request.response->module_count; i++) {
            struct limine_file *file = module_request.response->modules[i];
            
            if (file->string != NULL && memcmp(file->string, "FONT", 4) == 0) {
                font_buffer = (uint8_t *)file->address;
                continue;
            }
            if (file->string != NULL && memcmp(file->string, "bad_apple", 9) == 0) {
                bad_apple = (uint8_t *)file->address;
                bad_apple_size = file->size;
                continue;
            } 
        }
    } else {
        panic(framebuffer);
    }
    if (bad_apple == NULL) {
        clear_screen(framebuffer);
        print("WE CANNOT FIND THE BAD_APPLE.BIN FILE SO WE GO KABLOOY\n");
        hcf();
    }
    uint32_t bad_apple_frames = bad_apple_size / 2400;

    if (font_buffer == NULL) {
        panic(framebuffer);
    }

    clear_screen(framebuffer);


    print("Video binary located! Initializing playback loop...\n");
    
    // Give the user 1 second to read the text before drawing
    uint64_t startup_delay = rdtsc() + tsc_hz;
    while(rdtsc() < startup_delay);

    // 4. BLAST IT!
    play_bw_video(bad_apple, bad_apple_frames, tsc_hz);

    if (tsc_hz == 0) {
        clear_screen(framebuffer);
        print("FUCK");
    }
    // We're done, just hang...
    hcf();
}
