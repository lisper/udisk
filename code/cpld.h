/*
 * cpld.h
 * $Id: cpld.h 70 2013-11-17 17:59:13Z brad $
 */

#ifndef _CPLD_H_
#define _CPLD_H_

/* CPLD registers */

#define CPLD_REG_PASS_THRU	0	/* write */

/* pass thru enable bits (1=pass,0=use-assert-bit) */
#define CV_PASS_BG4	0x02
#define CV_PASS_BG5	0x04
#define CV_PASS_NPG	0x08

#define CPLD_REG_ASSERT		1	/* write */
#define CPLD_REG_STATUS		1	/* read */

/* assert bits */
#define CV_ASSERT_INTR	0x0001
#define CV_ASSERT_BR4	0x0002
#define CV_ASSERT_BR5	0x0004
#define CV_ASSERT_NPR	0x0008
#define CV_ASSERT_BG4	0x0010
#define CV_ASSERT_BG5	0x0020
#define CV_ASSERT_NPG	0x0040
#define CV_ASSERT_MSYN	0x0100
#define CV_ASSERT_SSYN	0x0200
#define CV_ASSERT_BBSY	0x0400
#define CV_ASSERT_C0	0x0800
#define CV_ASSERT_C1	0x1000
#define CV_ASSERT_SACK	0x2000
#define CV_ASSERT_DATA_DIR 0x4000
#define CV_ASSERT_ADDR_DIR 0x8000

#define CPLD_REG_MADDR1		2	/* write */
#define CPLD_REG_MASK1		3	/* write */

#define CPLD_REG_RD_ADDR_HI	2	/* write */
#define CPLD_REG_RD_ADDR_LO	3	/* write */

#define CPLD_REG_WR_ADDR_HI	4	/* write */
#define CPLD_REG_WR_ADDR_LO	5	/* write */

#define CPLD_REG_ID		7	/* read */
#define CPLD_REG_SUBREG		7	/* write */

#define CPLD_SUBREG_LED		0x02
#define CPLD_SUBREG_CF_ENA	0x03
#define CPLD_SUBREG_DMA_MODE	0x04
#define CPLD_SUBREG_CPU_RELEASE	0x05
#define CPLD_SUBREG_INT_RESET	0x01
#define CPLD_SUBREG_ID		0x00

//#define CPLD_SUBREG_MASK1	0x06
#define CPLD_SUBREG_MASK2	0x07

#define CPLD_REG_DATA		6	/* r/w */

/* SUBREG_CF_ENA */
#define CF_ENA_0	0x01
#define CF_ENA_1	0x02
#define CF_RESET	0x04

/* REG_STATUS - bus signal bits */
#define CV_BUS_INIT	0000001
#define CV_BUS_NPG	0000002
#define CV_BUS_BG5	0000004
#define CV_BUS_BG4	0000010
#define CV_BUS_SACK	0000020
#define CV_BUS_C0	0000040
#define CV_BUS_C1	0000100
#define CV_BUS_BBSY	0000200

#define CV_S_STATE	0003400
#define CV_M_WAIT	0004000
#define CV_M_IDLE	0010000

/* REG_STATUS - match state bits */
#define CV_MATCH_1	0020000
#define CV_MATCH_2	0040000
#define CV_INT		0100000

#define cpld_read bus_read
#define cpld_read_long bus_read_long
#define cpld_write bus_write
#define cpld_write_long bus_write_long

extern u_short cpld_asserting;

static void inline
cpld_assert(int new)
{
    cpld_asserting |= new;
    cpld_write(CPLD_REG_ASSERT, cpld_asserting);
}

static void inline
cpld_deassert(int new)
{
    cpld_asserting &= ~new;
    cpld_write(CPLD_REG_ASSERT, cpld_asserting);
}

static void inline
cpld_assert_deassert(int new)
{
    bus_write2(CPLD_REG_ASSERT, cpld_asserting | new, cpld_asserting);
}

extern int ide_state;

#if 0
/* used by cpld irq to save/restore ide state */
static void inline
ide_state_save(int *pstate)
{
    *pstate = ide_state;
    ide_state = 0;
    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);
}

static void inline
ide_state_restore(int state)
{
    if (state) {
        ide_state = state;
        cpld_write_subreg(CPLD_SUBREG_CF_ENA, ide_state);
    }
}
#endif

extern void cpld_isr(void);

//#define DISABLE_CPLD_INT	*AT91C_PIOA_IDR = AT91C_PIO_PA25
//#define ENABLE_CPLD_INT	*AT91C_PIOA_IER = AT91C_PIO_PA25
//#define DISABLE_CPLD_INT	*AT91C_AIC_IDCR = 1 << AT91C_ID_PIOA
//#define ENABLE_CPLD_INT		*AT91C_AIC_IECR = 1 << AT91C_ID_PIOA

#include "irq.h"
#define DISABLE_CPLD_INT	DISABLE_INTERRUPTS()
#define ENABLE_CPLD_INT		ENABLE_INTERRUPTS()


static void inline
disable_cpld_int(void)
{
#if 0
    *AT91C_PIOA_IDR = AT91C_PIO_PA25;

    if ((*AT91C_PIOA_ISR) & AT91C_PIO_PA25)
	    cpld_isr();
#else
    DISABLE_CPLD_INT;
#endif
}

static void inline
enable_cpld_int(void)
{
#if 0
    if ((*AT91C_PIOA_ISR) & AT91C_PIO_PA25)
	    cpld_isr();

    *AT91C_PIOA_IER = AT91C_PIO_PA25;
#else
    ENABLE_CPLD_INT;
#endif
}


static inline volatile void
cpld_write_subreg(int subreg, int data)
{
    bus_write_long(CPLD_REG_SUBREG, (subreg << 8) | (u_char)data);
}

// this is a hack - why does write_long turn off ints?
static inline volatile void
cpld_write_sub(int subreg, int data)
{
    bus_write(CPLD_REG_SUBREG, (subreg << 8) | (u_char)data);
}

// ------------------------

static void inline
safe_cpld_assert(int new)
{
    disable_cpld_int();
    cpld_assert(new);
    enable_cpld_int();
}

static void inline
safe_cpld_deassert(int new)
{
    disable_cpld_int();
    cpld_deassert(new);
    enable_cpld_int();
}

static int inline
safe_cpld_read(int reg)
{
    int ret;
    disable_cpld_int();
    ret = cpld_read(reg);
    enable_cpld_int();
    return ret;
}

static void inline
safe_cpld_write(int reg, int data)
{
    disable_cpld_int();
    cpld_write(reg, data);
    enable_cpld_int();
}

static inline volatile void
safe_cpld_write_subreg(int subreg, int data)
{
    disable_cpld_int();
    bus_write_long(CPLD_REG_SUBREG, (subreg << 8) | (u_char)data);
    enable_cpld_int();
}

#endif /* _CPLD_H_ */
