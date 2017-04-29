/*
 * irq.c
 * $Id: irq.c 49 2010-10-10 14:10:10Z brad $
 */

#include "main.h"
#include "board.h"
#include "irq.h"

int
irq_configure(int irq_id,   /* interrupt number to initialize */
	      int priority, /* priority to give to the interrupt */
	      int src_type, /* activation and sense of activation */
	      void (*handler)(void)) /* interrupt handler */
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;
    unsigned int oldHandler, mask;

    extern void irq_handler(int);

    oldHandler = pAic->AIC_SVR[irq_id];

    mask = 1 << irq_id;

    /* disable the interrupt on the interrupt controller */
    pAic->AIC_IDCR = mask;

    /* set irq stub to call - it calls handler */
    pAic->AIC_SVR[irq_id] = (unsigned int)handler;

    /* store the source mode register and the interrupt priority */
    pAic->AIC_SMR[irq_id] = src_type | priority;

    /* clear the interrupt on the interrupt controller */
    pAic->AIC_ICCR = mask;

    return 0;
}

void
irq_enable(int irq_id)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;

    /* enable the interrupt on the interrupt controller */
    pAic->AIC_IECR = 1 << irq_id;
}

void
irq_disable(int irq_id)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;
    unsigned int mask = 1 << irq_id;

    /* disable the interrupt on the interrupt controller */
    pAic->AIC_IDCR = mask;

#if 0
//this is dangerous and incorrect
    /* clear the interrupt on the interrupt controller */
    /* (in case one is pending) */
    pAic->AIC_ICCR = mask;
#endif
}

void
irq_clear(int irq_id)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;

    /* clear the interrupt on the interrupt controller */
    pAic->AIC_ICCR = 1 << irq_id;
}

void
irq_ack(void)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;

    pAic->AIC_EOICR = pAic->AIC_EOICR;
}

void
irq_trigger(int irq_id)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;

    pAic->AIC_ISCR = 1 << irq_id;
}

int
irq_active(void)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;

    return pAic->AIC_ISR;
}

int
irq_pending(int irq_id)
{
    AT91PS_AIC pAic = AT91C_BASE_AIC;

    return pAic->AIC_IPR & (1 << irq_id);
}

void
irq_setup(void)
{
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
