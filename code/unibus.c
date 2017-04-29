#include "main.h"
#include "board.h"
#include "bus.h"
#include "cpld.h"

/*
 * assert an interrupt on the UNIBUS
 */

extern u_short vector;
extern int gen_ints;
extern int dma_writes;
extern int dma_reads;

/*
 * DMA to/from unibus
 */

void
unibus_dma_buffer(int write, unsigned int addr, u_short *data, int len)
{
    u_char burst, burst_size;

if (write) dma_writes++; else dma_reads++;

//    burst_size = 2;
    burst_size = 1;

    /* assert NPG to drive low (deasserted) when pass-thru disabled */
    safe_cpld_assert(CV_ASSERT_NPG);

    while (len > 0) {

        /* disable NPG pass-thru w/cpld */
        safe_cpld_write(CPLD_REG_PASS_THRU, ~CV_PASS_NPG);

        /* assert NPR */
        safe_cpld_assert(CV_ASSERT_NPR);

        /* wait for NPG */
        while ((safe_cpld_read(CPLD_REG_STATUS) & CV_BUS_NPG))
            ;

        /* assert SACK */
        safe_cpld_assert(CV_ASSERT_SACK);

        disable_cpld_int();

        /* deassert NPR */
        cpld_deassert(CV_ASSERT_NPR);

        /* wait for NPG to deassert */
        while (!(cpld_read(CPLD_REG_STATUS) & CV_BUS_NPG))
            ;

        /* wait for BBSY to deassert */
        while (BUS_BBSY())
            ;

        /* assert BBSY */
        cpld_assert(CV_ASSERT_BBSY);

        /* set addr */
        cpld_write(CPLD_REG_WR_ADDR_HI, addr >> 16);
        cpld_write(CPLD_REG_WR_ADDR_LO, addr);

        /* set 'dma mode' */
        cpld_write_subreg(CPLD_SUBREG_DMA_MODE, 1);

        /* do 1st read to prime */
        if (!write)
            cpld_read(CPLD_REG_DATA);

        for (burst = 0; burst < burst_size; burst++) {

            if (write) {
                /* assert data */
                cpld_write(CPLD_REG_DATA, *data++);
            }

            /* wait for data to appear or go away */
            while ((cpld_read(CPLD_REG_STATUS) & CV_M_IDLE) == 0)
                ;

	    if (!write) {
                /* read, sample data */
                *data++ = cpld_read(CPLD_REG_DATA);
            }

            len--;
            addr += 2;
        }

        if (!write) {
            /* wait for data to appear */
            while ((cpld_read(CPLD_REG_STATUS) & CV_M_IDLE) == 0)
                ;
        }

        /* enable NPG pass-thru w/cpld */
        cpld_write(CPLD_REG_PASS_THRU, ~0);

        /* this is important - cpld won't answer slave in dma mode */
        cpld_write_subreg(CPLD_SUBREG_DMA_MODE, 0);

	/* deassert SACK */
        cpld_deassert(CV_ASSERT_SACK);

        /* deassert BBSY */
        cpld_deassert(CV_ASSERT_BBSY);

        enable_cpld_int();
    }

    cpld_deassert(CV_ASSERT_NPG);
}


#if 1
#define INT_A_BR CV_ASSERT_BR5
#define INT_A_BG CV_ASSERT_BG5
#define INT_B_BG CV_BUS_BG5
#define INT_P_BG CV_PASS_BG5
#endif

#if 0
#define INT_A_BR CV_ASSERT_BR4
#define INT_A_BG CV_ASSERT_BG4
#define INT_B_BG CV_BUS_BG4
#define INT_P_BG CV_PASS_BG4
#endif

void
unibus_interrupt(void)
{
    /* assert BG* to drive low (deasserted) when pass-thru disabled */
    safe_cpld_assert(INT_A_BG);

    /* disable BRx pass-thru w/cpld */
    safe_cpld_write(CPLD_REG_PASS_THRU, ~INT_P_BG);
    
    /* assert BRx */
    safe_cpld_assert(INT_A_BR);

    /* wait for BGx */
    while ((safe_cpld_read(CPLD_REG_STATUS) & INT_B_BG))
        ;

    /* assert SACK */
    safe_cpld_assert(CV_ASSERT_SACK);

    disable_cpld_int();

    cpld_deassert(INT_A_BR);

    /* wait for BGx to go away*/
    while ((cpld_read(CPLD_REG_STATUS) & INT_B_BG) == 0)
        ;

    /* wait for BBSY to deassert */
    while (BUS_BBSY())
        ;

    /* assert BBSY */
    cpld_assert(CV_ASSERT_BBSY);

    cpld_write(CPLD_REG_DATA, vector/*0160*/);
    cpld_assert(CV_ASSERT_DATA_DIR);

    /* wait for no SSYN */
    while (BUS_SSYN())
        ;

    cpld_assert(CV_ASSERT_INTR);
    cpld_deassert(CV_ASSERT_SACK);

    /* wait for SSYN */
    while (!BUS_SSYN())
        ;

    cpld_deassert(CV_ASSERT_DATA_DIR | CV_ASSERT_INTR | CV_ASSERT_BBSY);

    cpld_write(CPLD_REG_PASS_THRU, ~0);
    cpld_deassert(INT_A_BG);

gen_ints++;

    enable_cpld_int();
}


#if 0
void
unibus_interrupt(void)
{
    volatile AT91PS_PIO pPio = AT91C_BASE_PIOA;

#if 1
#define CPLD_START

#define CPLD_START_WRITE \
        pPio->PIO_OER = BUS; pPio->PIO_OWER = BUS; /* drive */

#define CPLD_START_READ \
	pPio->PIO_ODR = BUS; /* passive */

#define CPLD_WRITE(reg, data) \
	pPio->PIO_SODR = (data & 0xffff) | ((reg) << 17); \
	pPio->PIO_SODR = P_CPLD_WR; \
	asm volatile("nop"); asm volatile("nop"); \
	pPio->PIO_CODR = P_CPLD_WR; \
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);

#define CPLD_READ(reg, result) \
	pPio->PIO_SODR = (reg) << 17;   \
	pPio->PIO_SODR = P_CPLD_RD; \
	result = pPio->PIO_PDSR; \
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0) | P_CPLD_RD;

#define CPLD_DONE \
	pPio->PIO_ODR = BUS; /* passive */ \
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);
#else
#define CPLD_START
#define CPLD_START_WRITE
#define CPLD_START_READ
#define CPLD_WRITE(reg, data) cpld_write(reg, data)
#define CPLD_READ(reg, result) result = cpld_read(reg)
#define CPLD_DONE
#endif

    CPLD_START;

#define A_BG	CV_ASSERT_BG5
#define A_BR	CV_ASSERT_BR5
#define P_BG	CV_PASS_BG5

#define A_SACK	CV_ASSERT_SACK
#define A_BBSY	CV_ASSERT_BBSY
#define A_DATA	CV_ASSERT_DATA_DIR
#define A_INTR	CV_ASSERT_INTR

    CPLD_START_WRITE;

    /* assert BG* to drive low (deasserted) when pass-thru disabled */
    CPLD_WRITE(CPLD_REG_ASSERT, A_BG);

    /* disable BRx pass-thru w/cpld */
    CPLD_WRITE(CPLD_REG_PASS_THRU, ~P_BG);
    
    /* assert BRx */
    CPLD_WRITE(CPLD_REG_ASSERT, A_BG | A_BR);

#if 1
    CPLD_WRITE(CPLD_REG_SUBREG, (CPLD_SUBREG_DMA_MODE << 8) | (u_char)1);
#endif

    DISABLE_CPLD_INT;

    /* wait for BGx */
    int loops;
    CPLD_START_READ;
    for (loops = 0; loops < 1000; loops++) {
        unsigned short data;
	CPLD_READ(CPLD_REG_STATUS, data);
	if  (data & CV_BUS_BG5)
	    ;
        else
	    break;
    }

//    DISABLE_CPLD_INT;

#if 0
    /* wait for client state machine to be idle */
    unsigned short sigs;
    do {
	    CPLD_READ(CPLD_REG_STATUS, sigs);
    } while ((sigs & (7 << 8)) != 0);
#else
    unsigned short sigs;
//    int i;
//    for (i = 0; i < 2; i++) {
//	    CPLD_READ(CPLD_REG_STATUS, sigs);
//	    if ((sigs & (7 << 8)) == 0) break;
//    } 
    CPLD_READ(CPLD_REG_STATUS, sigs);
//    CPLD_READ(CPLD_REG_STATUS, sigs);
//    CPLD_READ(CPLD_REG_STATUS, sigs);
//    CPLD_READ(CPLD_REG_STATUS, sigs);
#endif

    CPLD_START_WRITE;

    /* assert SACK */
    CPLD_WRITE(CPLD_REG_ASSERT, A_BG | A_BR | A_SACK);
    CPLD_WRITE(CPLD_REG_ASSERT,	A_BG        | A_SACK);

    /* wait for BGx to go away*/
    int loops2;
    CPLD_START_READ;
    for (loops2 = 0; loops2 < 300/*1000*/; loops2++) {
        unsigned short data;
	CPLD_READ(CPLD_REG_STATUS, data);
	if ((data & CV_BUS_BG5) == 0)
	    break;
    }

//    DISABLE_CPLD_INT;

    CPLD_START_WRITE;

    CPLD_WRITE(CPLD_REG_DATA, vector);
//    CPLD_WRITE(CPLD_REG_DATA, 0160);

    CPLD_WRITE(CPLD_REG_ASSERT, A_BG | A_SACK | A_BBSY | A_DATA);
    CPLD_WRITE(CPLD_REG_ASSERT, A_BG | A_SACK | A_BBSY | A_DATA | A_INTR);
    CPLD_WRITE(CPLD_REG_ASSERT, A_BG          | A_BBSY | A_DATA | A_INTR);
    CPLD_WRITE(CPLD_REG_ASSERT,	A_BG);

    CPLD_WRITE(CPLD_REG_PASS_THRU, ~0);
    CPLD_WRITE(CPLD_REG_ASSERT, 0);
    
#if 1
    CPLD_WRITE(CPLD_REG_SUBREG, (CPLD_SUBREG_DMA_MODE << 8) | (u_char)0);
#endif
    CPLD_DONE;

gen_ints++;

    ENABLE_CPLD_INT;
}
#endif


