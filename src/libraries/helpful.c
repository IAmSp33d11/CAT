#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"

struct limine_framebuffer *framebuffer = NULL;
uint8_t *font_buffer = NULL;
uint32_t text_color = 0x00FFFFFF;


void draw_pixel(size_t x, size_t y, uint32_t color) {
    volatile uint32_t *fb_ptr = framebuffer->address;
    fb_ptr[y * (framebuffer->pitch / 4) + x] = color;
}


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




// get the TSC if fucking limine won't give us it
uint64_t calibrate_tsc(void) {
    outb(0x43, 0x30);

    outb(0x40, 0xFF);
    outb(0x40, 0xFF);

    uint16_t start_pit = read_pit_count();
    uint16_t current_pit = start_pit;

    uint16_t pit_ticks_to_wait = 10000; 
    uint16_t target_pit = start_pit - pit_ticks_to_wait;

    uint64_t start_tsc = rdtsc();

    while (current_pit > target_pit) {
        current_pit = read_pit_count();
        
        // Handle a potential wrap-around just in case
        if (current_pit > start_pit) {
            // PIT wrapped around early, abort or restart calibration
            return 0; 
        }
    }

    uint64_t end_tsc = rdtsc();
    uint64_t tsc_elapsed = end_tsc - start_tsc;

    uint64_t tsc_hz = (tsc_elapsed * 1193182) / pit_ticks_to_wait;

    return tsc_hz;
}

// Halt and catch fire function.
void hcf(void) {
    __asm__ volatile("cli; hlt;");
}

void panic(struct limine_framebuffer *framebuffer) {
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            draw_pixel(x, y, 0x00FF0000);
        }
    }
    hcf();
}

void panic_but_msg(struct limine_framebuffer *framebuffer, const char* msg) {
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            draw_pixel(x, y, 0x00FF0000);
        }
    }
    print(msg);
    hcf();
}

// This is a very important function.
void draw_trans_flag(struct limine_framebuffer *framebuffer) {
    size_t stripe_height = framebuffer->height / 5;
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            if (y < stripe_height) {
                draw_pixel(x, y, 0x005BCFFB);
            } else if (y < stripe_height * 2) {
                draw_pixel(x, y, 0x00F5ABB9);
            } else if (y < stripe_height * 3) {
                draw_pixel(x, y, 0x00FFFFFF);
            } else if (y < stripe_height * 4) {
                draw_pixel(x, y, 0x00F5ABB9);
            } else {
                draw_pixel(x, y, 0x005BCFFB);
            }
            
        }
    }
}

void draw_char(struct limine_framebuffer *framebuffer, uint8_t *font_buffer, 
               char c, size_t start_x, size_t start_y, uint32_t text_color) {
    

    uint8_t *char_glyph = font_buffer + ((uint8_t)c * 16);

    for (size_t row = 0; row < 16; row++) {
        uint8_t bits = char_glyph[row];

        for (size_t col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                size_t pixel_x = start_x + col;
                size_t pixel_y = start_y + row;

                if (pixel_x < framebuffer->width && pixel_y < framebuffer->height) {
                    draw_pixel(pixel_x, pixel_y, text_color);
                }
            }
        }
    }
}

static size_t cursor_x = 0;
static size_t cursor_y = 0;

void clear_screen() {
    cursor_x = 0;
    cursor_y = 0;
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            draw_pixel(x, y, 0);
        }
    }
}


void print(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (cursor_x + 8 >= framebuffer->width) {
            cursor_x = 0;
            cursor_y += 16;
        }
        if (cursor_y + 16 >= framebuffer->height) {
            cursor_y = 0; 
        }
        if (str[i] == '\n') {
            cursor_x = 0;
            cursor_y += 16; // font height
            continue;
        }

        draw_char(framebuffer, font_buffer, str[i], cursor_x, cursor_y, text_color);
        cursor_x += 8; // font width
    }
}


// Functions I grabbed from my old OS's? OS'es? I dunno my old string.h file.
size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}


void itoa(uint64_t n, char s[])
{
    uint64_t i;

    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    s[i] = '\0';
    reverse(s);
}

void itoa_hex(uint64_t n, char s[]) {
    uint64_t i;
    i = 0;
    do {
        long digit = n % 16;
        if (digit < 10)
            s[i++] = digit + '0';
        else
            s[i++] = digit - 10 + 'A';
    } while ((n /= 16) > 0);
    s[i] = '\0';
    reverse(s);
}

void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void colon_three() {
    print("\n\n\n\n"
        "             333333333333333\n"
        "            3:::::::::::::::33\n"
        "            3::::::33333::::::3\n"
        "            33333333    3:::::3\n"
        "                        3:::::3\n"
        "    ::::::              3:::::3\n"
        "    ::::::      33333333:::::3\n"
        "    ::::::      3:::::::::::3\n"
        "                33333333:::::3\n"
        "                        3:::::3\n"
        "                        3:::::3\n"
        "    ::::::              3:::::3\n"
        "    ::::::  33333333    3:::::3\n"
        "    ::::::  3::::::33333::::::3\n"
        "            3:::::::::::::::33\n"
        "             333333333333333\n"
        "                \n\n\n\n");
}


void kernel_panic(const char* msg) {
    text_color = 0x00000000;
    clear_screen();
    draw_trans_flag(framebuffer);
    print("KERNEL PANIC: ");
    print(msg);
    print("\n");
    colon_three();
    hcf();
}