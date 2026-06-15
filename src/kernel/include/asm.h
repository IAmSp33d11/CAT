#ifndef ASM_H
#define ASM_H

#define fence() __asm__ volatile ("":::"memory")
#define load_fence() __asm__ volatile("LFENCE":::"memory")
#define mem_fence() __asm__ volatile("MFENCE":::"memory")
#define store_fence() __asm__ volatile("SFENCE":::"memory")

void hcf(void);



inline uint64_t rdmsr(uint64_t msr)
{
    uint32_t low, high;
    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );
	return ((uint64_t)high << 32) | low;
}

inline void wrmsr(uint64_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(low), "d"(high)
    );
}

inline void invlpg(void* m)
{
    /* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
    __asm__ volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}


// Direct I/O port helpers
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif