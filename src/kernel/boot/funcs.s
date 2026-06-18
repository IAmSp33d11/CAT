.macro pushaq
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro popaq
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm


.section .data
.balign 1
gdtr:
    .word 0 # For limit storage
    .quad 0 # For base storage


.section .text
.global setGdt
# setGdt(uint16_t limit, uint64_t base)
setGdt:
    movw %di, gdtr
    movq %rsi, gdtr+2
    lgdt gdtr
    

reloadSegments:
    pushq $0x08
    leaq .reload_CS(%rip), %rax
    pushq %rax
    lretq

.reload_CS:
    movw $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ret

.global check_support
# bool check_support(void);
check_support:
    pushaq

    # Check if we support XSAVE
    mov $1, %eax
    cpuid
    bt $26, %ecx
    jnc .false

    popaq
    mov $1, %rax
    ret

.false:
    popaq
    mov $0, %rax
    ret

.global enable_stuff
.type enable_stuff, @function
enable_stuff:
# void enable_stuff(void);
    movq %cr0, %rax
    andq $0xFFFFFFFFFFFFFFFB, %rax
    orq  $0x2, %rax
    movq %rax, %cr0

    movq %cr4, %rax
    orq  $0x600, %rax
    movq %rax, %cr4

    movq %cr4, %rax
    orq  $0x200, %rax
    movq %rax, %cr4

    movq %cr4, %rax
    orq $(1 << 18), %rax
    movq %rax, %cr4

    ret

.global jump_to_usermode
# void jump_to_usermode(uint64_t entry_point, uint64_t user_stack);
jump_to_usermode:
    # Disable interrupts so shit don't break
    cli

    leaq .kernel_return(%rip), %rax
    movq %rax, saved_kernel_rip(%rip)
    movq %rsp, saved_kernel_rsp(%rip)


    movq $202, %r11
    movq %rdi, %rcx

    movq %rsi, %rsp

    sysretq

.kernel_return:
    sti
    ret

.global return_to_kernel
# void return_to_kernel(void);
return_to_kernel:
    cli

    movq saved_kernel_rsp(%rip), %rsp
    movq saved_kernel_rip(%rip), %rax

    jmp *%rax



.global syscall_setup
# void syscall_setup(void);
syscall_setup:
    movl $0xC0000080, %ecx
    rdmsr
    orl  $1, %eax
    wrmsr


    movl $0xC0000082, %ecx

    movabs $syscall_stub, %rax

    movq %rax, %rdx
    shrq $32, %rdx

    wrmsr

    movl $0xC0000081, %ecx
    movl $0x00180008, %edx
    xorl %eax, %eax
    wrmsr

    movl $0xC0000084, %ecx
    movl $0x200, %eax
    xorl %edx, %edx
    wrmsr

    ret




.extern syscall
# void syscall(uint64_t rax, uint64_t rbx, uint64_t rdx, uint64_t r8, uint64_t r9);
syscall_stub:
    swapgs

    xchgq %rsp, %gs:0

    pushq %rcx # return address
    pushq %r11 # RFLAGS

    movq %rax, %rdi
    movq %rbx, %rsi
    movq %r8, %rcx
    movq %r9, %r8

    call syscall


    popq %r11
    popq %rcx

    xchgq %rsp, %gs:0

    swapgs

    sysretq


.global start_scheduler
# void start_scheduler(struct regs* registers, uint8_t xsave);
start_scheduler:

    leaq .kernel_return(%rip), %rax
    movq %rax, saved_kernel_rip(%rip)
    movq %rsp, saved_kernel_rsp(%rip)

    movl $0x7, %eax
    xor %edx, %edx
    xsave (%rsi)

    movq 0x90(%rdi), %rax
    movq %rax, %cr3

    movq 0x0(%rdi), %rax
    movq 0x8(%rdi), %rbx
    movq 0x18(%rdi), %rdx
    movq 0x20(%rdi), %rsi
    movq 0x30(%rdi), %rbp
    movq 0x40(%rdi), %r8
    movq 0x48(%rdi), %r9
    movq 0x50(%rdi), %r10
    movq 0x60(%rdi), %r12
    movq 0x68(%rdi), %r13
    movq 0x70(%rdi), %r14
    movq 0x78(%rdi), %r15

    movq 0x80(%rdi), %rcx # Program address
    movq 0x88(%rdi), %r11 # RFLAGS

    movq 0x38(%rdi), %rsp # Stack Pointer

    sysretq


