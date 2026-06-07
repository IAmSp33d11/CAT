#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

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


// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;



__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = dest;
    const uint8_t *restrict psrc = src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = dest;
    const uint8_t *psrc = src;

    if ((uintptr_t)src > (uintptr_t)dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if ((uintptr_t)src < (uintptr_t)dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// Halt and catch fire function.
static void hcf(void) {
    __asm__ volatile("cli; hlt;");
}

void panic(struct limine_framebuffer *framebuffer) {
    volatile uint32_t *fb_ptr = framebuffer->address;
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            fb_ptr[y * (framebuffer->pitch / 4) + x] = 0x00FF0000;
        }
    }
    hcf();
}

// This is a very important function.
void draw_trans_flag(struct limine_framebuffer *framebuffer) {
    volatile uint32_t *fb_ptr = framebuffer->address;
    size_t stripe_height = framebuffer->height / 5;
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            if (y < stripe_height) {
                fb_ptr[y * (framebuffer->pitch / 4) + x] = 0x005BCFFB;
            } else if (y < stripe_height * 2) {
                fb_ptr[y * (framebuffer->pitch / 4) + x] = 0x00F5ABB9;
            } else if (y < stripe_height * 3) {
                fb_ptr[y * (framebuffer->pitch / 4) + x] = 0x00FFFFFF;
            } else if (y < stripe_height * 4) {
                fb_ptr[y * (framebuffer->pitch / 4) + x] = 0x00F5ABB9;
            } else {
                fb_ptr[y * (framebuffer->pitch / 4) + x] = 0x005BCFFB;
            }
            
        }
    }
}

void draw_char(struct limine_framebuffer *framebuffer, uint8_t *font_buffer, 
               char c, size_t start_x, size_t start_y, uint32_t text_color) {
    
    volatile uint32_t *fb_ptr = framebuffer->address;
    size_t pitch_pixels = framebuffer->pitch / 4;

    uint8_t *char_glyph = font_buffer + ((uint8_t)c * 16);

    for (size_t row = 0; row < 16; row++) {
        uint8_t bits = char_glyph[row];

        for (size_t col = 0; col < 8; col++) {
            // Check bit from left to right (MSB to LSB)
            if (bits & (0x80 >> col)) {
                size_t pixel_x = start_x + col;
                size_t pixel_y = start_y + row;

                // Screen boundary check
                if (pixel_x < framebuffer->width && pixel_y < framebuffer->height) {
                    fb_ptr[pixel_y * pitch_pixels + pixel_x] = text_color;
                }
            }
        }
    }
}



void draw_string(struct limine_framebuffer *framebuffer, uint8_t *font_buffer, 
                 const char *str, size_t x, size_t y, uint32_t text_color) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        draw_char(framebuffer, font_buffer, str[i], x + (i * 8), y, text_color);
    }

}
struct limine_framebuffer *framebuffer = NULL;
uint8_t *font_buffer = NULL;
uint32_t text_color = 0x00FFFFFF;


void print(const char *str) {
    static size_t cursor_x = 0;
    static size_t cursor_y = 0;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_x = 0;
            cursor_y += 16; // font height
            continue;
        }

        draw_char(framebuffer, font_buffer, str[i], cursor_x, cursor_y, text_color);
        cursor_x += 8; // font width
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
    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];
    if (font_buffer == NULL) {
        panic(framebuffer);
    }


    print("CAT is booting...\nTest #1\n");
    print("Test #2");


    // We're done, just hang...
    hcf();
}
