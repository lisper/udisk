/*
 * main.c
 * $Id: main.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"

#define waitns(n)

/* reboot by */
void
reboot(void)
{
    AT91PS_MC pMC = (AT91PS_MC) AT91C_BASE_MC;
    AT91PS_RSTC pRSTC = AT91C_BASE_RSTC;

    /* toggle mapping of flash/sram to 0x0 */
    pMC->MC_RCR = AT91C_MC_RCB;

    /* reset */
    pRSTC->RSTC_RCR = 0xA5000000 |
        (AT91C_RSTC_PROCRST | AT91C_RSTC_PERRST | AT91C_RSTC_EXTRST);
}

int programmed(int region)
{
    u32 *p;

    switch (region) {
    case 0: p = (u32 *)0x100000; break;
    case 1: p = (u32 *)0x110000; break;
    case 2: p = (u32 *)0x120000; break;
    case 3: p = (u32 *)0x130000; break;
    }

    return *p != 0xffffffff;
}

void wait_us(void)
{
    volatile int i;
    for (i = 0; i < 1; i++)
        ;
}

void wait_ms(void)
{
    int i;
    for (i = 0; i < 1000; i++)
        wait_us();
}

int wait_1s(void)
{
    int i;
    for (i = 0; i < 1000; i++) {
        if (usart_getc_rdy())
            return 1;
        wait_ms();
    }

    return 0;
}

int wait(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (wait_1s())
            return 0;
        printf(".");
    }

    return 1;
}

void copy_and_run(int region)
{
    int i;
    u32 *p, *f, *t;
    void (*func)();

    switch (region) {
    case 0: f = (u32 *)0x100000; break;
    case 1: f = (u32 *)0x110000; break;
    case 2: f = (u32 *)0x120000; break;
    case 3: f = (u32 *)0x130000; break;
    }

    /* copy region to sram */
    t = (u32 *)0x200000;
    p = t;
    for (i = 0; i < 0x10000/4; i++)
        *t++ = *f++;

    /* and jump to it */
    func = (void (*)(void))p;
    (*func)();
}

main()
{
    debug_setup();
    usart_setup();
    flash_setup();
    usb_setup();
    led_setup();
    samba_setup();

#if 0
    while (1) {
        putc('A');
    }
#endif

    puts("\nudisk loader v0.2\n");

    usb_open();

    /* turn on interrupts */
    enable_interrupts();

    if (programmed(1) && wait()) {
        printf("booting\n");
        copy_and_run(1);
    }

    printf("entering usb loader\n");

    while (1) {
        cli_poll();
        usb_poll();
        led_poll();
    }
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
