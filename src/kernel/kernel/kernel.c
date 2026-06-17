#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "asm.h"
#include "helpful.h"
#include "physmem.h"
#include "virtmem.h"
#include "acpi.h"
#include "apic.h"

extern void return_to_kernel(void);

void syscall(uint64_t rax, uint64_t rbx, uint64_t rdx, uint64_t r8, uint64_t r9) {
    if (rax == 1) {
        print((char*) rbx);
    } else if (rax == 2) {
        return_to_kernel();
    } else if (rax == 3) {
        colon_three();
    } else if (rax == 4) {
        clear_screen();
    }
}