#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

// Halt and catch fire function.
void hcf(void) {
    __asm__ volatile("cli; hlt;");
}

void rdmsr(uint32_t msr, uint32_t *low, uint32_t *high) {
    __asm__ __volatile__("rdmsr" : "=a"(*low), "=d"(*high) : "c"(msr));
}