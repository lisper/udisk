/*
 * rl.c
 * $Id: rl.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "bus.h"
#include "cpld.h"
#include "rl11.h"

int debug;

/*
rl11 emulation

i/o addresses 17774400 - 17774407

17774400  rlcs	read/write
17774402  rlba	read/write
17774404  rlda	read/write
17774406  rlmp	read/write

so, address match on 177744xx
*/


unsigned short cs;
unsigned short ba;
unsigned short da;
unsigned short mp[3];
unsigned short mp_gs;

u_char ds10;		/* currently selected drive */

u_char cmd_pending;
u_char int_pending;

u_char seek_pending;
u_short seek_time;
short seek_ms;

u_char init_pending;

int idle_time;

static void inline 
logw(int reg, int v1, int v2)
{
extern  u_int32 log[64][3];
extern int log_p;
    log[log_p][0] = reg;
    log[log_p][1] = v1;
    log[log_p][2] = v2;
    if (log_p < 63)
        log_p++;
    else
        log_p = 0;
}

struct drive_s {
    char ready;
    char rl02;
    char write_prot;

    u_short da;
    u_short cyls;

    char curr_head;
    u_short curr_cyl;

    char new_head;
    u_short new_cyl;
} drive[4];

#define byte_place(addr, old, byte) \
    (((addr) & 1) ? ((old) & 0377) | ((byte) << 8) : ((old) & ~0377) | (byte))

static void inline
update_cs(void)
{
    if (cs & CS_ANY_ERR)
        cs |= CS_ERR;

    /* check drive ready */
    ds10 = (cs >> 8) & 3;
    if (drive[ds10].ready) {
        cs |= CS_DRDY;
        mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK;
    } else {
        cs &= ~CS_DRDY;
        mp_gs = MP_GS_CO | MP_GS_ST_LOAD /*| MP_GS_SPE*/;
    }
//hack
//if (drive[ds10].ready)
//     { cs |= CS_DRDY; mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK; }
//else { cs &= ~CS_DRDY; mp_gs = MP_GS_CO | MP_GS_ST_LOAD | MP_GS_SPE; }
}

/*
  Basic register based device psuedo code:

  READ/WRITE:
  clear cpld int ena
  set address & mask
  set cpld int ena
  wait for BBSY/MSYN/addr-match interrupt

  at interrupt
  sample C0,C1
    (c1==0->read,c1==1->write;c0==0->16bit,c0==1->8bit,
    A00==1 use D<15:08>,A00==0 use D<07:00>)
  if read
     write data
     write cpu-release
  if write
     write cpu-release
     read  data

  INTERRUPT:
  disable BRx pass-thru w/cpld
  assert BRx
  wait for BGx
  assert SACK
  wait for BBSY to deassert
  assert BBSY
  assert data (w/vector)
  assert INTR
  wait for SSYN

  enable BRx pass-thru w/cpld
  deassert data
  deassert INTR
  deassert BBSY

  NPR:
  disable NPG pass-thru w/cpld
  assert NPR
  wait for NPG
  assert SACK
  wait for BBSY to deassert
  assert BBSY

  assert addr
  assert C0,C1
  if write
    assert data
  wait 150ns
  assert MSYN

  if write
    wait for SSYN to assert
    deassert MSYN
    deassert data

  if read
    wait for SSYN to assert
    sample data
    deassert MSYN

  wait 75ns
  deassert addr

  enable NPG pass-thru w/cpld
  deassert BBSY
*/

extern unsigned long cpld_ints;
extern int rmws;
extern int reads_cs;
extern int writes_cs;
extern int bad_match;

extern u_short vector;

//#include "isr1.c"
#include "isr2.c"

rl11_drive_online()
{
    int i;

    for (i = 0; i < 4; i++) {
        if (disk_part_valid(i)) {
            printf("drive %d online\n", i);
            drive[i].ready = 1;
            drive[i].rl02 = 1;
        }
    }

    if (drive[ds10].ready)
        mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK;

    update_cs();
}

rl11_drive_offine()
{
    drive[ds10].ready = 0;
    mp_gs = MP_GS_CO | MP_GS_ST_LOAD;
    printf("drive %d offline\n", ds10);

    update_cs();
}

void
rl11_reset(void)
{
    int i;

printf("rl11_reset\n");
    cmd_pending = 0;
    int_pending = 0;
    init_pending = 1;

    cs = CS_CRDY;
    ba = 0;
    da = 0;
    mp[0] = mp[1] = mp[2] = 0;
    ds10 = 0;

    rl11_drive_offine();
    update_cs();

    for (i = 0; i < 4; i++) {
        memset((char *)&drive[i], 0, sizeof(struct drive_s));
        drive[i].ready = 0;
        drive[i].rl02 = 0;
        drive[i].write_prot = 0;
        drive[i].da = 0;
        drive[i].cyls = 512;

        drive[i].curr_cyl = 0;
        drive[i].curr_head = 0;
        drive[i].new_cyl = 0;
        drive[i].new_head = 0;
    }
}

void
rl11_restart(void)
{
    rl11_reset();
    reset_stats();
}

void
rl11_setup(void)
{
    rl11_reset();

    vector = 0160;

log_reset();
    setup_cpld_addr(017774400, 017777700);

    setup_cpld_intr();
}

/* 256 byte block */
u_short buffer2[128];

static inline void
cmd_done()
{
    cs |= CS_CRDY | CS_DRDY;
    int_pending = (cs & CS_IE) ? 1 : 0;
}

void
rl11_poll(void)
{
    u_short offset, da_cyl;
    short newcyl, maxcyl, wlen;
    u_char func, da_hd, da_sect, max_sectors, swlen, unit;
    int i, phys_addr, blockno;

    if (int_pending) {
        int_pending = 0;
        idle_time = 0;
        unibus_interrupt();
    }

    if (cmd_pending) {
        cmd_pending = 0;
        idle_time = 0;

        cs &= ~CS_DRDY;
        func = (cs >> 1) & 7;

        if (debug > 1) printf("rl%d: cmd %d cs %o %c%c\n",
                              ds10, func, cs,
                              (cs & CS_CRDY) ? 'r' : '-',
                              (cs & CS_IE) ? 'i' : '-' );

        switch (func) {
        case CS_FUNC_NOP:
            cmd_done();
            break;

        case CS_FUNC_GETSTATUS:
            /* check da */
//hack
mp_gs = MP_GS_HO | MP_GS_BH | MP_GS_ST_LOCK;
            if (da & DA_CLR) {
                mp_gs &=
                    ~(MP_GS_DSE | MP_GS_VC | MP_GS_WGE | MP_GS_SPE | 
                      MP_GS_SKTO | MP_GS_CHE | MP_GS_WDE);
            }

            mp_gs &= ~MP_GS_HS;
            mp_gs |= (drive[ds10].curr_head ? MP_GS_HS : 0);

            if (drive[ds10].rl02)
                mp_gs |= MP_GS_DT;
            if (drive[ds10].write_prot)
                mp_gs |= MP_GS_WL;

//hack
mp_gs |= MP_GS_DT;

            mp[0] = mp[1] = mp[2] = mp_gs;
//            mp[0] = mp_gs;
//            mp[1] = mp_gs;
//            mp[2] = 0;
            cmd_done();
            break;

        case CS_FUNC_SEEK:
            offset = da >> 7;

            if (debug) printf("seek: at cyl %o, da %o, offset %o\n",
                              drive[ds10].curr_cyl, da, offset);

            if (da & DA_DIR) {
                /* forward */
                newcyl = drive[ds10].curr_cyl + offset;
                maxcyl = drive[ds10].cyls;
                if (newcyl >= maxcyl)
                    newcyl = maxcyl - 1;
            } else {
                /* reverse */
                newcyl = drive[ds10].curr_cyl - offset;
                if (newcyl < 0)
                    newcyl = 0;
            }

            drive[ds10].new_head = (da >> 4) & 1;
            drive[ds10].new_cyl = newcyl;

            drive[ds10].da = newcyl << 7 | (da & DA_HS);

            seek_pending = 1;
            seek_time = (newcyl - drive[ds10].curr_cyl) * seek_ms;
            if (debug) printf("seek: to cyl %o\n", drive[ds10].new_cyl);
#if 0
seek_pending = 0;
seek_time = 0;
drive[0].curr_head = drive[ds10].new_head;
drive[0].curr_cyl = drive[ds10].new_cyl;
cmd_done();
#else
seek_time = 1000;
#endif
            break;

        case CS_FUNC_READHDR:
            mp[0] = (drive[ds10].curr_cyl << 7) | 
                (drive[ds10].curr_head << 6) |
                (da & 077);
            if (0) printf("readhdr; ba %o, da %o, mp %o\n", ba, da, mp[0]);
            mp[1] = 0;
            mp[2] = 0; /* crc */
            cmd_done();
            break;

        case CS_FUNC_READNOHDR:
            break;

        case CS_FUNC_WRITE:
            /* are we write-protected? */
            if (drive[ds10].write_prot) {
                mp_gs |= MP_GS_WGE;
                cs |= CS_ERR | CS_DE;
                cmd_done();
                break;
            }
            /* fall through */

        case CS_FUNC_READ:
        case CS_FUNC_WRITECHK:
            unit = (cs >> 8) & 3;
            da_cyl = da >> 7;
            da_hd = (da >> 6) & 1;
            da_sect = da & 077;

            if (0) printf("read: ba %o, da %o, wc %o\n", ba, da, mp[0]);

if (debug) printf("read: da %o, offset %o, u %d, c %d, h %d s %d\n",
                  da, ((da >> 6)*40 + da_sect)*256,
                  unit, da_cyl, da_hd, da_sect);

            drive[ds10].curr_head = (da >> 4) & 1;

#if 0
            if (drive[ds10].curr_cyl != da_cyl || da_sect >= 40) {
                /* cyl doesn't match */
printf("cyl! %d %d %d %d\n", ds10, drive[ds10].curr_cyl, da_cyl, da_sect);
printf("da %x\n", da);
// just for now - looks like we complete seek too soon
//                cs |= CS_ERR | CS_E_HCRC | CS_E_OPI;
            }
#endif

            phys_addr = (((cs & CS_BA1617) >> 4) << 16) | ba;
            wlen = 02000000 - mp[0];
if (debug) printf("wlen %o, mp[0] %o\n", wlen, mp[0]);

            /* clamp wlen at remaining sectors */
            max_sectors = 40 - (da & 077);
            if (wlen > max_sectors * 128)
                wlen = max_sectors * 128;

            if (wlen < 0) {
printf("wlen! %d\n", wlen);
                cs |= CS_E_OPI;
                cmd_done();
                break;
            }

            blockno = (da_cyl*80) + (da_hd*40) + da_sect;

#if 0
            printf("rl%d: da %o, offset %o, blockno %d, chs %d/%d/%d "
                   "wlen %d\n",
                   unit, da, ((da >> 6)*40 + da_sect)*256, blockno,
                   da_cyl, da_hd, da_sect, wlen);
#endif

            while (wlen > 0) {

                u_short *bufferp;

                swlen = wlen > 128 ? 128 : wlen;

                if (func == CS_FUNC_READ) {
if (debug) printf("read; u%d, b%d, len %d => %o\n", unit, blockno, swlen, phys_addr);
                    read_disk_block256(unit, blockno, &bufferp);
#if 1
                    unibus_dma_buffer(1, phys_addr, bufferp, swlen);
#else
//#define MW 32
#define MW 8
                    if (swlen < MW) {
                        unibus_dma_buffer(1, phys_addr, bufferp, swlen);
                    } else {
                        int l, s, pa;
                        l = swlen;
                        pa = phys_addr;
                        while (l > 0) {
                            s = l > MW ? MW : l;
//printf("pa %o, bufferp %x, s %d\n", pa, bufferp, s);
                            unibus_dma_buffer(1, pa, bufferp, s);
                            l -= s;
                            pa += s*2;
                            bufferp += s;
                        }
                    }
#endif
                }

                if (func == CS_FUNC_WRITECHK) {
printf("writechk; u%d, b%d, len %d => %o\n", unit, blockno, swlen, phys_addr);
                    read_disk_block256(unit, blockno, &bufferp);
                    unibus_dma_buffer(0, phys_addr, buffer2, swlen);
                    for (i = 0; i < swlen; i++) {
                        if (bufferp[i] != buffer2[i]) {
                            cs |= CS_ERR | CS_E_DCRC;
                        }
                    }
                }

                if (func == CS_FUNC_WRITE) {
if (debug) printf("write; u%d, b%d, len %d => %o\n", unit, blockno, swlen, phys_addr);
                    unibus_dma_buffer(0, phys_addr, buffer2, swlen);
                    if (swlen < 128) {
                        int resid_bytes = (128-swlen)*2;
                        memset(((char *)buffer2) + swlen, 0, resid_bytes);
                    }
                    write_disk_block256(unit, blockno, buffer2);
                }

//printf("mp[0] %o + swlen %o = %o\n", mp[0], swlen, (mp[0] + swlen) & 0177777);

                mp[0] = (mp[0] + swlen) & 0177777;
                phys_addr += swlen*2;

                ba = phys_addr & 0177776;
                cs = (cs & ~CS_BA1617) | (((phys_addr >> 16) & 3) << 4);
                da++;

                blockno++;
                wlen -= swlen;
            }
            
            if (mp[0] != 0) {
printf("mp[0]! %o\n", mp[0]);
                cs |= CS_ERR | CS_E_OPI;
            }

//drive[ds10].curr_cyl = da >> 7;
//drive[ds10].curr_head = (da >> 6) & 1;

if (debug) printf("done\n");
            cmd_done();
            break;
        }
    }

    if (seek_pending) {
        if (--seek_time == 0) {
            seek_pending = 0;
            idle_time = 0;

            drive[ds10].curr_head = drive[ds10].new_head;
            drive[ds10].curr_cyl = drive[ds10].new_cyl;

//printf("seek done; new h %d, c %d\n", drive[ds10].new_head, drive[ds10].new_cyl);
            cmd_done();
        }
    }

    if (int_pending) {
        int_pending = 0;
        idle_time = 0;
        unibus_interrupt();
    }

    if (init_pending) {
        rl11_reset();
        idle_time = 0;
        init_pending = 0;
        rl11_drive_online();
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
    /* RL11 */
    rl11_setup();

    debug = 0;
    idle_time = 0;
}

void
controller_online(void)
{
    rl11_drive_online();
}

void
controller_restart(void)
{
    rl11_restart();
}

void
controller_poll(void)
{
    rl11_poll();
}

void
controller_print_state(void)
{
    int i;

    printf("RL11:\n");
    printf("cs %o\n", cs);
    printf("ba %o\n", ba);
    printf("da %o\n", da);
    printf("mp %o\n", mp[0]);

    for (i = 0; i < 4; i++) {
        printf("drive %d ", i);
        if (drive[i].ready) printf("on"); else printf("off");
        printf("line, rl0%d", drive[i].rl02 ? 2 : 1);
        printf("\n");
    }
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
