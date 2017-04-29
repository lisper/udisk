/*
 * debug.c
 *                       
 * $Id: debug.c 49 2010-10-10 14:10:10Z brad $
 */

#include "main.h"
#include "board.h"


#ifdef DEBUG_VECTORS
void
markup_ram(void)
{
    unsigned int *p = (unsigned int *)AT91C_ISRAM;
    int i;

    for (i = 0; i < 8190; i++)
        *p++ = 0xffffffff;
}

void __data_abort(void) __attribute__ ((naked));
void __data_abort(void)
{
    register unsigned long *lnk_ptr;
    register unsigned long *stack_ptr, *old_sp;

    /* grab lr */
    __asm__ __volatile__ ("sub lr, lr, #8\n"
			  "mov %0, lr\n" : "=r" (lnk_ptr));

    /* reenter supervisor mode with interrupts disabled */
    __asm__ __volatile__ ("msr cpsr_c, #0xd3");

    /* */
    __asm__ __volatile__ ("mov %0, sp\n" : "=r" (old_sp));
    __asm__ __volatile__ ("stmfd sp!, { r0-r12, r14}");
    __asm__ __volatile__ ("mov %0, sp\n" : "=r" (stack_ptr));

//    enable_interrupts();

    /* On data abort exception the LR points to PC+8 */
    printf("data abort at %x\n", lnk_ptr);

    printf("regs:\n");
    printf("%x %x %x %x\n",
	   stack_ptr[0], stack_ptr[1], stack_ptr[2], stack_ptr[3]);

#if 0
    printf("%x ", stack_ptr[4]); puts(" ");
    printf("%x ", stack_ptr[5]); puts(" ");
    printf("%x ", stack_ptr[6]); puts(" ");
    printf("%x ", stack_ptr[7]); puts("\n");

    printf("%x ", stack_ptr[8]); puts(" ");
    printf("%x ", stack_ptr[9]); puts(" ");
    printf("%x ", stack_ptr[10]); puts(" ");
    printf("%x ", stack_ptr[11]); puts("\n");

    printf("%x ", stack_ptr[8]); puts(" ");
    printf("%x ", stack_ptr[9]); puts(" ");
    printf("%x ", stack_ptr[10]); puts(" ");
    printf("%x ", stack_ptr[11]); puts("\n");
#endif

    printf("r12 %x r14 %x", stack_ptr[12], stack_ptr[13]);

    for(;;);
}

void __prefetch_abort(void) __attribute__ ((naked));
void __prefetch_abort(void)
{
    register unsigned long *lnk_ptr;

    __asm__ __volatile__ (
        "sub lr, lr, #8\n"
        "mov %0, lr\n" : "=r" (lnk_ptr)
    );

    /* reenter supervisor mode with interrupts disabled */
    __asm__ __volatile__ (
	"msr cpsr_c, #0xd3"
    );

    /* On prefetch abort exception the LR points to PC+8 */
    printf("Prefetch Abort at %x\n", lnk_ptr);
    for(;;);
}

void __undefined_instruction(void) __attribute__ ((naked));
void __undefined_instruction(void)
{
    register unsigned long *lnk_ptr;

    __asm__ __volatile__ (
        "sub lr, lr, #8\n"
        "mov %0, lr\n" : "=r" (lnk_ptr)
    );

    /* reenter supervisor mode with interrupts disabled */
    __asm__ __volatile__ (
	"msr cpsr_c, #0xd3"
    );

    /* On prefetch abort exception the LR points to PC+8 */
    printf("undefined instruction at %x\n", lnk_ptr);
    for(;;);
}

void __software_interrupt(void) __attribute__ ((naked));
void __software_interrupt(void)
{
    register unsigned long *lnk_ptr;

    __asm__ __volatile__ (
        "sub lr, lr, #4\n"
        "mov %0, lr\n" : "=r" (lnk_ptr)
    );

    /* reenter supervisor mode with interrupts disabled */
    __asm__ __volatile__ (
	"msr cpsr_c, #0xd3"
    );

    /* On prefetch abort exception the LR points to PC+8 */
    printf("software interrupt at %x\n", lnk_ptr);
    for(;;);
}

void __spurious_handler(void) __attribute__ ((naked));
void __spurious_handler(void)
{
    register unsigned long *lnk_ptr;
    register unsigned long *stack_ptr, *old_sp;

    __asm__ __volatile__ (
        "sub lr, lr, #4\n"
        "mov %0, lr\n" : "=r" (lnk_ptr)
    );

    /* reenter supervisor mode with interrupts disabled */
    __asm__ __volatile__ (
	"msr cpsr_c, #0xd3"
    );

    /* */
    __asm__ __volatile__ ("mov %0, sp\n" : "=r" (old_sp));
    __asm__ __volatile__ ("stmfd sp!, { r0-r12, r14}");
    __asm__ __volatile__ ("mov %0, sp\n" : "=r" (stack_ptr));

    /* On data abort exception the LR points to PC+8 */
    printf("spurious interrupt at %x\n", lnk_ptr);

    printf("regs:\n");
    printf("%x %x %x %x\n",
	   stack_ptr[0], stack_ptr[1], stack_ptr[2], stack_ptr[3]);
    printf("r12 %x r14 %x", stack_ptr[12], stack_ptr[13]);

    for(;;);
}

#endif /* DEBUG_VECTORS */

void
debug_setup(void)
{
#ifdef DEBUG_VECTORS
    extern void __spurious_handler(void);
//    AT91C_BASE_AIC->AIC_SPU = (int)__spurious_handler;
#endif
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
