/*
 * main.h
 *                       
 * $Id: main.h 65 2013-11-17 17:53:53Z brad $
 */

#ifndef main_h
#define main_h

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;

typedef unsigned short u_int16;
typedef unsigned long u_int32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define FIRMWARE_VER_MAJOR	0
#define FIRMWARE_VER_MINOR	1

#define TIMER0_INTERRUPT_LEVEL	1
#define USART_INTERRUPT_LEVEL	3
#define TIMER1_INTERRUPT_LEVEL	4

#define printf xprintf

#endif /* main_h */
