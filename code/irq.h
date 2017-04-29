/*
 * irq.h
 *                       
 * $Id: irq.h 70 2013-11-17 17:59:13Z brad $
 */

#if 0
#define DISABLE_INTERRUPTS() \
    asm volatile ( "stmdb   sp!, {r0}" );     /* push r0 */                   \
    asm volatile ( "mrs     r0, cpsr" );      /* get CPSR */                  \
    asm volatile ( "orr     r0, r0, #0xc0" ); /* disable IRQ, FIQ */          \
    asm volatile ( "msr     cpsr, r0" );      /* write back modified value */ \
    asm volatile ( "ldmia   sp!, {r0}" )      /* pop r0 */

#define ENABLE_INTERRUPTS() \
    asm volatile ( "stmdb   sp!, {r0}" );     /* push r0 */                   \
    asm volatile ( "mrs     r0, cpsr" );      /* get CPSR */                  \
    asm volatile ( "bic     r0, r0, #0xc0" ); /* enable IRQ, FIQ */           \
    asm volatile ( "msr     cpsr, r0" );      /* write back modified value */ \
    asm volatile ( "ldmia   sp!, {r0}" )      /* pop r0 */
#endif

#define DISABLE_INTERRUPTS() \
	{ u32 scratch; \
	asm volatile("mrs %[reg], cpsr\n\t" \
		     "orr %[reg], %[reg], #0xC0\n\t" \
		     "msr cpsr_c, %[reg]\n\t" : [reg] "=r" (scratch) :: "cc"); }

#define ENABLE_INTERRUPTS() \
	{ u32 scratch; \
	asm volatile("mrs %[reg], cpsr\n" \
		     "bic %[reg], %[reg], #0xC0\n" \
		     "msr cpsr_c, %[reg]" : [reg] "=r" (scratch) :: "cc"); }

int irq_configure(int irq_id,          /* interrupt number to initialize */
		  int priority,        /* priority to give to the interrupt */
		  int src_type,        /* activation and sense of activation */
		  void (*handler)(void)); /* interrupt handler */

void irq_enable(int irq_id);
void irq_disable(int irq_id);
void irq_clear(int irq_id);
void irq_ack(void);
void irq_trigger(int irq_id);
int irq_pending(int irq_id);
int irq_active(void);


