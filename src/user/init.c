#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


// Kernel will load us here. We have almost complete control of the machine.
// We must startup everything so everything else may run.
// But first.
// Test print.
void main(void);
void _start(void) {
    main();
    __asm__ volatile(
        "int $33"
        :
        : "a"(2), "b"(0)
        : "memory"
    );  
}


void sys_test(uint64_t action, const char* message) {
    __asm__ volatile (
        "int $33"
        :
        : "a"(action), "b"(message)
        : "memory"
    );
}

void main(void) {
    sys_test(1, "Hello, from init!\n");
}



