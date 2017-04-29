/*
 * disk.c
 * $Id: disk.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"

extern int ide_present;
extern char ide_buffer[];
extern int verbose;

extern int disk_cyls;
extern int disk_sectors_per_track;
extern int disk_heads;

int disk_ok;

/* ms-dos disk partition structure */
struct part_ent {
  u_char p_boot;
  u_char p_start[3];
  u_char p_type;
  u_char p_end[3];
  u_char p_start_sector[4];
  u_char p_size_sectors[4];
};

struct {
    u_char ptype;
    int start;
    int size;
} part_map[4];

/* ugg - bug; this needs to be at even addr */
static u_char buffer[512];

int
read_disk_block256(int part, int block256no, char **pbuffer)
{
    int block512;

#if 0
    printf("read_disk_block: part %d, blockno %d -> %d\n",
           part, block256no, block256no + part_map[part].start);
#endif

    /* we should cache, but for now just re-read */
    block512 = (block256no/2) + part_map[part].start;

    *pbuffer = (block256no & 1) ? buffer+256 : buffer;

    return hd_read_mapped(block512, buffer);
}

int
write_disk_block256(int part, int block256no, char *buffer256)
{
    int block512, ret;
    char *pbuffer;

#if 0
    //tempdebug!
    return 0;
#endif

    block512 = (block256no/2) + part_map[part].start;

    /* read old block and change proper half */
    ret = hd_read_mapped(block512, buffer);

    pbuffer = (block256no & 1) ? buffer+256 : buffer;
    memcpy(pbuffer, buffer256, 256);

    return hd_write_mapped(block512, buffer);
}

int
read_disk_block512(int part, int blockno, char **pbuffer)
{
    *pbuffer = buffer;
    blockno += part_map[part].start;
    return hd_read_mapped(blockno, buffer);
}

int
write_disk_block512(int part, int blockno, char *buffer)
{
    blockno += part_map[part].start;
    return hd_write_mapped(blockno, buffer);
}

int
read_buffer(int sectornum, char *buf)
{
    return hd_read_mapped(sectornum, buf);
}

int
le32_p(u_char *p)
{
    int n;

    n = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
    return n;
}

int
disk_part_valid(int partnum)
{
    if (partnum > 3)
        return 0;
    return part_map[partnum].ptype;
}

/* check master boot record; locate parition info */
int
disk_read_part(void)
{
    struct part_ent *p;
    int i, h, s, c;

    printf("check MBR... ");

    if (read_buffer(0, ide_buffer)) {
        printf("mbr read error\n");
        return -1;
    }

    for (i = 0; i < 4; i++) {
        part_map[i].ptype = 0;
        part_map[i].start = 0;
        part_map[i].size = 0;
    }

    if ((u_char)ide_buffer[510] != 0x55 || (u_char)ide_buffer[511] != 0xaa) {
        printf("bad mbr signature %2x%2x\n", 
               (u_char)ide_buffer[510], (u_char)ide_buffer[511]);
        if (verbose > 1) dumpmem(ide_buffer, 512);
        part_map[0].ptype = 1;
        part_map[0].size = disk_cyls * disk_sectors_per_track * disk_heads;
        return -1;
    }

    /* XXX 1st partition */
    p = (struct part_ent *)&ide_buffer[0x1be];

    for (i = 0; i < 4; i++) {
        h = p->p_start[0];
        s = p->p_start[1] & 0x3f;
        c = ((p->p_start[1] & 0xc0) << 2) + p->p_start[2];

        if (p->p_type) {
            part_map[i].ptype = p->p_type;
            part_map[i].start = le32_p(p->p_start_sector);
            part_map[i].size = le32_p(p->p_size_sectors);

            if (/*verbose > 1*/1) {
                printf("#%d: start %d, size %d\n",
                       i, part_map[i].start, part_map[i].size);
                printf("partition @ h=%d,s=%d,c=%d\n", h, s, c);
            }
        }

        p++;
    }

    printf("ok\n");

    return 0;
}

void
disk_setup()
{
    disk_ok = 0;

    /* find disk */
    if (!ide_present)
        return;

    /* read config file */
    /* setup disks */
    if (disk_read_part())
        return;

    disk_ok = 1;
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
