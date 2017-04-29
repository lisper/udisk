/*
 * pio.c
 *                       
 * $Id: pio.c 49 2010-10-10 14:10:10Z brad $
 */

#include "main.h"
#include "board.h"

void
pio_pins_pio_mode(int pins)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    /* configure PIO pins to pio mode */
    pPio->PIO_PER = pins;

}

void
pio_pins_in(int pins)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    pPio->PIO_PPUDR = pins;
    pPio->PIO_CODR = pins;
    pPio->PIO_MDDR = pins;
    pPio->PIO_ODR = pins;
    pPio->PIO_OWDR = pins;
}

void
pio_pins_pullup(int pins)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;
    pPio->PIO_PPUER = pins;
}

void
pio_pins_out(int pins)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    pPio->PIO_PPUDR = pins;
    pPio->PIO_CODR = pins;
    pPio->PIO_MDDR = pins;
    pPio->PIO_OER = pins;
    pPio->PIO_OWER = pins;
}

void
pio_pins_peripheral_mode(int a_pins, int b_pins)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    /* configure PIO pins to periph mode */
    pPio->PIO_ASR = a_pins;
    pPio->PIO_BSR = b_pins;
    pPio->PIO_PDR = a_pins | b_pins;
}

