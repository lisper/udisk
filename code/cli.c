/*
 * cli.c
 *                       
 * $Id: cli.c 70 2013-11-17 17:59:13Z brad $
 */

#include "main.h"
#include "board.h"
#include "bus.h"
#include "cpld.h"

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

    case '+':
        debug++;
        printf("debug=%d\n", debug);
        break;

    case '-':
        if (debug > 0) debug--;
        printf("debug=%d\n", debug);
        break;

    case 'b':
        switch (c2) {
        case 'g': bus_wiggle_npg(); break;
        case '5': bus_wiggle_br5(); break;
        case 'D': bus_wiggle_dma(); break;
        case 'd': bus_wiggle_data(); break;
        case 'l': bus_listen(); break;
        case 'f': bus_force(); break;
        case 's': bus_show(); break;
        case 'm': bus_match(); break;
        case 't': bus_match_test(); break;
        case 'r': bus_reset(); break;
        case 'w':
            bus_wiggle();
            bus_reset();
            break;
        }
        break;

    case 'c':
        show_cpld_regs();
        break;

    case 'D':
        switch (c2) {
        case 'w':
            size = 4/*128*/;
            for (i = 0; i < size; i++) 
                buffer[i] = (i << 9) | i;
            printf("start dma\n");
            unibus_dma_buffer(1, 0x0, buffer, size);
            printf("done\n");
            break;
        case 'r':
            for (i = 0; i < size; i++) 
                buffer[i] = 0;
            unibus_dma_buffer(0, 0x0, buffer, size);
            for (i = 0; i < size; i += 4) {
                printf("%4x %4x %4x %4x\n",
                       buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
            }
            break;
        }
        break;

    case 'g':
        switch (c2) {
        case '1': load_boot1(); break;
        case '2': load_boot2(); break;
        case '3': load_boot3(); break;
        case 'i': load_int(); break;
        case 'r': load_int_rti(); break;
        case 'p': load_poll(*p ? p[0] - '0' : -1); break;
        case 'm': load_regs(); break;
        }
        break;

    case 'd':
        switch (c2) {
        case 's': test_dma_simple(); break;
        case 'r': test_dma_read(); break;
        case 'f': test_dma_fill(); break;
        case 'c': test_dma_cont(); break;
        case 't':
//            while (1) {
                test_dma();
//            }
            break;
        }
        break;

    case 't':
        switch (c2) {
        case 'i': test_cause_int(); break;
        case 'c': test_int_cont(); break;
        case 'r': test_cpld_read(); break;
        case 'w': test_cpld_write(); break;
        case 'm': test_cpld_msyn(); break;
        case 'x': test_cpld_io(); break;
        }
        break;

    case 'i':
        switch (c2) {
        case 'd': ide_en(0); break;
        case 'e': ide_en(1); break;
        case 'i': ide_identify_drive(); break;
        case 'b': ide_read_block_cont(); break;
        case '0': ide_read_block(0); break;
        case '1': ide_read_block(1); break;
        case 'm': ide_read_reg_cont(); break;
        case 'r': ide_show_regs(); break;
        case 'w': ide_wiggle_reset(); break;
        case 'x': ide_assert_reset(); break;
        case 's':
            ide_setup();
            disk_setup();
            break;
        }
        break;

    case 'l':
        switch (c2) {
        case 'w': test_led_wiggle(); break;
        case '0': test_led_set(0); break;
        case '1': test_led_set(1); break;
        case 's': log_show(); break;
        case 'r': log_reset(); break;
        }
        break;

    case 's':
        show_stats();
        break;

    case 'r':
        controller_restart();
        break;

    case 'u':
        run_updater();
        break;

    case 'x':
        switch (c2) {
        case 'x':
            cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
            irq_clear(AT91C_ID_PIOA);
            break;
        case 'r':
            reboot();
            break;
        }
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
