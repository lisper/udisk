/*
 * rk.c
 * $Id: rk.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "bus.h"
#include "cpld.h"
#include "rk11.h"

u_char cmd_pending;
u_char int_pending;
u_char seek_pending;
u_char init_pending;

int seek_time;

u16 rkds;
u16 rker;
u16 rkcs;
u16 rkwc;
u32 rkba;
u16 rkda;

/*
 * rkda
 *    111111
 *    5432109876543210
 *    dddtttttttttssss
 *
 */

u_char ds;
struct drive_s {
    u_char ready;
} drive[8];

int idle_time;

#define byte_place(addr, old, byte) \
    (((addr) & 1) ? ((old) & 0377) | ((byte) << 8) : ((old) & ~0377) | (byte))

extern unsigned long cpld_ints;
extern int rmws;
extern int reads_cs;
extern int writes_cs;
extern int bad_match;

extern u_short vector;

void
rk11_drive_check(void)
{
    if (drive[ds].ready)
        rkds = RKDS_RK05 | RKDS_RDY | RKDS_RWS;
    else
        rkds = 0;
}

/*
 * handle interrupt from cpld - addresss match
 */
void
cpld_isr(void)
{
    AT91PS_PIO pPio;
    unsigned short sigs, data, addr_lo;
    volatile u32 v;

    pPio = AT91C_BASE_PIOA;
    cpld_ints++;

    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
    *AT91C_AIC_ICCR = 0xffffffff;
    v = *AT91C_AIC_IPR;
    v = *AT91C_PIOA_ISR;

    /* sample C0,C1 */
    sigs = cpld_read(CPLD_REG_STATUS);

    /* low bits of matching address */
    addr_lo = cpld_read(CPLD_REG_RD_ADDR_LO);

#if 1
    /* short circuit rkcs reads */
    if ((sigs & (CV_BUS_C1 | CV_BUS_C0)) == 0 && (addr_lo & 077) == RKCS) {
        reads_cs++;
        cpld_write(CPLD_REG_DATA, rkcs);
        cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);
        cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
        *AT91C_AIC_ICCR = 0xffffffff;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
        return;
    }
#endif

    /* 18 bit address on unibus */
    if (cpld_read(CPLD_REG_RD_ADDR_HI) != 03) {
        bad_match++;
        goto done;
    }

    /* read? */
    if ((sigs & CV_BUS_C1) == 0) {
        /* read */
        switch (addr_lo & 016) {
        case RKDS:
            data = rkds;
            break;
        case RKER:
            data = rker;
            break;
        case RKCS:
            data = rkcs;
            break;
        case RKWC:
            data = rkwc;
            break;
        case RKBA:
            data = rkba;
            break;
        case RKDA:
            data = rkda;
            break;
        }

        cpld_write(CPLD_REG_DATA, data);
        /* done */
    }

    /* catch DATIP (r-m-w) */
    if ((sigs & (CV_BUS_C0 | CV_BUS_C1)) == (CV_BUS_C0)) {

        /* release first half */
        cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);

        /* wait for state machine to cycle */
        do {
            sigs = cpld_read(CPLD_REG_STATUS);
        } while (sigs & (1 << 8));

        /* wait for new msyn */
	while ((sigs & (1 << 8)) == 0) {
            sigs = cpld_read(CPLD_REG_STATUS);
        }

        rmws++;
    }

    /* write? */
    if (sigs & CV_BUS_C1) {
        /* write; get the data bus */
        data = cpld_read(CPLD_REG_DATA);

        /* honor byte writes */
        if (sigs & CV_BUS_C0)
            data = byte_place(addr_lo, rkcs, data);

        switch (addr_lo & 016) {
        case RKDS:
        case RKER:
            break;
        case RKCS:
            /* clear IE */
            if ((data & CS_IE) == 0)
                int_pending = 0;
            else
                /* setting IE */
                if ((rkcs & (CS_CRDY | CS_IE)) == CS_CRDY)
                    int_pending = 1;

            rkcs = data & 0176;
            rkba = (rkba & 0x0ffff) | ((data & 0x30) << 16);

            if (data & CS_GO) {
                rkcs &= ~CS_CRDY;
                cmd_pending = 1;
                int_pending = 0;
            }

            writes_cs++;
            break;
        case RKWC:
            rkwc = data;
            break;
        case RKBA:
            rkba = (rkba & 0x30000) | data;
            break;
        case RKDA:
            rkda = data;
            ds = (data >> 13) & 7;
            rk11_drive_check();
            break;
        }
    }

    cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);

done:
    *AT91C_AIC_ICCR = 0xffffffff;
    v = *AT91C_AIC_IPR;
    v = *AT91C_PIOA_ISR;

    logw(8, sigs, addr_lo);
}

void
rk11_drive_online(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (disk_part_valid(i)) {
            printf("drive %d online\n", i);
            drive[i].ready = 1;
        }
    }

    rk11_drive_check();
}

void
rk11_drive_offline(void)
{
    rkcs = 0;
    rkds = 0;
    rker = 0;
}

/* */
void
rk11_reset(void)
{
    int i;

    cmd_pending = 0;
    int_pending = 0;
    init_pending = 1;
    ds = 0;
    idle_time = 0;

    rk11_drive_offline();
}


void
rk11_setup(void)
{
    rk11_reset();

    vector = RK11_VECTOR;

    setup_cpld_addr(RK11_BASE, 017777700);

    setup_cpld_intr();

    log_reset();
}

void
rk11_restart(void)
{
    rk11_reset();
    reset_stats();
}

/* 512 byte block */
u_short buffer2[256];

static inline void
cmd_done()
{
    rkcs |= CS_CRDY;
    int_pending = (rkcs & CS_IE) ? 1 : 0;
}

void
rk11_poll(void)
{
    u_char cmd;
    int sector, track, blockno, unit, wlen;
    u_short *bufferp;

    if (cmd_pending) {
        cmd_pending = 0;
        idle_time = 0;
        cmd = (rkcs >> 1) & 7;
        switch (cmd) {
        case RKCS_CMD_CTLRESET:
            rkda = 0;
            rkba = 0;
            cmd_done();
            break;
        case RKCS_CMD_WCHK:
            break;
        case RKCS_CMD_SEEK:
            cmd_done();
            break;
        case RKCS_CMD_RCHK:
            break;
        case RKCS_CMD_DRVRESET:
            cmd_done();
            break;
        case RKCS_CMD_WLK:
            break;

        case RKCS_CMD_WRITE:
        case RKCS_CMD_READ:
            unit = ds;
            sector = rkda & 0xf;
            track = (rkda >> 4) & 0x1ff;
            blockno = (track * 12) + sector;

            if (0) printf("rk%d: cmd %d block %d; wc %x, da %o, ba %o\n",
                          unit, cmd, blockno, rkwc, rkda, rkba);

	    do {
                if (cmd == RKCS_CMD_READ) {
//printf("rk: read %d; wc %x\n", blockno, rkwc);
                    read_disk_block512(unit, blockno, &bufferp);
//                    wlen = (rkwc == 0 || rkwc > 256) ? 256 : rkwc;
                    wlen = 0200000 - rkwc;
                    if (wlen > 256)
                        wlen = 256;
//printf("buffer %6o %6o %6o %6o\n", bufferp[0], bufferp[1], bufferp[2], bufferp[3]);
#if 1
                    unibus_dma_buffer(1, rkba, bufferp, wlen);
#else
#define MW 32
                    if (wlen < MW) {
                        unibus_dma_buffer(1, rkba, bufferp, wlen);
                    } else {
                        int l, s, pa;
                        l = wlen;
                        pa = rkba;
                        while (l > 0) {
                            s = l > MW ? MW : l;
                            unibus_dma_buffer(1, pa, bufferp, s);
                            l -= s;
                            pa += s*2;
                            bufferp += s;
                        }
                   }
#endif
                } else {
//                    wlen = (rkwc == 0 || rkwc > 256) ? 256 : rkwc;
                    wlen = 0200000 - rkwc;
                    if (wlen > 256)
                        wlen = 256;
                    unibus_dma_buffer(0, rkba, buffer2, wlen);
                    if (wlen < 256) {
                        int resid_bytes = (256-wlen)*2;
                        memset((char *)(buffer2 + wlen), 0, resid_bytes);
printf("rk: write %d; wc %x, wlen %d!\n", blockno, rkwc, wlen);
                    }
                    write_disk_block512(unit, blockno, buffer2);
                }

                blockno++;
                rkba += wlen*2;
                rkwc += wlen;
            } while (rkwc != 0);

            cmd_done();
            break;
        }
    }

    if (seek_pending) {
        if (--seek_time == 0) {
            seek_pending = 0;
            idle_time = 0;
//            drive[ds].curr_head = drive[ds].new_head;
//            drive[ds].curr_cyl = drive[ds].new_cyl;
        }
    }

    if (int_pending) {
        int_pending = 0;
        idle_time = 0;
        rkds = (rkds & ~(7<<13)) | (ds << 13);
        unibus_interrupt();
    }

    if (init_pending) {
        rk11_reset();
        rk11_drive_online();
        init_pending = 0;
        idle_time = 0;
    }

    idle_time++;
    if (idle_time > 10000) {
        idle_time = 0;
        led_clear();
    }
}

/* */
void
controller_setup(void)
{
    rk11_setup();
}

void
controller_online(void)
{
    rk11_drive_online();
}

void
controller_restart(void)
{
    rk11_restart();
}

void
controller_poll(void)
{
    rk11_poll();
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
