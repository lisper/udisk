/*
 * ide.c
 * $Id: ide.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "ide.h"
#include "bus.h"
#include "cpld.h"
#include "irq.h"

int ide_present;
int disk_cyls;
int disk_sectors_per_track;
int disk_heads;

char ide_buffer[512];
char ide_buffer2[512];
int verbose;

/* structure returned by HDIO_GET_IDENTITY, as per ANSI ATA2 rev.2f spec */
struct hd_driveid {
    unsigned short	config;		/* lots of obsolete bit flags */
    unsigned short	cyls;		/* "physical" cyls */
    unsigned short	reserved2;	/* reserved (word 2) */
    unsigned short	heads;		/* "physical" heads */
    unsigned short	track_bytes;	/* unformatted bytes per track */
    unsigned short	sector_bytes;	/* unformatted bytes per sector */
    unsigned short	sectors;	/* "physical" sectors per track */
    unsigned short	vendor0;	/* vendor unique */
    unsigned short	vendor1;	/* vendor unique */
    unsigned short	vendor2;	/* vendor unique */
    unsigned char	serial_no[20];	/* 0 = not_specified */
    unsigned short	buf_type;
    unsigned short	buf_size;	/* 512 byte increments; 0 = not_specified */
    unsigned short	ecc_bytes;	/* for r/w long cmds; 0 = not_specified */
    unsigned char	fw_rev[8];	/* 0 = not_specified */
    unsigned char	model[40];	/* 0 = not_specified */
    unsigned char	max_multsect;	/* 0=not_implemented */
    unsigned char	vendor3;	/* vendor unique */
    unsigned short	dword_io;	/* 0=not_implemented; 1=implemented */
    unsigned char	vendor4;	/* vendor unique */
    unsigned char	capability;	/* bits 0:DMA 1:LBA 2:IORDYsw 3:IORDYsup*/
    unsigned short	reserved50;	/* reserved (word 50) */
    unsigned char	vendor5;	/* vendor unique */
    unsigned char	tPIO;		/* 0=slow, 1=medium, 2=fast */
    unsigned char	vendor6;	/* vendor unique */
    unsigned char	tDMA;		/* 0=slow, 1=medium, 2=fast */
    unsigned short	field_valid;	/* bits 0:cur_ok 1:eide_ok */
    unsigned short	cur_cyls;	/* logical cylinders */
    unsigned short	cur_heads;	/* logical heads */
    unsigned short	cur_sectors;	/* logical sectors per track */
    unsigned short	cur_capacity0;	/* logical total sectors on drive */
    unsigned short	cur_capacity1;	/*  (2 words, misaligned int)     */
    unsigned char	multsect;	/* current multiple sector count */
    unsigned char	multsect_valid;	/* when (bit0==1) multsect is ok */
    unsigned int	lba_capacity;	/* total number of sectors */
    unsigned short	dma_1word;	/* single-word dma info */
    unsigned short	dma_mword;	/* multiple-word dma info */
    unsigned short  eide_pio_modes; /* bits 0:mode3 1:mode4 */
    unsigned short  eide_dma_min;	/* min mword dma cycle time (ns) */
    unsigned short  eide_dma_time;	/* recommended mword dma cycle time (ns) */
    unsigned short  eide_pio;       /* min cycle time (ns), no IORDY  */
    unsigned short  eide_pio_iordy; /* min cycle time (ns), with IORDY */
    unsigned short	words69_70[2];	/* reserved words 69-70 */
    /* HDIO_GET_IDENTITY currently returns only words 0 through 70 */
    unsigned short	words71_74[4];	/* reserved words 71-74 */
    unsigned short  queue_depth;	/*  */
    unsigned short  words76_79[4];	/* reserved words 76-79 */
    unsigned short  major_rev_num;	/*  */
    unsigned short  minor_rev_num;	/*  */
    unsigned short  command_set_1;	/* bits 0:Smart 1:Security 2:Removable 3:PM */
    unsigned short  command_set_2;	/* bits 14:Smart Enabled 13:0 zero */
    unsigned short  cfsse;		/* command set-feature supported extensions */
    unsigned short  cfs_enable_1;	/* command set-feature enabled */
    unsigned short  cfs_enable_2;	/* command set-feature enabled */
    unsigned short  csf_default;	/* command set-feature default */
    unsigned short  dma_ultra;	/*  */
    unsigned short	word89;		/* reserved (word 89) */
    unsigned short	word90;		/* reserved (word 90) */
    unsigned short	CurAPMvalues;	/* current APM values */
    unsigned short	word92;		/* reserved (word 92) */
    unsigned short	hw_config;	/* hardware config */
    unsigned short  words94_125[32];/* reserved words 94-125 */
    unsigned short	last_lun;	/* reserved (word 126) */
    unsigned short	word127;	/* reserved (word 127) */
    unsigned short	dlf;		/* device lock function
					 * 15:9	reserved
					 * 8	security level 1:max 0:high
					 * 7:6	reserved
					 * 5	enhanced erase
					 * 4	expire
					 * 3	frozen
					 * 2	locked
					 * 1	en/disabled
					 * 0	capability
					 */
    unsigned short  csfo;		/* current set features options
					 * 15:4	reserved
					 * 3	auto reassign
					 * 2	reverting
					 * 1	read-look-ahead
					 * 0	write cache
					 */
    unsigned short	words130_155[26];/* reserved vendor words 130-155 */
    unsigned short	word156;
    unsigned short	words157_159[3];/* reserved vendor words 157-159 */
    unsigned short	words160_255[95];/* reserved words 160-255 */
};

void
delayus(void)
{
    volatile long l;
    for (l = 0; l < /*50*/5; l++);
}

int ide_state;

static void inline
ide_enable(void)
{
    safe_cpld_write_subreg(CPLD_SUBREG_CF_ENA, CF_ENA_0);
}

static void inline
ide_disable(void)
{
    safe_cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);
}

void
ide_assert_reset(void)
{
    disable_cpld_int();
    cpld_write_subreg(CPLD_SUBREG_CF_ENA, CF_ENA_0 | CF_RESET);

    delayus();

    cpld_write_subreg(CPLD_SUBREG_CF_ENA, CF_ENA_0);
    enable_cpld_int();
}

/* ------------------------------------------------------- */

static int inline
port_read(int num)
{
    if (num == 0)			/* data port */
        return (u_int16)cpld_read_long(0x8 | num);

    return cpld_read_long(0x8 | num) & 0x00ff;
}

static int inline
port_read8(int num)
{
    return cpld_read_long(0x8 | num) & 0x00ff;
}

static int inline
port_read16(int num)
{
    return (u_int16)cpld_read_long(0x8 | num);
}

static void inline
port_write(int num, int val)
{
    cpld_write_long(0x8 | num, val);
}

int
hd_busy_wait(void)
{
    int count;
    u_char status, err;

    if (verbose > 2)
        printf("hd_busy_wait() status %8x\n", port_read(7) & 0x00ff);

    status = port_read8(7);
    if (status == 0xff) {
        printf("status 0xff - ide not present?\n");
        return -1;
    }

#define BUSY_TIMEOUT 500000
    count = 0;
    while (count++ < BUSY_TIMEOUT) {
        status = port_read8(7);
        if ((status & BUSY) == 0)
            break;
        delayus();
    }

    if (count >= BUSY_TIMEOUT) {
        printf("timeout, status %x\n", status);
        err = port_read8(1);
        printf("error reg %2x\n", err);
        return -1;
    }

#if 0
    printf("busy_wait: count %d\n", count);
    printf("status %x\n", status);
#endif

    if (status & 0x01) {
        printf("status reg %x\n", status);
        err = port_read8(1);
        printf("err reg %x\n", err);
        return -1;
    }

    return 0;
}

static int
hd_command(u_char hd_cmd[8])
{
#if 0
    port_write(0, (hd_cmd[1] << 8) | hd_cmd[0]);
    port_write(2, (hd_cmd[3] << 8) | hd_cmd[2]);
    port_write(4, (hd_cmd[5] << 8) | hd_cmd[4]);
    port_write(6, (hd_cmd[7] << 8) | hd_cmd[6]);
#else
    port_write(0, hd_cmd[0]);
    port_write(1, hd_cmd[1]);
    port_write(2, hd_cmd[2]);
    port_write(3, hd_cmd[3]);
    port_write(4, hd_cmd[4]);
    port_write(5, hd_cmd[5]);
    port_write(6, hd_cmd[6]);
    port_write(7, hd_cmd[7]);
#endif

    return hd_busy_wait();
}

static int
hd_recal(void)
{
      int i, hd_drive, hd_head, sector;
      u_char status, hd_cmd[8];

      hd_drive = 0;
      sector = 0;
      hd_head = sector & 0xffff;

      hd_cmd[1] = 0;
      hd_cmd[2] = 1;			/* # sectors */
      hd_cmd[3] = 0;
      hd_cmd[4] = 0;
      hd_cmd[5] = 0;
      hd_cmd[6] = (hd_drive << 4) | (hd_head & 0x0f) | 0xa0;
      hd_cmd[7] = HDC_RECAL;

      if (hd_command(hd_cmd))
          return -1;

      if (hd_busy_wait())
	  return -1;

      return 0;
}

static int
hd_identify(char *buffer)
{
      int i;
      u_char status, hd_cmd[8];

      if (hd_busy_wait())
          return -1;

      hd_cmd[1] = 0;
      hd_cmd[2] = 1;
      hd_cmd[3] = 0;
      hd_cmd[4] = 0;
      hd_cmd[5] = 0;
      hd_cmd[6] = 0xa0;
      hd_cmd[7] = HDC_IDENTIFY;

      if (hd_command(hd_cmd)) {
          printf("identify cmd failed\n");
          return -1;
      }

      /* 16 bit data */
      for (i = 0; i < 512; i += 2) {
          u_int16 p;
          p = port_read(0);
          buffer[i] = p & 0xff;
          buffer[i+1] = p >> 8;
      }

      status = port_read(7);

      if (status & ERROR) {
	  printf("identify status: %2x\n", status);
	  return -1;
      }

      if (verbose > 2) {
          printf("identify status %8x\n", status);
          dumpmem(buffer, 512);
      }

      return 0;
}

static int 
hd_read_hsc(int head, int sector, int cyl, char *buffer)
{
      int i, hd_drive, hd_cyl, hd_head, hd_sector;
      u_char status, hd_cmd[8];

      hd_drive = 0;
      hd_cyl = cyl;
      hd_head = head;
      hd_sector = sector;

      hd_cmd[1] = 0;
      hd_cmd[2] = 1;			/* # sectors */
      hd_cmd[3] = hd_sector;		/* starting sector */
      hd_cmd[4] = cyl & 0xff;		/* cylinder low byte */
      hd_cmd[5] = (cyl >> 8) & 0xff;	/* cylinder hi byte */
      hd_cmd[6] = (hd_drive << 4) | (hd_head & 0x0f) | 0xa0;
      hd_cmd[7] = HDC_READ;

      if (verbose > 2)
	  printf("hd_read_hsc() h=%d s=%d c=%d\n", hd_head, hd_sector, hd_cyl);

#if 0
      if (hd_head > 7) {
	//	hd_control |= 0x08;
	//	reg[0] = hd_control;
	//	hd_control &= 0xf7;
		reg[0] = 0x08;
      }
#endif

      /* --- */
      if (hd_busy_wait())
          return -1;

      if (hd_command(hd_cmd)) {
          printf("read cmd failed\n");
          return -1;
      }

      for (i = 0; i < 512; i += 2) {
          u_int16 p;
          p = port_read16(0);
          buffer[i] = p & 0xff;
          buffer[i+1] = p >> 8;
#if 1
          delayus();
#endif
      }

      status = port_read8(7);

      if (status & ERROR) {
	  printf("read status: %2x\n", status);

	  printf("h=%d s=%d c=%d\n", hd_head, hd_sector, hd_cyl);
	  for (i = 1; i < 7; i++) {
	    printf("set reg[%d] %2x\n", i, hd_cmd[i]);
	  }
      }

      if (status & ERROR)
	  return -1;

      return 0;
}

static int 
hd_write_hsc(int head, int sector, int cyl, char *buffer)
{
      int i, hd_drive, hd_cyl, hd_head, hd_sector;
      u_char status, hd_cmd[8];
      u_int16 *pb = (u_int16 *)buffer;

      hd_drive = 0;
      hd_cyl = cyl;
      hd_head = head;
      hd_sector = sector;

      hd_cmd[1] = 0;
      hd_cmd[2] = 1;			/* # sectors */
      hd_cmd[3] = hd_sector;		/* starting sector */
      hd_cmd[4] = cyl & 0xff;		/* cylinder low byte */
      hd_cmd[5] = (cyl >> 8) & 0xff;	/* cylinder hi byte */
      hd_cmd[6] = (hd_drive << 4) | (hd_head & 0x0f) | 0xa0;
      hd_cmd[7] = HDC_WRITE;

      if (verbose > 2)
	  printf("hd_write_hsc() h=%d s=%d c=%d\n", hd_head, hd_sector, hd_cyl);

      /* --- */
      if (hd_busy_wait())
          return -1;

      if (hd_command(hd_cmd)) {
          printf("read cmd failed\n");
          return -1;
      }

      for (i = 0; i < 512; i += 2) {
          port_write(0, *pb++);
      }

      status = port_read(7);

      if (status & ERROR) {
	  printf("write status: %2x\n", status);

	  printf("h=%d s=%d c=%d\n", hd_head, hd_sector, hd_cyl);
	  for (i = 1; i < 7; i++) {
	    printf("set reg[%d] %2x\n", i, hd_cmd[i]);
	  }
      }

      if (status & ERROR)
	  return -1;

      return 0;
}

static int 
hd_read_lba(int block, char *buffer)
{
    int i;
    u_char status, hd_cmd[8];

    hd_cmd[1] = 0;
    hd_cmd[2] = 1;			/* # sectors */
    hd_cmd[3] = (block >>  0) & 0xff;	/* lba[7:0] */
    hd_cmd[4] = (block >>  8) & 0xff;	/* lba[15:8] */
    hd_cmd[5] = (block >> 16) & 0xff;	/* lba[23:16] */
    hd_cmd[6] = 0x40;
    hd_cmd[7] = HDC_READ;

      /* --- */
      if (hd_busy_wait())
          return -1;

      if (hd_command(hd_cmd)) {
          printf("read cmd failed\n");
          return -1;
      }

      for (i = 0; i < 512; i += 2) {
          u_int16 p;
          p = port_read16(0);
          buffer[i] = p & 0xff;
          buffer[i+1] = p >> 8;
#if 1
          delayus();
#endif
      }

      status = port_read8(7);

      if (status & ERROR) {
	  printf("read status: %2x\n", status);
	  for (i = 1; i < 7; i++) {
	    printf("set reg[%d] %2x\n", i, hd_cmd[i]);
	  }
      }

      if (status & ERROR)
	  return -1;

      return 0;
}

static int 
hd_write_lba(int block, char *buffer)
{
    int i;
    u_char status, hd_cmd[8];
    u_int16 *pb = (u_int16 *)buffer;

    hd_cmd[1] = 0;
    hd_cmd[2] = 1;			/* # sectors */
    hd_cmd[3] = (block >>  0) & 0xff;	/* lba[7:0] */
    hd_cmd[4] = (block >>  8) & 0xff;	/* lba[15:8] */
    hd_cmd[5] = (block >> 16) & 0xff;	/* lba[23:16] */
    hd_cmd[6] = 0x40;
    hd_cmd[7] = HDC_WRITE;

    /* --- */
    if (hd_busy_wait())
        return -1;

    if (hd_command(hd_cmd)) {
        printf("read cmd failed\n");
        return -1;
    }

    for (i = 0; i < 512; i += 2) {
        port_write(0, *pb++);
    }

    status = port_read(7);

    if (status & ERROR) {
        printf("write status: %2x\n", status);
        for (i = 1; i < 7; i++) {
	    printf("set reg[%d] %2x\n", i, hd_cmd[i]);
        }
    }

    if (status & ERROR)
        return -1;

    return 0;
}

static int led_state;

void
led_set(int set)
{
    cpld_write_subreg(CPLD_SUBREG_LED, set ? 0 : 1);
}

void
led_activity(void)
{
    led_state = led_state ? 0 : 1;
    led_set(led_state);
}

void
led_clear(void)
{
    led_state = 0;
    led_set(led_state);
}

/* read, mapping abs [0..max] sector # into head, sector, cylinder */
int 
hd_read_mapped(unsigned int sector_num, char *buffer)
{
#if 0
    int head, sector, cyl, ret;

    if (verbose > 2)
	printf("hd_read_mapped(sector_num=%d, buffer=%x)\n",
	       sector_num, buffer);

    sector = (sector_num % disk_sectors_per_track) + 1;
    sector_num /= disk_sectors_per_track;

    head = sector_num % disk_heads;
    sector_num /= disk_heads;

    cyl = sector_num;

    ide_enable();
    led_activity();

    ret = hd_read_hsc(head, sector, cyl, buffer);

    ide_disable();
    return ret;
#else
    int ret;

    if (verbose > 2)
	printf("hd_read_mapped(sector_num=%d, buffer=%x)\n",
	       sector_num, buffer);

    ide_enable();
    led_activity();

    ret = hd_read_lba(sector_num, buffer);

    ide_disable();
    return ret;
#endif
}

/* write, mapping abs [0..max] sector # into head, sector, cylinder */
int 
hd_write_mapped(unsigned int sector_num, char *buffer)
{
#if 0
    int head, sector, cyl, ret;

    if (verbose > 2)
	printf("hd_write_mapped(sector_num=%d, buffer=%x)\n",
	       sector_num, buffer);

    sector = (sector_num % disk_sectors_per_track) + 1;
    sector_num /= disk_sectors_per_track;

    head = sector_num % disk_heads;
    sector_num /= disk_heads;

    cyl = sector_num;

    ide_enable();
    led_activity();

    ret = hd_write_hsc(head, sector, cyl, buffer);

    ide_disable();
    return ret;
#else
    int ret;

    if (verbose > 2)
	printf("hd_write_mapped(sector_num=%d, buffer=%x)\n",
	       sector_num, buffer);

    ide_enable();
    led_activity();

    ret = hd_write_lba(sector_num, buffer);

    ide_disable();
    return ret;
#endif
}

void ide_fixstring (u_char *s, const int bytecount, const int byteswap)
{
    u_char *p = s, *end = &s[bytecount & ~1]; /* bytecount must be even */

    if (byteswap) {
        /* convert from big-endian to host byte order */
        for (p = end ; p != s;) {
            unsigned short *pp = (unsigned short *) (p -= 2);
            *pp = ((*pp >> 8) & 0xff) | ((*pp & 0xff) << 8);
        }
    }

    /* strip leading blanks */
    while (s != end && *s == ' ')
        ++s;

    /* compress internal blanks and strip trailing blanks */
    while (s != end && *s) {
        if (*s++ != ' ' || (s != end && *s && *s != ' '))
            *p++ = *(s-1);
    }

    /* wipe out trailing garbage */
    while (p != end)
        *p++ = '\0';
}

/* do an "ide indentify" command to the ATA drive to get it's geometry */
int
ide_identify_drive(void)
{
    struct hd_driveid *id;

    ide_enable();

    memset(ide_buffer, 0, sizeof(ide_buffer));

    if (hd_identify(ide_buffer)) {
        ide_disable();
        return -1;
    }

    id = (struct hd_driveid *)ide_buffer;

    ide_fixstring (id->model, sizeof(id->model), 1);

    printf("model: %s\n", id->model);
    printf("CHS:   %d/%d/%d\n", id->cyls, id->heads, id->sectors);

    if (id->sectors && id->heads) {
      disk_cyls = id->cyls;
      disk_sectors_per_track = id->sectors;
      disk_heads = id->heads;
    }

    ide_disable();

    return 0;
}

void
ide_show_regs(void)
{
    int i, r;

    ide_enable();

    for (i = 0; i < 8; i++) {
        r = port_read(i);
        printf("port %d = %x\n", i, r);
    }

    ide_disable();
}

void
ide_wiggle_reset(void)
{
    while (1) {
        cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);
        cpld_write_subreg(CPLD_SUBREG_CF_ENA, CF_ENA_0 | CF_RESET);

        if (usart_getc_rdy())
            break;
    }
}

void
ide_en(int en)
{
    if (en) ide_enable(); else ide_disable();
}

static void inline
raise_scope_line(void)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	pPio->PIO_SODR = P_USB_PUP;
}

static void inline
lower_scope_line(void)
{
        AT91PS_PIO pPio = AT91C_BASE_PIOA;
	pPio->PIO_CODR = P_USB_PUP;
}

void
ide_read_reg_cont(void)
{
#if 0
    cpld_write(CPLD_REG_CF_ENA, CF_ENA_0 | CF_RESET);
    cpld_write(CPLD_REG_CF_ENA, CF_ENA_0);
#endif

    while (1) {
        raise_scope_line();
        cpld_read_long(0x8 | 7);
        lower_scope_line();

        if (usart_getc_rdy())
            break;
    }
}

void
ide_read_block_cont(void)
{
    int i, count;

    ide_enable();

    count = 0;
    hd_read_hsc(1, 1, 0, ide_buffer);
    while (1) {
        printf("%d        \r", count);
        hd_read_hsc(1, 1, 0, ide_buffer2);
        for (i = 0; i < 512; i++) {
            if (ide_buffer[i] != ide_buffer2[i]) {
                printf("re-read failed @ byte %d %x %x\n",
                       i, ide_buffer[i], ide_buffer2[i]);
                break;
            }
        }
        
        if (usart_getc_rdy())
            break;
        count++;
    }

    ide_disable();
}

void
ide_read_block(int n)
{
    int i, count;

    ide_enable();

    count = 0;
    hd_read_hsc(0, n+1, 0, ide_buffer);

    printf("%2x %2x %2x %2x %2x %2x %2x %2x\n",
           ide_buffer[0], ide_buffer[1], ide_buffer[2], ide_buffer[3],
           ide_buffer[4], ide_buffer[5], ide_buffer[6], ide_buffer[7]);
        
    ide_disable();
}

void
ide_setup(void)
{
    ide_present = 0;
    verbose = 1;

    ide_assert_reset();

    if (ide_identify_drive() == 0) {
        ide_present = 1;
    }
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
