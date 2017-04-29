/*
 * main.c
 * $Id: main.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "bus.h"
#include "cpld.h"
#include "rl11.h"

#define waitns(n)

u_short cpld_asserting;


unsigned long cpld_ints;
int writes_cs;
int reads_cs;
int rmws;
int dma_writes;
int dma_reads;
int gen_ints;
int missed_ints;
int bad_match;
int npg_timeout;
int npg_again;

void reset_stats()
{
    cpld_ints = 0;
    writes_cs = 0;
    reads_cs = 0;
    rmws = 0;
    dma_writes = 0;
    dma_reads = 0;
    gen_ints = 0;
    missed_ints = 0;
    bad_match = 0;
    npg_timeout = 0;
    npg_again = 0;
}

void show_stats()
{
    printf("cpld ints %d\n", cpld_ints);
    printf("missed ints %d\n", missed_ints);
    printf("cs writes %d\n", writes_cs);
    printf("cs reads %d\n", reads_cs);
    printf("r-m-w %d\n", rmws);
    printf("dma writes %d\n", dma_writes);
    printf("dma reads %d\n", dma_reads);
    printf("gen ints %d\n", gen_ints);
    printf("bad match %d\n", bad_match);
    printf("npg_timeout %d\n", npg_timeout);
    printf("npg_again %d\n", npg_again);

    controller_print_state();
}

/*static*/ u_int32 log[64][3];
/*static*/ int log_p;

#if 0
void
_logw(int reg, int v1, int v2)
{
    log[log_p][0] = reg;
    log[log_p][1] = v1;
    log[log_p][2] = v2;
    if (log_p < 63)
        log_p++;
    else
        log_p = 0;
}
#endif

void
log_show(void)
{
    int i;
    printf("log_p %d\n", log_p);
    for (i = 0; i < 64; i++) {
        if (log[i][0] < 0xff) {
            printf("%d: %d %o %o\n", i, log[i][0], log[i][1], log[i][2]);
        }
    }
}

void
log_reset(void)
{
    int i;
    log_p = 0;
    for (i = 0; i < 64; i++)
        log[i][0] = 0xff;
}

/*
 * assert an interrupt on the UNIBUS
 */

u_short vector;


#if 0
/* test code */
void
cpld_isr(void)
{
    AT91PS_PIO pPio;
    unsigned short sigs, data, addr_lo, addr_hi;
    int saved_ide_state;
    volatile int d;

    pPio = AT91C_BASE_PIOA;
    cpld_ints++;

    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
    *AT91C_AIC_ICCR = 0xffffffff;

    {
        volatile u32 v;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
    }

    /* sample C0,C1 */
    sigs = cpld_read(CPLD_REG_STATUS);

    /* low bits of matching address */
    addr_lo = cpld_read(CPLD_REG_RD_ADDR_LO);

    if (sigs & CV_BUS_C1) {
    } else {
        /* read */
        data = 01230 | (addr_lo & 016);
        cpld_write(CPLD_REG_DATA, data);
    }

    cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);
}
#endif


void
setup_cpld_addr(int addr, int mask)
{
    /* address bits [17:2] */
    cpld_write(CPLD_REG_MADDR1, addr >> 2);

    /* 8 bit mask for address bits [9:2] */
    cpld_write(CPLD_REG_MASK1, mask >> 2);

//    /* don't use 2nd matcher */
//    cpld_write(CPLD_REG_MADDR2, 1);
//    cpld_write_subreg(CPLD_SUBREG_MASK2, 0);
}

#define CPLD_INTERRUPT_LEVEL	1
#define INIT_INTERRUPT_LEVEL	5

void
setup_cpld_intr()
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    cpld_ints = 0;

    /* pa25 */
    irq_configure(AT91C_ID_PIOA,
                  CPLD_INTERRUPT_LEVEL,
                  AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED |
                  AT91C_AIC_SRCTYPE_EXT_POSITIVE_EDGE,
                  cpld_isr);

    pio_pins_in(AT91C_PIO_PA25);

    pPio->PIO_IER |= AT91C_PIO_PA25;

    irq_enable(AT91C_ID_PIOA);

#if 0
    /* pa26 */
    irq_configure(AT91C_ID_PIOA,
                  INIT_INTERRUPT_LEVEL,
                  AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED |
                  AT91C_AIC_SRCTYPE_EXT_POSITIVE_EDGE,
                  init_isr);
    pPio->PIO_IER |= AT91C_PIO_PA26;
#endif
}

void
reboot(void)
{
    AT91PS_RSTC pRSTC = AT91C_BASE_RSTC;

    disable_interrupts();
    force_mapping_to_rom();

    /* reset */
    pRSTC->RSTC_RCR = 0xA5000000 |
        (AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST);
}

void
run_updater(void)
{
    reboot();
}



void
resetup(void)
{
    controller_setup();

    cpld_reset();

    /* enable all pass-thrus w/cpld */
    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);
    cpld_write(CPLD_REG_PASS_THRU, ~0);
    cpld_write(CPLD_REG_ASSERT, 0);
    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

    cpld_deassert(CV_ASSERT_BG5 | CV_ASSERT_BG4 | CV_ASSERT_NPG);
}

main()
{
    reset_stats();

    cpld_asserting = 0;

    debug_setup();
    usart_setup();

#if 0
    while (1) {
        putc('A');
    }
#endif

    puts("\nudisk code3 cpld3 v0.3\n");

    irq_setup();
    bus_setup();
    cli_setup();

    /* disk */
    ide_setup();
    disk_setup();

    /* controller */
    controller_setup();

    /* enable all pass-thrus w/cpld */
    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);
    cpld_write(CPLD_REG_PASS_THRU, ~0);
    cpld_write(CPLD_REG_ASSERT, 0);
    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

    cpld_deassert(CV_ASSERT_BG5 | CV_ASSERT_BG4 | CV_ASSERT_NPG);

    controller_online();

    /* turn on interrupts */
    enable_interrupts();

    while (1) {
        cli_poll();
        controller_poll();
    }
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
