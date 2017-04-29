/*
 * usb.c
 * $Id: samba.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"
#include "usb.h"
#include "flash.h"

#define BUFFER_SIZE	512
char out[BUFFER_SIZE + 1];
int out_len;

#define PROGRAM_NAME	"udisk"
#define PROGRAM_VERSION	"0.2"

char *samba_sendfile_addr;
u32 usb_bulk_read_length;

static int interactive;

extern unsigned char buf[];

void samba_setup(void)
{
    interactive = 1;
}


/*
 * simple "SAM-BA(tm) Boot Assistant"-protocol implementation
 */
void samba(char *cmd)
{
    unsigned int addr, len, val;
    unsigned int i, i2, v;
    int prompt;
    char c[9];

    /* Parse hex values */
    parse_two_comma_separated_hex(cmd + 1, &i, &i2);
    if (0) printf("cmd %c i %x i2 %x\n", cmd[0], i, i2);

    /* Execute commands */
    out[0] = 0;
    out_len = 0;
    prompt = interactive ? 1 : 0;

    switch (cmd[0]) {
    case 0:
        /* empty command */
        break;

    case 'V':
        /* Version */
        cat_str(out, PROGRAM_NAME " " PROGRAM_VERSION);
        out_len = strlen(out);
        break;

    case 'w':
	/* read word */
        addr = i;
        v = *(unsigned int *)addr;
        if (interactive)
            cat_hex(out, v);
        else
            out_len = out_raw(out, out_len, 4, v);
        if (0) printf("[%x] -> %x\n", addr, *(unsigned int *)addr);
        break;
    case 'W':
        /* write word */
        addr = i;
        val = i2;
        *((u32 *)addr) = val;
        if (0) printf("[%x] <- %x\n", addr, val);
        break;

    case 'O':
        /* write byte */
        addr = i;
        val = i2;
        ///* write to last byte */
        //*((unsigned int *)addr) = ((~0xFF & *((unsigned int *)addr)) | val);
        *((u8 *)addr) = val;
        break;
    case 'o':
        /* read byte */
        addr = i;
        char c[9];
        c[0] = 0;
        v = *(u8 *)addr;
        if (interactive) {
            cat_hex(c, v);
            cat_str(out, (c + 6));
        } else
            out_len = out_raw(out, out_len, 1, v);
        break;

    case 'H':
        /* write half word */
        addr = i;
        val = i2;
        //*((unsigned int *)addr) = ((~0xFFFF & *((unsigned int *)addr)) | val);
        *((u16 *)addr) = val;
        break;
    case 'h':
        /* read half word */
        addr = i;
        c[0] = 0;
        v = *(u16 *)addr;
        if (interactive) {
            cat_hex(c, v);
            cat_str(out, (c + 4));
        } else
            out_len = out_raw(out, out_len, 2, v);
        break;
    case 'S':
        /* send file */
        samba_sendfile_addr = (char *)i;
        usb_bulk_read_length = i2;
        break;

    case 'R':
        /* read file */
        addr = i;
        len = i2;
        if (interactive) {
            usb_print("\n\r");
            usb_write((char *)addr, len);
            usb_print(">");
            prompt = 0;
        } else
            out_len = out_raw_ptr(out, out_len, addr, len);
        break;

    case 'G':
        /* goto */
        addr = i;
        ((void (*)(void))addr)();
        // Reset instead of goto:
        // *AT91C_RSTC_RCR = AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST | (0xa5 << 24);
        break;

    case 'N':
        /* non-interactive mode */
        if (interactive) {
            cat_str(out, "\n\r");
            out_len = strlen(out);
        }
        interactive = 0;
        break;

    case 'T':
        /* interactive mode */
        if (interactive) {
            cat_str(out, "\n\r");
            out_len = strlen(out);
        }
        interactive = 1;
        prompt = 1;
        break;

    default:
        /* unknown command */
        break;
    }

    if (interactive) {
        if (prompt)
            cat_str(out, "\n\r>");

        /* write response */
        if (out[0] != 0) {
            if (0) printf("resp: %s\n", out);
            usb_print(out);
        }
    } else {
        if (out_len > 0)
            usb_write(out, out_len);
    }
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
