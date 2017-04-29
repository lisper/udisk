/*
 * start.S
 * $Id$
 */

#include "AT91SAM7S128_inc.h"

    .text
.globl _start
_start:
       	b	reset
	b	undefined_instruction
	b	software_interrupt
	b	abort_prefetch
	b	abort_data
	b	not_used
	b	irq_entry
	b	fiq

undefined_instruction:
	b	undefined_instruction
	
software_interrupt:
	b	software_interrupt

abort_prefetch:
	b	abort_prefetch

abort_data:
	b	abort_data

not_used:
	b	not_used

fiq:
	b	fiq

/*
 * Stack
 *
 * Interrupt and Supervisor Stack are separate.  Both are located at
 * the top of internal memory.
 *
 */

#define IRQ_STACK_SIZE	256

#define ARM_MODE_FIQ	0x11
#define ARM_MODE_IRQ	0x12
#define ARM_MODE_SVC	0x13

#define I_BIT		0x80
#define F_BIT		0x40

/*#define __iramend 0x7fff*/
/*#define __iramend 0x207ffc*/
#define __iramend 0x20fffc
	
reset:
        mrs     r0, cpsr		/* get CPSR */
        orr     r0, r0, #0xc0		/* disable IRQ, FIQ */
        msr     cpsr, r0		/* */
	
	ldr	r13, =__iramend		/* temporary stack in internal RAM */
#ifdef DEBUG_VECTORS_MARK
	ldr	r0, =markup_ram		/* fill ram w/ff's */
	mov	lr, pc
	bx	r0
#endif
	ldr	r0, =startup		/* low level init routine */
	mov	lr, pc
	bx	r0

	/* Setup the stack for each mode */
	ldr	r0, =__iramend

	/* Set up Fast Interrupt Mode and set FIQ Mode Stack */
	msr	CPSR_c, #ARM_MODE_FIQ | I_BIT | F_BIT
	
	/* Init the FIQ register */
	ldr	r8, =AT91C_BASE_AIC

	/* Set up Interrupt Mode and set IRQ Mode Stack */
	msr	CPSR_c, #ARM_MODE_IRQ | I_BIT | F_BIT
	mov	r13, r0		 /* Init stack IRQ */
	sub	r0, r0, #IRQ_STACK_SIZE

	/* disable ints, set supervisor mode and set supervisor mode stack */
	msr	CPSR_c, #ARM_MODE_SVC | I_BIT | F_BIT
	mov	r13, r0		 

	.globl main
	ldr	lr, =call_exit
	ldr	r0, =main
__main:
	bx	r0

	/* Loop for ever*/
call_exit:
End:	
	b	End


/* ------------------------------------------------------------------------- */

/*
 * lowest level interrupt code
 * called from IRQ vector
 */

irq_entry:
	/* adjust and save irq LR in irq stack */
	sub	lr, lr, #4
	stmfd	sp!, {lr}

	/* save r0 on irq stack */
	stmfd	sp!, {r0}

	/*
	 * write in the IVR to support protected mode
	 * no effect in normal mode
	 * de-assert the NIRQ and clear the source in protected mode
	 */
	ldr	r14, =AT91C_BASE_AIC
	ldr	r0 , [r14, #AIC_IVR]
	str	r14, [r14, #AIC_IVR]

       	/* switch to supervisor mode, interrupts disabled */
	msr	CPSR_c, #ARM_MODE_SVC | I_BIT

	/* save registers and LR in supervisor stack */
/*	stmfd	sp!, { r1-r3,r11-r12, lr } */
	stmfd	sp!, { r1-r12, lr }

        /* branch to central arm mode handle */
	mov	r14, pc
	bx	r0

       	/* restore registers and LR from supervisor stack */
/*	ldmia	sp!, { r1-r3,r11-r12, lr } */
	ldmia	sp!, { r1-r12, lr }

	/* disable Interrupt and switch back into irq mode */
	msr	CPSR_c, #I_BIT | ARM_MODE_IRQ

	/* mark the end of interrupt on the AIC */
	ldr	r14, =AT91C_BASE_AIC
	str	r14, [r14, #AIC_EOICR]

	/* restore ieq SPSR and r0 from IRQ stack */
	ldmia	sp!, {r0}

	/* restore adjusted irq LR from irq stack directly into the pc */
        /* (and return from interrupt) */
	ldmia	sp!, {pc}^

/*
 * Default routines which allow us to catch uninitialized irqs
 * (not that that will ever happen, but what the heck)
 */
	.globl	AT91F_Default_FIQ_handler
	.globl	AT91F_Default_IRQ_handler
	.globl	AT91F_Spurious_handler

AT91F_Default_FIQ_handler:
	b	AT91F_Default_FIQ_handler

AT91F_Default_IRQ_handler:
	b	AT91F_Default_IRQ_handler

AT91F_Spurious_handler:
#if 1
        bx      lr
#endif
        b	AT91F_Spurious_handler

/*
 *
 */
        .globl  disable_ints
disable_ints:    
	stmfd	sp!, {r0-r1}
        ldr     r1, =0x80
        b       disint
disable_fiq:
	stmfd	sp!, {r0-r1}
        ldr     r1, =0x40
disint: 
        mrs     r0, CPSR
        orr     r0, r0 ,r1
        msr     CPSR, r0
        mrs     r0, CPSR
        ands    r0, r0, r1
        beq     disint
	ldmia	sp!, {r0-r1}
        bx      lr

/*
 * vectors which get copied into ram before ram is mapped to 0
 */
        .globl copy_vectors
copy_vectors:
        ldr     pc, _reset
        ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _abort_prefetch
	ldr	pc, _abort_data
	ldr	pc, _not_used
	ldr	pc, _irq_entry
	ldr	pc, _fiq

/* the "debug vectors" print out messages when bad things happen */
#ifdef DEBUG_VECTORS
_reset:			.word     reset
_undefined_instruction: .word     __undefined_instruction
_software_interrupt:	.word     __software_interrupt
_abort_prefetch:	.word     __prefetch_abort
_abort_data:		.word     __data_abort
_not_used:		.word     not_used
_irq_entry:		.word     irq_entry
_fiq:           	.word     fiq
#else
_reset:			.word     reset
_undefined_instruction: .word     undefined_instruction
_software_interrupt:	.word     software_interrupt
_abort_prefetch:	.word     abort_prefetch
_abort_data:		.word     abort_data
_not_used:		.word     not_used
_irq_entry:		.word     irq_entry
_fiq:           	.word     fiq
#endif
        

/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
