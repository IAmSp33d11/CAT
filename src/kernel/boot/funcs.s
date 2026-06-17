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

.global enable_SSE
.type enable_SSE, @function
enable_SSE:
# void enable_SSE(void);
mov %cr0, %rax
    and $0xFFFFFFFFFFFFFFFB, %rax
    or  $0x2, %rax
    mov %rax, %cr0

    mov %cr4, %rax
    or  $0x600, %rax
    mov %rax, %cr4

    mov %cr4, %rax
    or  $0x200, %rax
    mov %rax, %cr4

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


