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

	_rom_end = ABSOLUTE(.);

	. = 0x200040;
	.bss : { *(.bss) }
}
