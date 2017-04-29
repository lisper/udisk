/*
 * startup.c
 * $Id: startup.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"

extern void AT91F_Default_FIQ_handler(void);
extern void AT91F_Default_IRQ_handler(void);
extern void AT91F_Spurious_handler(void);

/* called from start.S, initialize the cpu & clocks */

void
startup(void)
{
    AT91PS_PMC pPMC = AT91C_BASE_PMC;
    AT91PS_RSTC pRSTC = AT91C_BASE_RSTC;

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

/* loader runs with interrupts disabled */
void
disable_interrupts(void)
{
}

void
enable_interrupts(void)
{
}

int
ints_are_enabled(void)
{
    return 0;
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
