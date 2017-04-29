/*
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
ENTRY(_start)

SECTIONS
{
	. = 0x0;
	.text  : { *(.text); }

	. = ALIGN(4);
	.rodata : { *(.rodata) *(.rodata.*) }
	.data : { *(.data) }
	.bss : { *(.bss) }
	_ram_end = ABSOLUTE(.);
}
