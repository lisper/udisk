/*
 * loader/loader.c
 *
 * Copyright (C) 2006 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include "at91.h"

// SAM-BA apears to write the following data to 0x201400
//
//        0x00: data0
//        0x01: data1
//                ...
// PAGE_SIZE-1: last data byte
// PAGE_SIZE  : page number to write


#define SAMBA_PAGE_BUFF ((uint32_t *) 0x201400)

int main( void )
{
  int i;
  uint32_t *page_buff = SAMBA_PAGE_BUFF;
  // keep this in units of 32 bit words
  int page_num = *(page_buff+PAGE_SIZE/4);
  int page_offset = page_num * PAGE_SIZE/4;

  // wait for mc to be ready
  while( !(mc.fsr & AT91_MC_FMR_FRDY) ) { }

  // write samba page buffer to flash buffer
  for( i=0 ; i < (PAGE_SIZE/4) ; i++ ) {
    AT91_FLASH_ADDR[ page_offset + i ] =  page_buff[i];
  }

  // write page
  mc.fcr = AT91_MC_FCR_FCMD_WP | AT91_MC_FCR_PAGEN(page_num) |  
    AT91_MC_FCR_KEY;

  // dont wait for mc to be ready here so that flash ops can stack
  
  return 0;
}
