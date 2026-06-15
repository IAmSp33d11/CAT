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

    ret

.extern saved_kernel_rsp
.extern saved_kernel_rip

.global jump_to_usermode
# void jump_to_usermode(uint64_t entry_point, uint64_t user_stack);
jump_to_usermode:
    # Disable interrupts so shit don't break
    cli

    leaq .kernel_return(%rip), %rax
    movq %rax, saved_kernel_rip(%rip)
    movq %rsp, saved_kernel_rsp(%rip)


    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushq $0x23

    # Push the Stack Pointer
    pushq %rsi

    # Allow Interrupts
    pushq $0x202

    # Push a fake CS
    pushq $0x1B

    # Push fake RIP
    pushq %rdi


    iretq

.kernel_return:
    sti
    ret

.global return_to_kernel
# void return_to_kernel(void);
return_to_kernel:
    cli

    mov $0x80B, %ecx
    xor %eax, %eax
    xor %edx, %edx
    wrmsr

    movq saved_kernel_rsp(%rip), %rsp
    movq saved_kernel_rip(%rip), %rax

    jmp *%rax