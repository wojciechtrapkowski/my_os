SECTIONS
{
    _kernel_start = .;  /* Beginning of kernel */
    
    /* Your existing sections here */
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    
    _kernel_end = .;    /* End of kernel */
} 