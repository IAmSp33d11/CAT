#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

// Halt and catch fire function.
void hcf(void) {
    __asm__ volatile("cli; hlt;");
}

