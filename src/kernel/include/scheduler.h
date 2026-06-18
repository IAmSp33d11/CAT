#ifndef SCHEDULER_H
#define SCHEDULER_H

#define STATIC_ASSERT(expr) extern char compile_time_assert[(expr) ? 1 : -1]



typedef enum {
    STATUS_UNKNOWN,
    STATUS_DED,
    STATUS_RUNNING,
    STATUS_ALIVE
    
} prgm_status_t;

struct regs {
    uint64_t rax; // 0x0
    uint64_t rbx; // 0x8
    uint64_t rcx; // 0x10
    uint64_t rdx; // 0x18
    uint64_t rsi; // 0x20
    uint64_t rdi; // 0x28
    uint64_t rbp; // 0x30
    uint64_t rsp; // 0x38
    uint64_t r8; // 0x40
    uint64_t r9; // 0x48
    uint64_t r10; // 0x50
    uint64_t r11; // 0x58
    uint64_t r12; // 0x60
    uint64_t r13; // 0x68
    uint64_t r14; // 0x70
    uint64_t r15; // 0x78
    uint64_t rip; // 0x80
    uint64_t rflags; // 0x88
    uint64_t cr3; // 0x90
    uint64_t gs; // 0x98
    uint64_t fs; // 0x100
} __attribute__ ((packed));

struct prgm {
    prgm_status_t status;
    struct regs registers;
    uint8_t xsave_region[832] __attribute__((aligned(64))); // Thou shall not use AVX-512
    uint64_t kernel_stack_pointer;
    uint64_t pid; // Program ID
} __attribute__((aligned(16)));


#define TASKS_PER_PAGE ((4096 - (sizeof(uint8_t) + (sizeof(void*) * 2))) / sizeof(struct prgm))
#define TASKS_PAGE_BITMAP (~((1U << TASKS_PER_PAGE) - 1))

struct prgm_page {
    union {
        struct {
            struct prgm_page* prev;
            struct prgm_page* next;
            uint8_t task_allocation_mask;
            struct prgm slots[TASKS_PER_PAGE];
        };
        uint8_t raw_page_buffer[4096];
    };
}__attribute__((aligned(4096)));

STATIC_ASSERT(sizeof(struct prgm_page) == 4096);

void init_prgm_handler();
struct prgm_page* add_prgm_page();
struct prgm* add_prgm(uint64_t cr3, uint64_t rip, uint64_t rsp);
struct prgm* schedule();
void run_prgm(struct prgm* prgm);
#endif