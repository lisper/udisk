/*
 * flash.c
 * $Id: flash.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"
#include "flash.h"

void nvm_init(void)
{
}

void flash_setup(void)
{
}

void flash_init(void)
{
    //* Set number of Flash Waite sate
    //  SAM7S64 features Single Cycle Access at Up to 30 MHz
    //  if MCK = 47923200, 72 Cycles for 1 µseconde ( field MC_FMR->FMCN)
    AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&(72 <<16)) | AT91C_MC_FWS_1FWS ;
}

int flash_ready(void)
{
    unsigned int status = 0;

    /* wait for the end of command */
    while ((status & AT91C_MC_FRDY) != AT91C_MC_FRDY) {
        status = AT91C_BASE_MC->MC_FSR;
    }

    return status;
}

int flash_lock_status(void)
{
    return (AT91C_BASE_MC->MC_FSR & AT91C_MC_FSR_LOCK);
}

int flash_lock(unsigned int Flash_Lock_Page)
{
    AT91PS_MC ptMC = AT91C_BASE_MC;

    nvm_init();

    disable_interrupts();

    /* write the Set Lock Bit command */
    ptMC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_LOCK | (AT91C_MC_PAGEN & (Flash_Lock_Page << 8) ) ;

    /* wait for the end of command */
    flash_ready();

    enable_interrupts();

    return flash_lock_status();
}

int flash_unlock(unsigned int Flash_Lock_Page)
{
    nvm_init();

    disable_interrupts();

    /* write the Clear Lock Bit command */
    AT91C_BASE_MC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_UNLOCK | (AT91C_MC_PAGEN & (Flash_Lock_Page << 8) ) ;

    /* wait for the end of command */
    flash_ready();

    enable_interrupts();

    return flash_lock_status();
}


int flash_erase_all(void)
{
    AT91PS_MC ptMC = AT91C_BASE_MC;

    flash_init();

    disable_interrupts();

    /* write the Erase All command */
    ptMC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_ERASE_ALL ;

    /* wait for the end of command */
    flash_ready();

    enable_interrupts();

    /* check the result */
    return (ptMC->MC_FSR & ( AT91C_MC_PROGE | AT91C_MC_LOCKE )) == 0;
}

int flash_write( unsigned int Flash_Address ,int size ,unsigned int * buff)
{
    //* set the Flash controller base address
    AT91PS_MC ptMC = AT91C_BASE_MC;
    unsigned int i, page, status;
    volatile u32 *flash;

    flash = (u32 *)Flash_Address;

    flash_init();

    /* get the Flash page number */
    page = ((Flash_Address - (unsigned int)AT91C_IFLASH ) /FLASH_PAGE_SIZE_BYTE);

    for (i=0; (i < FLASH_PAGE_SIZE_BYTE) & (size > 0); i++) {
        *flash++ = *buff++;
        size -= 4;
    }

    disable_interrupts();

    /* Write the write page command */
    ptMC->MC_FCR = AT91C_MC_CORRECT_KEY | AT91C_MC_FCMD_START_PROG | (AT91C_MC_PAGEN & (page <<8)) ;

    /* Wait for the end of command */
    status = flash_ready();

    enable_interrupts();

    //* Check the result
    if ( (status & ( AT91C_MC_PROGE | AT91C_MC_LOCKE ))!=0)
        return 0;

    return 1;
}

void flash_read( unsigned int Flash_Address ,int size ,unsigned int * buff)
{
    unsigned int i;
    u32 *flash;
	
    flash = (u32 *)Flash_Address;
		
    for(i = 0; i < (unsigned int) size; i++) {
        *buff++ = *flash++;
    }
}

int flash_blank_check(unsigned int * start, unsigned int size)
{
    unsigned int i;

    /* check if flash is erased */
    for (i = 0; i < (size/4); i++) {
        if (start[i] != ERASE_VALUE) return 0;
    }

    return 1;
}

int flash_write_all( unsigned int Flash_Address ,int size ,unsigned int * buff)
{

    int   next, status;
    unsigned int  dest,page;
    unsigned int * src;

    dest = Flash_Address;
    src = buff;
    status = 1;
	

    while( (status == 1) & (size > 0) )
    {
        //* Check the size
        if (size <= FLASH_PAGE_SIZE_BYTE) next = size;
        else next = FLASH_PAGE_SIZE_BYTE;
        page = (dest - (unsigned int)AT91C_IFLASH ) /FLASH_PAGE_SIZE_BYTE;
        //* Unlock current sector base address - current address by sector size
	flash_unlock(page);
        //* Write page and get status
        status = flash_write( dest ,next ,src);
        // * get next page param
        size -= next;
        src += FLASH_PAGE_SIZE_BYTE/4;
        dest +=  FLASH_PAGE_SIZE_BYTE;
    }

    return status;
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
