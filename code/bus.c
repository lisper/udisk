/*
 * bus.c
 * $Id: bus.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "bus.h"

void
cpld_reset(void)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
        int i;
        /* reset cpld */
	pPio->PIO_SODR = P_CPLD_RD | P_CPLD_WR;
//        for (i = 0; i < 5; i++)
            delayus();
	pPio->PIO_CODR = P_CPLD_RD | P_CPLD_WR;
}

/* called by main setup code to initialize the CPLD bus */

void
bus_setup(void)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	AT91PS_PMC pPMC = AT91C_BASE_PMC;
	int bus, addr, drive, input;

        /* enable cpld clock (pa31) */
        pPio->PIO_PDR = 0x80000000;
        pPio->PIO_MDDR = 0x80000000;
        pPio->PIO_PPUDR = 0x80000000;
        pPio->PIO_BSR = 0x80000000;

        // pll_clk / 2 (same as main clk)
        AT91C_PMC_PCKR[2] = (1 << 2) | 0x03;

        delayus();
//        while ((*AT91C_PMC_SR & 0x400) == 0)
//            ;

        /* enable pclk2 */
        *AT91C_PMC_SCER = 0x400;

   	/* enable the PIO clock */
	pPMC->PMC_PCER = 1 << AT91C_ID_PIOA;

	bus = BUS;

	addr = P_A3 | P_A2 | P_A1 | P_A0;

	drive = P_CPLD_RD | P_CPLD_WR;

	input = 0;

        pPio->PIO_OWER = addr | drive;

        cpld_reset();

	pPio->PIO_CODR = P_CPLD_RD | P_CPLD_WR;
	pPio->PIO_CODR = P_A0 | P_A1 | P_A2 | P_A3;

        pPio->PIO_PER = bus | addr | drive | input;
        pPio->PIO_OER = addr | drive;

        /* */
        pPio->PIO_OWER = P_USB_PUP;
        pPio->PIO_PER = P_USB_PUP;
        pPio->PIO_OER = P_USB_PUP;
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
