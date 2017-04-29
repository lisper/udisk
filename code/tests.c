/*
 * tests.c
 *
 * $Id: tests.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "bus.h"
#include "cpld.h"

u_short buffer1[256];
u_short buffer2[256];

#define BUS_MSYN()	((*AT91C_PIOA_PDSR) & AT91C_PIO_PA27)
#define BUS_SSYN()	((*AT91C_PIOA_PDSR) & AT91C_PIO_PA28)
#define BUS_BBSY()	((*AT91C_PIOA_PDSR) & AT91C_PIO_PA29)

#define BUS_SSYN_OR_BBSY() \
			((*AT91C_PIOA_PDSR) & (AT91C_PIO_PA29|AT91C_PIO_PA28))

void bus_show(void);

static void d(void)
{
	volatile long l;
	for (l = 0; l < 300000; l++)
		;
}

void
show_cpld_regs(void)
{
    int id, reg;

    id = cpld_read(CPLD_REG_ID);
    printf("cpld id %x\n", id);

    for (reg = 0; reg < 16; reg++) {
        id = cpld_read(reg);
        printf("cpld %d = %x\n", reg, id);
    }
}

void
test_cpld_io(void)
{
    unsigned short n, nn, n1, n2, n3;

    disable_interrupts();
    n = 0;
    while (1) {
        n++;

        cpld_write(CPLD_REG_DATA, n);
        n1 = cpld_read(CPLD_REG_DATA);
        n2 = cpld_read(CPLD_REG_DATA);
        n3 = cpld_read(CPLD_REG_DATA);
        if (n != n1 || n != n2 || n != n3) {
            printf("data mismatch 0x%x, 0x%x 0x%x 0x%x\n", n, n1, n2, n3);
        }

        nn = ~n;
        cpld_write(CPLD_REG_DATA, nn);
        n1 = cpld_read(CPLD_REG_DATA);
        n2 = cpld_read(CPLD_REG_DATA);
        n3 = cpld_read(CPLD_REG_DATA);
        if (nn != n1 || nn != n2 || nn != n3) {
            printf("data mismatch 0x%x, 0x%x 0x%x 0x%x\n", n, n1, n2, n3);
        }

        if (usart_getc_rdy())
            break;
        if (n == 0xffff) {
            printf("done\n");
            break;
        }
    }
    enable_interrupts();
}

void
test_led_wiggle(void)
{
    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);

    disable_interrupts();
    while (1) {
        cpld_write_subreg(CPLD_SUBREG_LED, 1);
        d();
        cpld_write_subreg(CPLD_SUBREG_LED, 0);
        d();

        if (usart_getc_rdy())
            break;
    }
    enable_interrupts();
}

void
test_led_set(int set)
{
    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);

    if (set)
        cpld_write_subreg(CPLD_SUBREG_LED, 0);
    if (!set)
        cpld_write_subreg(CPLD_SUBREG_LED, 1);
}

void
test_dma(void)
{
    int bn, i, addr, diffs, diff, blocks, size;

    blocks = 256;
    size = 128;
//    blocks = 64;

    printf("fill %d blocks\n", blocks);

    printf("zero\n");
    addr = 0;
    for (bn = 0; bn < blocks; bn++) {
        for (i = 0; i < size; i++) 
            buffer1[i] = 0;

        unibus_dma_buffer(1, addr, buffer1, size);
        addr += 256;
    }


    printf("fill\n");
    addr = 0;
    for (bn = 0; bn < blocks; bn++) {
        for (i = 0; i < size; i++) 
            buffer1[i] = (bn << 8) | i;

        unibus_dma_buffer(1, addr, buffer1, size);
        addr += 256;
    }

    printf("check\n");
    addr = 0;

    for (bn = 0; bn < blocks; bn++) {
        for (i = 0; i < size; i++) 
            buffer1[i] = (bn << 8) | i;

        for (i = 0; i < size; i++) 
            buffer2[i] = 0;

        unibus_dma_buffer(0, addr, buffer2, size);

        diffs = 0;
        diff = 0;
        for (i = 0; i < size; i++) {
            if (buffer1[i] != buffer2[i]) {
                if (diffs == 0) diff = i;
                diffs++;
            }
        }

        if (diffs) {
            int start;

            printf("data does not compare; bn %d, addr %o\n",
                   bn, addr);
            printf("differences %d, first bad offset %d\n",
                   diffs, diff);

            start = (diff/4)*4;
            for (i = start-2; i < start+4; i++) {
                printf("%d wrote %4x read %4x\n",
                       i, buffer1[i], buffer2[i]);
            }
        }

        addr += 256;
    }
}

#define MAX_LOOPS	100000
#define waitns(n)

void
test_dma_rw(int write, int addr, short *buff, int len)
{
    //printf("calling unibus_dma_buffer()\n");
    unibus_dma_buffer(write, addr, buff, len);
    //printf("back from unibus_dma_buffer()\n");
}

void
test_dma_wr(int addr, short *buff, int size)
{
    test_dma_rw(1, addr, buff, size);
}

void
test_dma_rd(int addr, short *buff, int size)
{
    test_dma_rw(0, addr, buff, size);
}

void
test_dma_simple(void)
{
    u_short off, v;
    int i, bad;
    
    printf("test_dma_simple\n");

    for (i = 0; i < 256; i++) {
        v = i;
        buffer1[i] = v;
    }

    printf("write ");
    test_dma_wr(0, buffer1, 256);

    for (i = 0; i < 256; i++)
        buffer2[i] = 0xffff;

    printf("read ");
    test_dma_rd(0, buffer2, 256);

    printf("compare\n");
    bad = 0;
    for (i = 0; i < 256; i++) {
        if (buffer1[i] != buffer2[i]) {
            if (bad == 0) {
                printf("r-w mismatch @ %o, wrote %o read %o\n",
                       i*2, buffer1[i], buffer2[i]);
            }
            bad++;
            break;
        }
    }

//---
    printf("read2 ");
    test_dma_rd(0, buffer2, 256);

    printf("compare2\n");
    bad = 0;
    for (i = 0; i < 256; i++) {
        if (buffer1[i] != buffer2[i]) {
            if (bad == 0) {
                printf("r-w mismatch @ %o, wrote %o read %o\n",
                       i*2, buffer1[i], buffer2[i]);
            }
            break;
        }
    }

//---
    if (bad) {
        printf("mismatches %d\n", bad);

        for (i = 0; i < 8; i++)
            printf("%2o: %6o %6o\n", i*2, buffer1[i], buffer2[i]);
    }

    printf("done\n");
}

void
test_dma_read(void)
{
    u_short off, v;
    int i, bad;

    printf("test_dma_read\n");

    for (i = 0; i < 256; i++)
        buffer2[i] = 0xffff;

    printf("read\n");
    test_dma_rd(0, buffer2, 256);

    for (i = 0; i < 8; i++)
        printf("%2o: %6o\n", i*2, buffer2[i]);

    printf("done\n");
}

void
test_dma_fill(void)
{
    u_short off, v;
    int errs;
    
    printf("test_dma_fill\n");

    off = 0;
    errs = 0;
    while (1) {
        int i, bad;

        if (usart_getc_rdy())
            break;

        printf("%5d %5o ", errs, off);

        for (i = 0; i < 256; i++) {
            v = (i + off) & 0xff;
            v = (v << 8) | v;
            buffer1[i] = v;
        }

        printf("write ");
        test_dma_wr(0, buffer1, 256);

        for (i = 0; i < 256; i++)
            buffer2[i] = 0xffff;

        printf("read ");
        test_dma_rd(0, buffer2, 256);

        printf("compare\n");
        bad = 0;
        for (i = 0; i < 256; i++) {
            if (buffer1[i] != buffer2[i]) {
                if (bad == 0) {
                    printf("r-w mismatch @ %o, wrote %o read %o\n",
                           i*2, buffer1[i], buffer2[i]);
                }
                bad++;
//                break;
            }
        }

        if (bad) {
            printf("mismatches %d\n", bad);
            errs++;
        }

        if (bad) {
            printf("re-read ");
            test_dma_rd(0, buffer2, 256);

            printf("compare-again ");
            bad = 0;
            for (i = 0; i < 256; i++) {
                if (buffer1[i] != buffer2[i]) {
                    if (bad == 0) {
                        printf("r-w mismatch @ %o, wrote %o read %o\n",
                               i*2, buffer1[i], buffer2[i]);
                    }
                    bad++;
                }
            }

            if (bad) {
                printf("again-mismatches %d\n", bad);
                errs++;
            } else
                printf("ok\n");
        }

        off++;
    }

}

void
bus_show(void)
{
    unsigned int m, s, b;
    unsigned int ah, al, a, intr, bus_sig;

    ah = cpld_read(CPLD_REG_RD_ADDR_HI) & 3;
    al = cpld_read(CPLD_REG_RD_ADDR_LO);
    bus_sig = cpld_read(CPLD_REG_STATUS);
    s = BUS_SSYN();
    m = BUS_MSYN();
    b = BUS_BBSY();

    a = (ah << 16) | al;
    printf("match addr %o, bus_sig %x\n", a, bus_sig);
    if (bus_sig & CV_BUS_INIT) printf("INIT ");
    if (bus_sig & CV_BUS_NPG	) printf("NPG ");
    if (bus_sig & CV_BUS_BG5	) printf("BG5 ");
    if (bus_sig & CV_BUS_BG4	) printf("BG4 ");
    if (bus_sig & CV_BUS_SACK	) printf("SACK ");
    if (bus_sig & CV_BUS_C0	) printf("C0 ");
    if (bus_sig & CV_BUS_C1	) printf("C1 ");
    if (bus_sig & CV_BUS_BBSY	) printf("BBSY ");
    printf("\n");

    if (s) printf("SSYN ");
    if (m) printf("MSYN ");
    if (b) printf("BBSY ");
    printf("\n");

    intr = cpld_read(CPLD_REG_STATUS) & CV_INT;
    if (intr) {
        printf("int set\n");
    }

#if 0
    int ms, bah, bal, ba;
    ms = cpld_read(12);
    bah = cpld_read(13);
    bal = cpld_read(14);
    ba = (bah << 16) | bal;
    printf("bus addr %x, match state %x\n", ba, ms);
#else
    unsigned int status, ao, state;
    status = cpld_read(CPLD_REG_STATUS);
    printf("status %x; ", status);
    if (status & CV_INT) printf("INT ");
    printf("\n");

    printf("slave state %x\n", (status >> 8) & 0x7);

    state = cpld_read(CPLD_REG_RD_ADDR_HI) >> 2;
    printf("master state %x\n", state);
    printf("(%x)\n", cpld_read(CPLD_REG_RD_ADDR_HI));

    printf("IPR %x, PIOA_ISR %x\n",
           *AT91C_AIC_IPR, *AT91C_PIOA_ISR);

    ah = cpld_read(8);
    al = cpld_read(9);
    ao = (ah << 16) | al;
    printf("addr_out %o\n", ao);
#endif
}

void
show_bus_signals(int t)
{
    if (t & CV_BUS_INIT) printf("INIT ");
    if (t & CV_BUS_NPG	) printf("NPG ");
    if (t & CV_BUS_BG5	) printf("BG5 ");
    if (t & CV_BUS_BG4	) printf("BG4 ");
    if (t & CV_BUS_SACK	) printf("SACK ");
    if (t & CV_BUS_C0	) printf("C0 ");
    if (t & CV_BUS_C1	) printf("C1 ");
    if (t & CV_BUS_BBSY	) printf("BBSY ");
}

void
bus_listen(void)
{
    int addr, mask, match;
    int ssyn, msyn, bbsy, bus;

    printf("listing to bus\n");

    disable_interrupts();

//    addr = ~0;
    addr = 0;
    mask = 0;
//addr = 017774400;
//mask = 017777700;

addr = 017777777;
mask = 017777777;

    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);

    /* address bits [17:2] */
    cpld_write(CPLD_REG_MADDR1, addr >> 2);

    /* 8 bit mask for address bits [9:2] */
    cpld_write(CPLD_REG_MASK1, mask >> 2);

//    /* don't use 2nd matcher */
//    cpld_write(CPLD_REG_MADDR2, 1);
//    cpld_write_subreg(CPLD_SUBREG_MASK2, 0);

//setup_cpld_addr(017774400, 017777700);
//setup_cpld_addr(017774400, 0);

    ssyn = 0;
    msyn = 0;
    bbsy = 0;
    bus = 0;

    while (1) {
        int t, intr;

#if 1
        cpld_write(CPLD_REG_MADDR1, addr >> 2);
        cpld_write(CPLD_REG_MASK1, mask >> 2);
#endif

        if (usart_getc_rdy())
            break;

        t = BUS_SSYN() ? 1 : 0;
        if (t != ssyn) {
            if (t) printf("SSYN+ "); else printf("SSYN- ");
            ssyn = t;
        }

        t = BUS_MSYN() ? 1 : 0;
        if (t != msyn) {
            if (t) {
                int data1, data2, bs;
                unsigned int ms, bah, bal, ba;
                bal = cpld_read(CPLD_REG_RD_ADDR_LO);
                bah = cpld_read(CPLD_REG_RD_ADDR_HI);
                ms = cpld_read(12);

                data1 = cpld_read(CPLD_REG_DATA);
                data2 = cpld_read(CPLD_REG_DATA);
                bs = cpld_read(CPLD_REG_STATUS);

                ba = (bah << 16) | bal;
//                printf("(%x,%x) ba %x, ms %x ", bah, bal, ba, ms);
                printf("ba %x, ms %x, data %6o %6o\n", ba, ms,
                       data1 & 0xffff, data2 & 0xffff);
                show_bus_signals(bs);
            }

            if (t) printf("MSYN+ "); else printf("MSYN-\n");
            msyn = t;
        }

        t = BUS_BBSY() ? 1 : 0;
        if (t != bbsy) {
            if (t) printf("BBSY+ "); else printf("BBSY- ");
            bbsy = t;
        }

        t = cpld_read(CPLD_REG_STATUS);
        if (t != bus) {
            if (t & CV_BUS_C1) {
                int data1, data2;
                data1 = cpld_read(CPLD_REG_DATA);
                data2 = cpld_read(CPLD_REG_DATA);
                printf("data %6o %6o ", data1 & 0xffff, data2 & 0xffff);
            }

            show_bus_signals(t);
            printf("\n");

            bus = t;
        }

        match = cpld_read(CPLD_REG_STATUS) & (CV_MATCH_1 | CV_MATCH_2);
        if (match) {
            int ah, al, a, bus_sig;
            printf("match\n");
            ah = cpld_read(CPLD_REG_RD_ADDR_HI) & 3;
            al = cpld_read(CPLD_REG_RD_ADDR_LO);
            bus_sig = cpld_read(CPLD_REG_STATUS);

            a = (ah << 16) | al;
            printf("match addr %o, bus_sig %x\n", a, bus_sig);

            show_bus_signals(bus_sig);
            printf("\n");
        }

        intr = cpld_read(CPLD_REG_STATUS);
        if (intr & CV_INT) {
            printf("int set; state %x, IPR %x, PIOA_ISR %x\n",
                   intr, *AT91C_AIC_IPR, *AT91C_PIOA_ISR);

            if (1) {
                int ah, al, a;
                ah = cpld_read(CPLD_REG_RD_ADDR_HI) & 3;
                al = cpld_read(CPLD_REG_RD_ADDR_LO);
                a = (ah << 16) | al;
                printf("match addr %o\n", a);
            }

            cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

            intr = cpld_read(CPLD_REG_STATUS);
            printf("int cleared; state %x, IPR %x, PIOA_ISR %x\n",
                   intr, *AT91C_AIC_IPR, *AT91C_PIOA_ISR);

            printf("state %x\n", cpld_read(CPLD_REG_STATUS));

            cpld_write(CPLD_REG_MADDR1, 0);
            cpld_write(CPLD_REG_MASK1, 0);

            printf("state %x\n", cpld_read(CPLD_REG_STATUS));
        }

    }

    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
    enable_interrupts();
}

extern unsigned long cpld_ints;

void delayme(void)
{
    volatile int i;
    for (i = 0; i < 100; i++) {
        asm volatile ("nop");
    }
}

void
bus_force(void)
{
    unsigned int addr, mask;
    unsigned long old, loops;

//#define INTS_OFF    disable_interrupts();
//#define INTS_ON     enable_interrupts();
#define INTS_OFF    DISABLE_CPLD_INT
#define INTS_ON     ENABLE_CPLD_INT

    INTS_OFF;

//    addr = ~0;
    addr = 0;
    mask = 0;
//addr = 017774400;
//mask = 017777700;

addr = 017777777;
mask = 017777777;

    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);

//    cpld_write(CPLD_REG_ADDR1, addr >> 2);
//    cpld_write(CPLD_REG_MASK1, mask >> 2);
    cpld_write(CPLD_REG_MADDR1, 0);
    cpld_write(CPLD_REG_MASK1, 0);

//    cpld_write(CPLD_REG_ADDR2, 1);
//    cpld_write(CPLD_REG_MASK2, 0);

    old = cpld_ints;
    loops = 0;

    while (1) {
        volatile int i;
        cpld_write(CPLD_REG_MADDR1, addr >> 2);
        cpld_write(CPLD_REG_MASK1, mask >> 2);

        delayme();
        INTS_ON;
        delayme();
        INTS_OFF;

        if (cpld_ints != old) {
            printf("after; ints %d\n", cpld_ints);
            old = cpld_ints;
        }

        if (usart_getc_rdy())
            break;

        cpld_write(CPLD_REG_MADDR1, 0);
        cpld_write(CPLD_REG_MASK1, 0);
        cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

*AT91C_AIC_ICCR = 0xffffffff;
cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

        delayme();
        {
            printf("state %x, IPR %x, PIOA_ISR %x\n",
                   cpld_read(CPLD_REG_STATUS), *AT91C_AIC_IPR, *AT91C_PIOA_ISR);
            printf("state %x\n", cpld_read(CPLD_REG_STATUS));
        }

        loops++;
    }

    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
    INTS_ON;

    printf("loops %d\n", loops);
}

void
bus_reset(void)
{
    cpld_asserting = 0;

    cpld_write(CPLD_REG_MADDR1, 0);
    cpld_write(CPLD_REG_MASK1, 0);

    cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);
    cpld_write(CPLD_REG_PASS_THRU, ~0);
    cpld_write(CPLD_REG_ASSERT, 0);
    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

    cpld_deassert(CV_ASSERT_BG5 | CV_ASSERT_BG4 | CV_ASSERT_NPG);

    if (cpld_asserting)
        printf("cpld_asserting %x (should be 0)\n", cpld_asserting);

#if 1
    irq_clear(AT91C_ID_PIOA);
    *AT91C_AIC_ICCR = 0xffffffff;
    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);

    resetup();
#endif
}

void
bus_wiggle(void)
{
    int val;

    bus_reset();

    val = 0xaaaa;

    while (1) {
        if (usart_getc_rdy())
            break;

        cpld_write(CPLD_REG_ASSERT, val & 0x3fff);

        val = ~val;
    }
}

void
bus_wiggle_npg(void)
{
    disable_cpld_int();
    bus_reset();

    while (1) {
        if (usart_getc_rdy())
            break;

        /* assert NPG to drive low (deasserted) when pass-thru disabled */
        cpld_assert(CV_ASSERT_NPG);

        /* disable NPG pass-thru w/cpld */
        cpld_write(CPLD_REG_PASS_THRU, ~CV_PASS_NPG);

        /* assert NPR */
        cpld_assert(CV_ASSERT_NPR);

        /* deassert NPR */
        cpld_deassert(CV_ASSERT_NPR);

        cpld_write(CPLD_REG_PASS_THRU, ~0);
        cpld_deassert(CV_ASSERT_NPG);
    }

    enable_cpld_int();
}

void
bus_wiggle_br5(void)
{
    disable_cpld_int();
    bus_reset();

    while (1) {
        if (usart_getc_rdy())
            break;

        /* assert BG5 to drive low (deasserted) when pass-thru disabled */
        cpld_assert(CV_ASSERT_BG5);

        /* disable BG5 pass-thru w/cpld */
        cpld_write(CPLD_REG_PASS_THRU, ~CV_PASS_BG5);

        /* assert BR5 */
        cpld_assert(CV_ASSERT_BR5);

        while (cpld_read(CPLD_REG_STATUS) & CV_BUS_BG5)
            ;

/* assert SACK */
cpld_assert(CV_ASSERT_SACK);

        /* deassert BR5 */
        cpld_deassert(CV_ASSERT_BR5);

cpld_deassert(CV_ASSERT_SACK);

        cpld_write(CPLD_REG_PASS_THRU, ~0);
        cpld_deassert(CV_ASSERT_BG5);
    }

    enable_cpld_int();
}

void
bus_wiggle_dma(void)
{
    int val;

    disable_cpld_int();
    bus_reset();

    while (1) {
        if (usart_getc_rdy())
            break;

        /* assert NPG to drive low (deasserted) when pass-thru disabled */
        cpld_assert(CV_ASSERT_NPG);

        /* disable NPG pass-thru w/cpld */
        cpld_write(CPLD_REG_PASS_THRU, ~CV_PASS_NPG);

        /* assert NPR */
        cpld_assert(CV_ASSERT_NPR);

        /* assert SACK */
        cpld_assert(CV_ASSERT_SACK);

        /* deassert NPR */
        cpld_deassert(CV_ASSERT_NPR);

        /* wait for NPG to deassert */
        while (!(cpld_read(CPLD_REG_STATUS) & CV_BUS_NPG))
            ;

        /* wait for BBSY to deassert */
        while (BUS_SSYN_OR_BBSY())
            ;

        /* assert BBSY */
        cpld_assert(CV_ASSERT_BBSY);

        cpld_deassert(CV_ASSERT_SACK);

        /* do read/write here */

        /* enable NPG pass-thru w/cpld */
        cpld_write(CPLD_REG_PASS_THRU, ~0);
        cpld_deassert(CV_ASSERT_NPG);

        /* deassert BBSY */
        cpld_deassert(CV_ASSERT_BBSY);
    }

    enable_cpld_int();
}

void
bus_wiggle_data(void)
{
#if 0
    unsigned short data;

    printf("bus data wiggle\n");

    data = 0xaaaa;
    cpld_write(CPLD_REG_DATA, data);
    cpld_assert(CV_ASSERT_DATA_DIR);

    while (1) {
        if (usart_getc_rdy())
            break;

        cpld_write(CPLD_REG_DATA, data);
        data = ~data;
    }

    cpld_deassert(CV_ASSERT_DATA_DIR);
#endif
}

void
bus_match(void)
{
#if 0
    int match, ah, al, a;
    int addr, mask;

    {
        extern unsigned long cpld_ints;
        printf("cpld_ints %d\n", cpld_ints);
        cpld_write(CPLD_REG_INT_ACK, 0);
    }

    match = cpld_read(CPLD_REG_MATCH_STATE);
    ah = cpld_read(CPLD_REG_MADDR_HI);
    al = cpld_read(CPLD_REG_MADDR_LO);

    a = (ah << 16) | al;
    printf("match addr %o, match %o\n", a, match);
#endif
}

void 
bus_match_test(void)
{
#if 0
    int addr, mask;

    printf("bus match test\n");

    addr = 0;
    mask = 0;
    cpld_write(CPLD_REG_ADDR1, addr >> 2);
    cpld_write(CPLD_REG_MASK1, mask >> 2);

    int ms, bah, bal, ba;
    ms = cpld_read(12);
    bah = cpld_read(13);
    bal = cpld_read(14);
    ba = (bah << 16) | bal;
    printf("0,0 ba %x, ms %x\n", ba, ms);

    addr = ~0;
    mask = 0;
    cpld_write(CPLD_REG_ADDR1, addr >> 2);
    cpld_write(CPLD_REG_MASK1, mask >> 2);

    ms = cpld_read(12);
    bah = cpld_read(13);
    bal = cpld_read(14);
    ba = (bah << 16) | bal;
    printf("~0,0 ba %x, ms %x\n", ba, ms);

    addr = 0xffffff;
    mask = 0;
    cpld_write(CPLD_REG_ADDR1, addr >> 2);
    cpld_write(CPLD_REG_MASK1, mask >> 2);

    ms = cpld_read(12);
    bah = cpld_read(13);
    bal = cpld_read(14);
    ba = (bah << 16) | bal;
    printf("~0,0 ba %x, ms %x\n", ba, ms);

    addr = 0xf0;
    mask = 0;
    cpld_write(CPLD_REG_ADDR1, addr >> 2);
    cpld_write(CPLD_REG_MASK1, mask >> 2);

    ms = cpld_read(12);
    bah = cpld_read(13);
    bal = cpld_read(14);
    ba = (bah << 16) | bal;
    printf("4,0 ba %x, ms %x\n", ba, ms);

    addr = 0xffff;
    mask = 0xffff;
    cpld_write(CPLD_REG_ADDR1, addr >> 2);
    cpld_write(CPLD_REG_MASK1, mask >> 2);

    ms = cpld_read(12);
    bah = cpld_read(13);
    bal = cpld_read(14);
    ba = (bah << 16) | bal;
    printf("~0,~0 ba %x, ms %x\n", ba, ms);
#endif
}

void
test_dma_cont(void)
{
    int i, addr;

    printf("continuous dma test\n");
    addr = 02000;

    while (1) {
        if (usart_getc_rdy())
            break;

        unibus_dma_buffer(1, addr, buffer1, 128);
        addr += 0400;
        if (addr > 0200000)
            addr = 02000;
    }
}

void
test_cpld_read(void)
{
    while (1) {
        if (usart_getc_rdy())
            break;
        cpld_read(CPLD_REG_DATA);
    }
}

void
test_cpld_write(void)
{
    while (1) {
        if (usart_getc_rdy())
            break;
        cpld_write(CPLD_REG_DATA, 0);
    }
}

void
test_cpld_msyn(void)
{
    while (1) {
        cpld_assert(CV_ASSERT_MSYN);
        cpld_deassert(CV_ASSERT_MSYN);
    }
}

void
test_cause_int(void)
{
    int loops;

    printf("cause int\n");
    cpld_assert(CV_ASSERT_BG5);

//    /* disable BRx pass-thru w/cpld */
//    cpld_write(CPLD_REG_PASS_THRU, ~CV_PASS_BG5);

    /* assert BRx */
    cpld_assert(CV_ASSERT_BR5);

    /* wait for BGx */
    for (loops = 0; loops < 5000; loops++)
        if  ((cpld_read(CPLD_REG_STATUS) & CV_BUS_BG5) == 0)
            break;

    if (loops == 5000) {
        printf("t/o bg5\n");
        cpld_deassert(CV_ASSERT_BR5);
        return;
    }

    /* assert SACK */
    cpld_assert(CV_ASSERT_SACK);

    disable_cpld_int();

    cpld_deassert(CV_ASSERT_BR5);

    /* wait for BGx to go away*/
    while ((cpld_read(CPLD_REG_STATUS) & CV_BUS_BG5) == 0)
        ;

    /* wait for BBSY to deassert */
    while (BUS_BBSY())
        ;

    /* assert BBSY */
    cpld_assert(CV_ASSERT_BBSY);

    cpld_write(CPLD_REG_DATA, 0160);
    cpld_assert(CV_ASSERT_DATA_DIR);

//    /* wait for no SSYN */
//    while (BUS_SSYN())
//        ;

    cpld_assert(CV_ASSERT_INTR);
    cpld_deassert(CV_ASSERT_SACK);

    /*wait for SSYN */
    while (!BUS_SSYN())
        ;

    cpld_deassert(CV_ASSERT_DATA_DIR | CV_ASSERT_INTR | CV_ASSERT_BBSY);

//    cpld_write(CPLD_REG_PASS_THRU, ~0);
    cpld_deassert(CV_ASSERT_BG5);

    enable_cpld_int();
    printf("cause int done\n");
}

void
test_int_cont(void)
{
    int n;
    while (1) {
        if (usart_getc_rdy())
            break;
        test_cause_int();
        for (n = 0; n < 10; n++) {
            d();
        }
    }
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
