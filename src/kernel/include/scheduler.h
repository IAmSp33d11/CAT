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
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cr3;
    uint64_t gs;
    uint64_t fs; // If someone wants to use these, we'll also save them just in case.
} __attribute__ ((packed));

struct prgm {
    prgm_status_t status;
    struct regs registers;
    uint8_t fxsave_region[512] __attribute__((aligned(16))); // This is for our FPU data
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
struct prgm* get_current_running_prgm();
#endif