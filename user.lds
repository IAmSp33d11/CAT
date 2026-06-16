OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text.prologue)
        *(.text)
    }

    .data : { *(.data) }
    .bss  : { *(.bss) }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}