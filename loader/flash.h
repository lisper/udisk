#ifndef __FLASH_H
#define __FLASH_H

/*-------------------------------*/
/* Flash Status Field Definition */
/*-------------------------------*/

#define AT91C_MC_FSR_MVM  ((unsigned int) 0xFF << 8)	// (MC) Status Register GPNVMx: General-purpose NVM Bit Status
#define AT91C_MC_FSR_LOCK ((unsigned int) 0xFFFF << 16)	// (MC) Status Register LOCKSx: Lock Region x Lock Status

#define	 ERASE_VALUE 		0xFFFFFFFF

/*-----------------------*/
/* Flash size Definition */
/*-----------------------*/

#ifdef AT91SAM7S64
/* For Devices with 128Byte Pages (SAM7S64)*/
	#define FLASH_PAGE_SIZE_BYTE 	128
	#define FLASH_PAGE_SIZE_LONG	32
#else
 /* 256 Kbytes of Internal High-speed Flash, Organized in x Pages of 256 Bytes */
/* For Devices with 256Byte Pages (SAM7S128, SAM7S256) */
	#define FLASH_PAGE_SIZE_BYTE 	256
	#define FLASH_PAGE_SIZE_LONG	64
#endif

#define  FLASH_LOCK_BITS_SECTOR	16
#define  FLASH_SECTOR_PAGE		32
#define  FLASH_LOCK_BITS		16    /* 16 lock bits, each protecting 16 sectors of 32 pages*/
#define  FLASH_BASE_ADDRESS		0x00100000

#define AT91C_MC_CORRECT_KEY  ((unsigned int) 0x5A << 24) // (MC) Correct Protect Key


#endif /* FLASH_H */
