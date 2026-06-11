#ifndef HELPFUL_H
#define HELPFUL_H

#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);


// Direct I/O port helpers
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint64_t rdtsc(void) {
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

static inline uint16_t read_pit_count(void) {
    uint8_t low, high;
    // Latch channel 0
    outb(0x43, 0b00000000); 
    low = inb(0x40);
    high = inb(0x40);
    return (high << 8) | low;
}


uint64_t calibrate_tsc(void);
void hcf(void);
void panic(struct limine_framebuffer *framebuffer);
void draw_trans_flag(struct limine_framebuffer *framebuffer);
void draw_char(struct limine_framebuffer *framebuffer, uint8_t *font_buffer, 
               char c, size_t start_x, size_t start_y, uint32_t text_color);
void clear_screen();
void print(const char *str);
void panic_but_msg(struct limine_framebuffer *framebuffer, const char* msg);


extern struct limine_framebuffer *framebuffer;
extern uint8_t *font_buffer;
extern uint32_t text_color;

void reverse(char s[]);
void itoa_hex(uint32_t n, char s[]);
void itoa(uint32_t n, char s[]);
size_t strlen(const char* str);

void colon_three();
void kernel_panic(const char* msg);

#endif