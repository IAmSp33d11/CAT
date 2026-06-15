#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "helpful.h"
#include "asm.h"

// Yes, I got most of this from https://osdev.wiki/wiki/Interrupts_Tutorial.

typedef struct {
	uint16_t    isr_low;      // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t    isr_high;     // The higher 32 bits of the ISR's address
	uint32_t    reserved;     // Set to zero
} __attribute__((packed)) idt_entry_t;

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

typedef struct {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed)) idtr_t;

static idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08;
    descriptor->ist            = 0;
    descriptor->attributes     = flags;
    descriptor->isr_mid        = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high       = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved       = 0;
}

#define IDT_MAX_DESCRIPTORS 48


static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

void idt_init(void);
void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    idt_set_descriptor(33, isr_stub_table[33], 0xEE);

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
}


// Everything above this we do not touchy. Like ever.
// If you touch this I will personally burn your house down with lemons
// Everything below this is just interrupts.
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    
    uint64_t interrupt_number;
    uint64_t error_code;
    
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) registers_t;

extern void return_to_kernel(void);



void exception_handler(registers_t* regs) {
    if (regs->interrupt_number == 14) {
        uint64_t cr2_val;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2_val));
        bool is_user_mode = (regs->error_code & (1 << 2)) != 0;
        if (is_user_mode) {
            print("Segmentation Fault: Core Dumped.\n");
            regs->rip = (uint64_t)return_to_kernel;
            regs->cs = 0x08;
            regs->ss = 0x10;
            return;
        }
    } else if (regs->interrupt_number == 13) {
        bool is_user_mode = (regs->cs & 0x3) == 3;
        if (is_user_mode) {
            print("General Protection Fault in Usermode: Process Terminated.\n");
            // Kill the usermode task by redirecting RIP back to your kernel scheduler/cleanup
            regs->rip = (uint64_t)return_to_kernel;
            regs->cs = 0x08;  // Your kernel CS
            regs->ss = 0x10;  // Your kernel SS
            return;
        }
    }

    clear_screen();
    
    char buf[128];
    print("CRASHED AT VECTOR: ");
    itoa(regs->interrupt_number, buf);
    print(buf);
    print("\n");

    print("ERROR CODE: 0x");
    itoa_hex(regs->error_code, buf);
    print(buf);
    print("\n");

    print("FAULTING RIP: 0x");
    itoa_hex(regs->rip, buf);
    print(buf);
    print("\n");

    // If it's a Page Fault (Vector 14), read CR2 to find the exact offending address
    if (regs->interrupt_number == 14) {
        uint64_t cr2_val;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2_val));
        print("CR2 VALUE: 0x");
        itoa_hex(cr2_val, buf);
        print(buf);
        print("\n");
    }

    print("All right, I've been thinking.\nWhen life gives you lemons?\nDon't make lemonade.\nMake life take the lemons back!\nGet mad!\nI don't want your damn lemons!\nWhat am I supposed to do with these?\nDemand to see life's manager!\nMake life rue the day it thought it could give Cave Johnson lemons!\nDo you know who I am?\nI'm the man who's going to burn your house down!\nWith the lemons!\nI'm going to get my engineers to invent a combustible lemon that burns your house down!\n");

    __asm__ volatile ("cli; hlt");
}

void test(uint64_t rax, uint64_t rbx) {
    if (rax == 1) {
        print((char*) rbx);
    } else if (rax == 2) {
        return_to_kernel();
    }
}

void tick() {
    print("We got a timer interrupt!\n");

    __asm__ volatile(
        "mov $0x80b, %%ecx\n\t"
        "xor %%eax, %%eax\n\t"
        "xor %%edx, %%edx\n\t"
        "wrmsr"
        : : : "rax", "rcx", "rdx"
    );
}

__attribute__((noreturn))
void division_handler(void);
void division_handler() {
    kernel_panic("Division Error In Kernel?");
    __asm__ volatile ("cli; hlt");
}