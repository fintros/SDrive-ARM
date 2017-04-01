/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "stdarg.h"

#include "../../../../src/fat.h"
#include "../../../../src/mmcsd.h"
#include "../../../../src/HWContext.h"
#include "../../../../src/dprint.h"

int main(void)
{
    InitLoaderCTX();
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    dprint("Bootloader star\r\n");
    
    do
	{
		mmcInit();
	}
	while(mmcReset());

    dprint("MMC inited\r\n"); 

    
   	while ( !fatInit() ) //Inicializace FATky
	{
       CyDelay(100u);
    }

    dprint("FAT inited\r\n");
    
	unsigned short i;

	i=0;
	while( fatGetDirEntry(i,0) )
	{
        //if(atari_sector_buffer[0] == 254)
        //{
        //   mmcWriteCached(0);
        //}
        i++;
    }
 
    dprint("FAT readed\r\n");
    
    Launcher_Start();

    dprint("Launcher start complete\r\n");
    
    return 0;
}

cystatus CyBtldrCommWrite(uint8* buffer, uint16 size, uint16* count, uint8 timeOut)
{
    return CYRET_UNKNOWN; 
}

cystatus CyBtldrCommRead (uint8* buffer, uint16 size, uint16* count, uint8 timeOut)
{
    return CYRET_UNKNOWN;
}

void CyBtldrCommStart(void)
{
}

void CyBtldrCommStop (void)
{
}

/* [] END OF FILE */
