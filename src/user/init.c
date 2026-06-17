#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


// Kernel will load us here. We have almost complete control of the machine.
// We must startup everything so everything else may run.
// But first.
// Test print.
void main(void);
__attribute__((naked, section(".text.prologue"))) void _start(void) {
    main();
    __asm__ volatile (
        "syscall"
        :
        : "a"(2)
        : "rcx", "r11", "memory"
    );
}


inline void sys_test(uint64_t action, const char* message) {
    __asm__ volatile (
        "syscall"
        :
        : "a"(action), "b"(message)
        : "rcx", "r11", "memory"
    );
}



void main(void) {
    __asm__ volatile (
        "syscall"
        :
        : "a"(4)
        : "rcx", "r11", "memory"
    );

    __asm__ volatile (
        "syscall"
        :
        : "a"(3)
        : "rcx", "r11", "memory"
    );

    __asm__ volatile (
        "syscall"
        :
        : "a"(1), "b"("Hello, from init!\n")
        : "rcx", "r11", "memory"
    );


}



