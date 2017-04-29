/*
 * startup.c
 * $Id: startup.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "irq.h"

extern void AT91F_Default_FIQ_handler(void);
extern void AT91F_Default_IRQ_handler(void);
extern void AT91F_Spurious_handler(void);

int loc0_is_ram(void)
{
    volatile u32 *p = 0x0;

    /* see if we can write location 0x0 */
    *p = 0x1234;
    if (*p != 0x1234)
        return 0;

    *p = 0x1235;
    if (*p != 0x1235)
        return 0;

    return 1;
}

void force_mapping_to_ram()
{
    AT91PS_MC pMC = (AT91PS_MC) AT91C_BASE_MC;

    if (loc0_is_ram())
        /* ok */;
    else {
        /* toggle */
        pMC->MC_RCR = AT91C_MC_RCB;
    }
}

void force_mapping_to_rom()
{
    AT91PS_MC pMC = (AT91PS_MC) AT91C_BASE_MC;

    if (loc0_is_ram()) {
        /* toggle mapping of flash/sram to 0x0 */
        pMC->MC_RCR = AT91C_MC_RCB;

#if 0
        if (loc0_is_ram())
            printf("after undo, not ok (still ram)\n");
        else
            printf("after undo, ok\n");
#endif
    }
}


/* set up default interrupt handlers */

void
startup_irq_vectors(void)
{
    int i;
    AT91PS_MC pMC = (AT91PS_MC) AT91C_BASE_MC;

    extern char copy_vectors[];

    /* copy vectors to the beginging of ram */
    memcpy(AT91C_ISRAM, copy_vectors, 64);

    /* then map ram to location zero so irq vectors work */
    force_mapping_to_ram();

    /* set up the default interrupts handler vectors */
    AT91C_BASE_AIC->AIC_SVR[0] = (int)AT91F_Default_FIQ_handler;

    for (i = 1; i < 31; i++) {
        AT91C_BASE_AIC->AIC_SVR[i] = (int)AT91F_Default_IRQ_handler;
    }

    AT91C_BASE_AIC->AIC_SPU = (int)AT91F_Spurious_handler;
}

/* called from start.S, initialize the cpu & clocks */

void
startup(void)
{
    AT91PS_PMC pPMC = AT91C_BASE_PMC;
    AT91PS_RSTC pRSTC = AT91C_BASE_RSTC;

    /* copy vectors to ram and map ram to low memory */
    startup_irq_vectors();

    /* don't init cpu if we're running from flash - bootrom did it */
    if (!in_ram()) {
        return;
    }

    /* allow reset button to cause reset */
    pRSTC->RSTC_RMR = 0xA5000000 | AT91C_RSTC_URSTEN;

    /*
     * set flash wait state
     */
    AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(48 <<16)) | AT91C_MC_FWS_1FWS ;

    /* disable watchdog */
    AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;

    /*
     * 1. turn on main osc 
     */
    pPMC->PMC_MOR = (( AT91C_CKGR_OSCOUNT & (0x06 <<8) | AT91C_CKGR_MOSCEN ));

    /* wait the startup time */
    while (!(pPMC->PMC_SR & AT91C_PMC_MOSCS))
        ;

    /*
     * 2. Checking the Main Oscillator Frequency (Optional)
     * 3. Setting PLL and divider:
     * - Div by 2: Fin = 8,0000 = (16,000 / 2)
     * - Mul by 11+1: Fout = 96,0000 = (8,0000 *12)
     * for 96 MHz the error is 0.0%
     * Field out NOT USED = 0
     * PLLCOUNT pll startup time estimate at : 0.844 ms
     * PLLCOUNT 28 = 0.000844 /(1/32768)
     */
    pPMC->PMC_PLLR = ((AT91C_CKGR_DIV & 0x05) |
                      (AT91C_CKGR_PLLCOUNT & (28<<8)) |
                      (AT91C_CKGR_MUL & (25<<16)));

    /* wait the startup time */
    while(!(pPMC->PMC_SR & AT91C_PMC_LOCK))
        ;

    while(!(pPMC->PMC_SR & AT91C_PMC_MCKRDY))
        ;

    /*
     * 4. selection of master clock and processor clock
     *    select the PLL clock divided by 2
     */
    pPMC->PMC_MCKR =  AT91C_PMC_PRES_CLK_2;

    while (!(pPMC->PMC_SR & AT91C_PMC_MCKRDY))
        ;

    pPMC->PMC_MCKR |= AT91C_PMC_CSS_PLL_CLK;
    while (!(pPMC->PMC_SR & AT91C_PMC_MCKRDY))
        ;
}

void
disable_interrupts(void)
{
//    DISABLE_INTERRUPTS();
    disable_ints();
}

void
enable_interrupts(void)
{
    ENABLE_INTERRUPTS();
}

int
ints_are_enabled(void)
{
    asm volatile ( "mrs     r0, cpsr" );      /* get CPSR */                  \
    asm volatile ( "and     r0, r0, #0xc0" ); /* isolate IRQ, FIQ */          \
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
