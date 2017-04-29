/*
 * cli.c
 *                       
 * $Id: cli.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"

char rx_ring[16];
u_char rx_ring_head, rx_ring_tail, rx_ring_cnt;

#define CLI_LINE_MAX 32

char cli_line[CLI_LINE_MAX];
int cli_line_len;
int cli_state;
int cli_result;
int cli_disabled;

extern int debug;

void
putc(int c)
{
    if (c == '\n')
        usart_putc('\r');
    usart_putc(c);
}

int
puts(const char *str)
{
    int c;
    while ((c = *str++))
        putc(c);
    return 0;
}

void
help(void)
{
    printf("commands:\n");
}

int
toupper(int c)
{
    if ('a' <= c && c <= 'z')
        c -= 'a' - 'A';
    return c;
}

/*
 * parse a number (hex or decimal) from the the command line,
 * return the number
 * return ptr to chars after number
 */
int
parse_num(char **pp, int *v)
{
    char *p = *pp;
    int n = 0;
    char c;

    /* skip whitespace */
    while (*p && *p == ' ')
        p++;

    /* hex? */
    if (p[0] == '0' && (p[1] == 'x' | p[1] == 'X')) {
        /* hex */
        p += 2;
        while (*p) {
            c = toupper(*p++);
            if ('0' <= c && c <= '9')
                n = (n << 4) | (c - '0');
            else
                if ('A' <= c && c <= 'F')
                    n = (n << 4) | (c - 'A' + 10);
                else
                    break;
        }
    } else {
        /* decimal */
        while (*p) {
            c = *p++;
            if ('0' <= c && c <= '9')
                n = (n * 10) + c - '0';
            else
                break;
        }
    }

    *pp = p;
    *v = n;

    return 0;
}

int
parse_word(char **pp, char **pw)
{
    char *p = *pp;
    char c;

    /* skip whitespace */
    while (*p && *p == ' ')
        p++;

    /* start of word */
    *pw = p;
    while (*p) {
        if (*p == ' ')
            break;
        p++;
    }

    if (*p)
        *p++ = 0;

    *pp = p;

    return 0;
}

static unsigned short buffer[128];

void
cli_parse(void)
{
    char c1, c2, *p;
    int i, size;

#if 0
    printf("len  %d\n", cli_line_len);
    printf("line '%s'\n", cli_line);
#endif

    if (cli_line_len == 0)
        return;

    cli_result = 0;
    c1 = cli_line[0];
    c2 = cli_line[1];
    p = &cli_line[2];

    switch (c1) {
    case '?':
    case 'h':
        help();
        break;

    case 'u':
//        run_samba();
        break;

    default:
        puts("??\n");
    }

}

void
beep(void)
{
    putc(7);
}

void
cli_poll_rx_ring(void)
{
    char c;

    while (rx_ring_head != rx_ring_tail) {
        c = rx_ring[rx_ring_head];
        rx_ring_head = (rx_ring_head + 1) & 0xf;
        rx_ring_cnt--;

        /* otherwise we're in CLI mode */
        switch (c) {
        case '\r':
            cli_state = 2;
            break;
        case '\b':
        case 0x7f:
            if (cli_line_len == 0) {
                beep();
            } else {
                cli_line_len--;
                puts("\b \b");
            }
            break;
        default:
            if (cli_line_len < CLI_LINE_MAX) {
                cli_line[cli_line_len++] = c;
                putc(c);
            } else
                beep();
            break;
        }
    }
}

void
rx_ring_add(char c)
{
    rx_ring[rx_ring_tail] = c;
    rx_ring_tail = (rx_ring_tail + 1) & 0xf;
    rx_ring_cnt++;
}

int
rx_ring_space(void)
{
    return sizeof(rx_ring) - rx_ring_cnt;
}

void
cli_setup()
{
    cli_state = 0;
    cli_line_len = 0;
    cli_disabled = 0;
    rx_ring_tail = rx_ring_head = rx_ring_cnt = 0;
}

/*
 * poll the serial port, collecting characters into a line
 *
 * keeps track of state, printing prompt, etc...
 */
void
cli_poll(void)
{
    char c;

    /* add input chars to ring */
    if (usart_getc_rdy()) {
        c = usart_getc();
        rx_ring_add(c);
    }

    switch (cli_state) {
    case 0:
        puts("diag> ");
        cli_state = 1;
        break;
    case 1:
        cli_poll_rx_ring();
        break;
    case 2:
        puts("\n");
        cli_line[cli_line_len] = 0;
        cli_parse();

        cli_line_len = 0;
        cli_state = 0;
        break;
    }
}


char tohex(char b)
{
    b = b & 0xf;
    if (b < 10) return '0' + b;
    return 'a' + (b - 10);
}

void
dumpmem(char *ptr, int len)
{
    char line[80], chars[80], *p, b, *c, *end;
    int i, j, offset;

    offset = 0;
    end = ptr + len;
    while (ptr < end) {

	p = line;
	c = chars;
	printf("%3x ", offset);

	*p++ = ' ';
	for (j = 0; j < 16; j++) {
		if (ptr < end) {
			b = *ptr++;
			*p++ = tohex(b >> 4);
			*p++ = tohex(b);
			*p++ = ' ';
			*c++ = ' ' <= b && b <= '~' ? b : '.';
		} else {
			*p++ = 'x';
			*p++ = 'x';
			*p++ = ' ';
			*c++ = 'x';
		}
	}
	*p = 0;
	*c = 0;
        printf("%s %s\n",
	       line, chars);
        offset += 16;
    }
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
