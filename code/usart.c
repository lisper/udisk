/*
 * usart.c
 *                       
 * $Id: usart.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"

unsigned long usart_ints;

/* Standard Asynchronous Mode : 8 bits , 1 stop , no parity */
#define AT91C_US_ASYNC_MODE ( AT91C_US_USMODE_NORMAL + \
                        AT91C_US_NBSTOP_1_BIT + \
                        AT91C_US_PAR_NONE + \
                        AT91C_US_CHRL_8_BITS + \
                        AT91C_US_CLKS_CLOCK )

inline void AT91F_PDC_SetNextRx (AT91PS_PDC pPDC,
				 char *address,
				 unsigned int bytes)
{
    pPDC->PDC_RNPR = (unsigned int) address;
    pPDC->PDC_RNCR = bytes;
}

inline void AT91F_PDC_SetNextTx (
	AT91PS_PDC pPDC,
	char *address,
	unsigned int bytes)
{
    pPDC->PDC_TNPR = (unsigned int) address;
    pPDC->PDC_TNCR = bytes;
}

inline void AT91F_PDC_SetRx (
	AT91PS_PDC pPDC,
	char *address,
	unsigned int bytes)
{
    pPDC->PDC_RPR = (unsigned int) address;
    pPDC->PDC_RCR = bytes;
}

inline void AT91F_PDC_SetTx (
	AT91PS_PDC pPDC,
	char *address,
	unsigned int bytes)
{
    pPDC->PDC_TPR = (unsigned int) address;
    pPDC->PDC_TCR = bytes;
}

inline void AT91F_PDC_Open (
	AT91PS_PDC pPDC)
{
    /* Disable the RX and TX PDC transfer requests */
    pPDC->PDC_PTCR = AT91C_PDC_RXTDIS;
    pPDC->PDC_PTCR = AT91C_PDC_TXTDIS;

    /* Reset all Counter register Next buffer first */
    AT91F_PDC_SetNextTx(pPDC, (char *) 0, 0);
    AT91F_PDC_SetNextRx(pPDC, (char *) 0, 0);
    AT91F_PDC_SetTx(pPDC, (char *) 0, 0);
    AT91F_PDC_SetRx(pPDC, (char *) 0, 0);

    /* Enable the RX and TX PDC transfer requests */
    pPDC->PDC_PTCR = AT91C_PDC_TXTEN;
    pPDC->PDC_PTCR = AT91C_PDC_RXTEN;
}


unsigned int
AT91F_US_Baudrate (const unsigned int main_clock, // peripheral clock
                   const unsigned int baud_rate)  // UART baudrate
{
    unsigned int baud_value = ((main_clock*10)/(baud_rate * 16));
    if ((baud_value % 10) >= 5)
        baud_value = (baud_value / 10) + 1;
    else
        baud_value /= 10;
    return baud_value;
}


void
AT91F_US_Configure (
	AT91PS_USART pUSART,     // pointer to a USART controller
	unsigned int mainClock,  // peripheral clock
	unsigned int mode ,      // mode Register to be programmed
	unsigned int baudRate ,  // baudrate to be programmed
	unsigned int timeguard ) // timeguard to be programmed
{
    /* Disable interrupts */
    pUSART->US_IDR = (unsigned int) -1;

    /* Reset receiver and transmitter */
    pUSART->US_CR =
        AT91C_US_RSTRX | AT91C_US_RSTTX |
        AT91C_US_RXDIS | AT91C_US_TXDIS;

    /* Define the baud rate divisor register */
    pUSART->US_BRGR = AT91F_US_Baudrate(mainClock, baudRate);

    /* Write the Timeguard Register */
    pUSART->US_TTGR = timeguard;

    /* Clear Transmit and Receive Counters */
    AT91F_PDC_Open((AT91PS_PDC) &(pUSART->US_RPR));

    /* Define the USART mode */
    pUSART->US_MR = mode;
}

#if 1
void
usart_tx_flush(void)
{
    while (!usart_tx_empty())
        ;
}


int
usart_tx_empty(void)
{
    volatile AT91PS_USART pUSART = AT91C_BASE_US1;

    if ((pUSART->US_CSR & AT91C_US_TXEMPTY) == 0)
        return 0;

    return 1;
}

static void d(void) {
	volatile long l;
	for (l = 0; l < 5000; l++)
		;
}

void
usart_putc(int c)
{
    volatile AT91PS_USART pUSART = AT91C_BASE_US1;

#if 0
    if (pUSART->US_CSR & AT91C_US_TXRDY) {
        pUSART->US_THR = c & 0xff;
    }
    d();
#endif

    if (pUSART->US_CSR & AT91C_US_TXEMPTY)
        ;
    else
        while (!(pUSART->US_CSR & AT91C_US_TXRDY))
            ;

    pUSART->US_THR = c & 0xff;
}

int
usart_getc_rdy(void)
{
    volatile AT91PS_USART pUSART = AT91C_BASE_US1;

    if ((pUSART->US_CSR & AT91C_US_RXRDY))
        return 1;
    return 0;
}

int
usart_getc(void)
{
    volatile AT91PS_USART pUSART = AT91C_BASE_US1;

    while (!(pUSART->US_CSR & AT91C_US_RXRDY))
        ;

    return pUSART->US_RHR & 0xff;
}
#endif

void
usart_enable_int(void)
{
    enable_interrupts();
//    irq_enable(AT91C_ID_US1);
}

void
usart_disable_int(void)
{
//    irq_disable(AT91C_ID_US1);
    disable_interrupts();
}

void
usart_c_irq_handler(void)
{
    AT91PS_USART pUSART = AT91C_BASE_US1;
    unsigned int status;

    usart_ints++;

    /* get USART status register */
    status = pUSART->US_CSR;

    /* input? */
    if (status & AT91C_US_RXRDY) {
	u_char c = pUSART->US_RHR & 0xff;
        rx_ring_add(c);
    }

    if (status & AT91C_US_TXRDY) {
    }

    if (status & AT91C_US_OVRE) {
    }

    /* check error */
    if (status & AT91C_US_PARE) {
    }

    if (status & AT91C_US_FRAME) {
    }

    if (status & AT91C_US_TIMEOUT) {
        pUSART->US_CR = AT91C_US_STTTO;
    }

    /* reset the status bit */
    pUSART->US_CR = AT91C_US_RSTSTA;
}

void
usart_setup(void)
{
	volatile AT91PS_USART pUSART = AT91C_BASE_US1;
	AT91PS_PIO pPio = AT91C_BASE_PIOA;
	AT91PS_PMC pPMC = AT91C_BASE_PMC;

   	/* enable the PIO clock */
	pPMC->PMC_PCER = 1 << AT91C_ID_PIOA;

 	/* configure PIO controllers to periph mode */
	pPio->PIO_ASR = AT91C_PA21_RXD1 | AT91C_PA22_TXD1;
	pPio->PIO_BSR = 0;
	pPio->PIO_PDR = AT91C_PA21_RXD1 | AT91C_PA22_TXD1;

   	/* first, enable the clock of the PIOB */
	pPMC->PMC_PCER = 1 << AT91C_ID_US1;

	/* USART configure */
	AT91F_US_Configure(pUSART, MCK, AT91C_US_ASYNC_MODE,
                           AT91_BAUD_RATE, 0);

	/* Enable usart */
	pUSART->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;

        //pUSART->US_THR = '\n';

#if 0
	/* Enable USART IT error and RXRDY */
    	pUSART->US_IER = 
            AT91C_US_TIMEOUT |
            AT91C_US_FRAME |
            AT91C_US_OVRE |
            AT91C_US_RXRDY /*| AT91C_US_TXRDY*/;

    	/* setup Usart 1 interrupt */
	irq_configure(AT91C_ID_US1,
                      USART_INTERRUPT_LEVEL,
                      AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE,
                      usart_c_irq_handler);

        usart_ints = 0;

	irq_enable(AT91C_ID_US1);
#endif
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
