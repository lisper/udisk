/*
 * board.h
 *
 * $Id: board.h 65 2013-11-17 17:53:53Z brad $
 */

#ifndef board_h
#define board_h

#include "AT91SAM7S128.h"
#define __inline inline

#define AT91_BAUD_RATE	9600   /* Baud rate */

/*
 * Master Clock
 */
#define EXT_OC		18432000   /* external ocilator MAINCK */
#define MCK		47923200   /* MCK (PLLRC div by 2) */
#define MCKKHz		(MCK/1000)

/*
 * external bus
 */

#define P_D0		AT91C_PIO_PA0
#define P_D1		AT91C_PIO_PA1
#define P_D2		AT91C_PIO_PA2
#define P_D3		AT91C_PIO_PA3
#define P_D4		AT91C_PIO_PA4
#define P_D5		AT91C_PIO_PA5
#define P_D6		AT91C_PIO_PA6
#define P_D7		AT91C_PIO_PA7

#define P_D8		AT91C_PIO_PA8
#define P_D9		AT91C_PIO_PA9
#define P_D10		AT91C_PIO_PA10
#define P_D11		AT91C_PIO_PA11
#define P_D12		AT91C_PIO_PA12
#define P_D13		AT91C_PIO_PA13
#define P_D14		AT91C_PIO_PA14
#define P_D15		AT91C_PIO_PA15

#define P_USB_PUP	AT91C_PIO_PA16

#define P_A0		AT91C_PIO_PA17
#define P_A1		AT91C_PIO_PA18
#define P_A2		AT91C_PIO_PA19
#define P_A3		AT91C_PIO_PA20

#define P_BSSYN_L	AT91C_PIO_PA27
#define P_BMSYN_L	AT91C_PIO_PA28
#define P_BBBSY_L	AT91C_PIO_PA29
//cpld clk - pa31

#define P_CPLD_RD	AT91C_PIO_PA23
#define P_CPLD_WR	AT91C_PIO_PA24
#define P_CPLD_INT	AT91C_PIO_PA25

#endif /* board_h */
