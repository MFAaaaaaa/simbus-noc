OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80000000;
  .text : { *(.text.init) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }

  . = 0x80001000;
  .tohost : { KEEP(*(.tohost)) }

  /DISCARD/ : { *(.comment) *(.riscv.attributes) }
}
