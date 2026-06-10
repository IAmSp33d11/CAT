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