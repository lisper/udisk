/*
 * led.c
 * $Id: led.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"
#include "../code3.new/bus.h"
#include "../code3.new/cpld.h"

void delayus(void)
{
    volatile int i;
    for (i = 0;i < 10; i++);
}

#include "../code3.new/bus.c"

unsigned int led_count;
unsigned int led_state;

void
led_setup(void)
{
    bus_setup();
    cpld_reset();
}

void
led_poll(void)
{
    led_count++;
    //cpld_write_subreg(CPLD_SUBREG_CF_ENA, 0);

    if (led_count == 0) {
        led_state = ~led_state;
        cpld_write_subreg(CPLD_SUBREG_LED, led_state ? 1 : 0);
    }
}



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
