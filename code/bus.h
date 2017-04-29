/*
 * bus.h
 * $Id: bus.h 70 2013-11-17 17:59:13Z brad $
 */

#ifndef _BUS_H_
#define _BUS_H_

#define BUS (P_D15 | P_D14 | P_D13 | P_D12 | P_D11 | P_D10 | P_D9 | P_D8 | \
             P_D7 | P_D6 | P_D5 | P_D4 | P_D3 | P_D2 | P_D1 | P_D0)


#define BUS_MSYN()	((*AT91C_PIOA_PDSR) & AT91C_PIO_PA27)
#define BUS_SSYN()	((*AT91C_PIOA_PDSR) & AT91C_PIO_PA28)
#define BUS_BBSY()	((*AT91C_PIOA_PDSR) & AT91C_PIO_PA29)
#define BUS_SSYN_OR_BBSY() \
			((*AT91C_PIOA_PDSR) & (AT91C_PIO_PA29|AT91C_PIO_PA28))

static inline void
bus_drive(void)
{
#if 0
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	/* enable bus pins */
        pPio->PIO_OER = BUS;
        pPio->PIO_OWER = BUS;
#else
	*AT91C_PIOA_OER = BUS;
	*AT91C_PIOA_OWER = BUS;
#endif
}

static inline void
bus_passive(void)
{
#if 0
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	/* disable bus pins */
        pPio->PIO_ODR = BUS;
#else
	/* disable bus pins */
	*AT91C_PIOA_ODR = BUS;
#endif
}

/******************************************************************************
* function:    int bus_read(int reg)
*
* description: read from the CPLD using the pio "bus"
*
* passed:      none
*
* returns:     none
*
******************************************************************************/

static inline volatile int
bus_read(int reg)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	int data;

	/* assume bus is passive */

        /* set address */
	pPio->PIO_SODR = reg << 17;

	/* assert rd */
	pPio->PIO_SODR = P_CPLD_RD;
	data = pPio->PIO_PDSR;
	pPio->PIO_CODR = P_CPLD_RD;

	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);

	return data & 0xffff;
}

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

#if 0
#define DISABLE_INTERRUPTS() \
	asm volatile("mrs r12, cpsr\n\t" \
		     "orr r12, r12, #0xC0\n\t" \
		     "msr cpsr_c, r12\n\t" ::: "r12", "cc");
#define ENABLE_INTERRUPTS() \
	asm volatile("mrs r12, cpsr\n" \
		     "bic r12, r12, #0xC0\n" \
		     "msr cpsr_c, r12" ::: "r12", "cc");
#endif

#define DI(v) \
	asm volatile("mrs %[reg], cpsr\n\t" \
		     "orr %[reg], %[reg], #0xC0\n\t" \
		     "msr cpsr_c, %[reg]\n\t" : [reg] "=r" (v) :: "cc");
#define EI(v) \
	asm volatile("mrs %[reg], cpsr\n" \
		     "bic %[reg], %[reg], #0xC0\n" \
		     "msr cpsr_c, %[reg]" : [reg] "=r" (v) :: "cc");

//
// slow read - used for IDE reads
//
static inline volatile int
bus_read_long(int reg)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	int data;
	u32 scratch;

	/* assume bus is passive */
DI(scratch);
//DISABLE_INTERRUPTS();

        /* set address */
	pPio->PIO_SODR = reg << 17;
pPio->PIO_SODR = reg << 17;
pPio->PIO_SODR = reg << 17;
pPio->PIO_SODR = reg << 17;

	/* assert rd */
	pPio->PIO_SODR = P_CPLD_RD;
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
asm volatile("nop");
asm volatile("nop");
asm volatile("nop");
asm volatile("nop");

	data = pPio->PIO_PDSR;

	pPio->PIO_CODR = P_CPLD_RD;
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);

//ENABLE_INTERRUPTS();
EI(scratch);

	return data & 0xffff;
}

/******************************************************************************
* function:    void bus_write(int reg, int data)
*
* description: write to CPLD using the pio "bus"
*
* passed:      none
*
* returns:     none
*
******************************************************************************/

static inline volatile void
bus_write(int reg, int data)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;

	/* assume bus is passive */

	pPio->PIO_SODR = (data & 0xffff) | (reg << 17);
	bus_drive();

	pPio->PIO_SODR = P_CPLD_WR;
	asm volatile("nop");

	pPio->PIO_CODR = P_CPLD_WR;
	asm volatile("nop");

	bus_passive();
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);
}

static inline volatile void
bus_write_long(int reg, int data)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	u32 scratch;

//DISABLE_INTERRUPTS();
DI(scratch);

pPio->PIO_SODR = (data & 0xffff) | (reg << 17);
pPio->PIO_SODR = (data & 0xffff) | (reg << 17);
pPio->PIO_SODR = (data & 0xffff) | (reg << 17);
pPio->PIO_SODR = (data & 0xffff) | (reg << 17);

	/* assume bus is passive */
	bus_drive();

	pPio->PIO_SODR = (data & 0xffff) | (reg << 17);
	asm volatile("nop");
	asm volatile("nop");

	pPio->PIO_SODR = P_CPLD_WR;
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	pPio->PIO_CODR = P_CPLD_WR;
	asm volatile("nop");
	asm volatile("nop");

	bus_passive();
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);

//ENABLE_INTERRUPTS();
EI(scratch);
}

#undef DISABLE_INTERRUPTS
#undef ENABLE_INTERRUPTS

static inline volatile void
bus_write2(int reg, int data, int data2)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;

	/* assume bus is passive */
	bus_drive();

	pPio->PIO_SODR = (data & 0xffff) | (reg << 17);
	pPio->PIO_SODR = P_CPLD_WR;
	asm volatile("nop");

	pPio->PIO_CODR = P_CPLD_WR;
	asm volatile("nop");

	pPio->PIO_CODR = BUS;
	pPio->PIO_SODR = (data2 & 0xffff) | (reg << 17);
	pPio->PIO_SODR = P_CPLD_WR;
	asm volatile("nop");

	pPio->PIO_CODR = P_CPLD_WR;
	asm volatile("nop");

	bus_passive();
	pPio->PIO_CODR = BUS | (P_A3 | P_A2 | P_A1 | P_A0);
}

#endif /* _BUS_H_ */
