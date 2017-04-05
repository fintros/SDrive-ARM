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

#include "../../../src/fat.h"
#include "../../../src/mmcsd.h"
#include "../../../src/HWContext.h"
#include "../../../src/dprint.h"
#include "../../../src/Launcher.h"
#include "../../../src/Launcher_PVT.h"

#if (CY_PSOC4)
    #if defined(__ARMCC_VERSION)
        __attribute__ ((section(".bootloaderruntype"), zero_init))
    #elif defined (__GNUC__)
        __attribute__ ((section(".bootloaderruntype")))
   #elif defined (__ICCARM__)
        #pragma location=".bootloaderruntype"
    #endif  /* defined(__ARMCC_VERSION) */
    volatile uint32 cyBtldrRunType;
#endif  /* (CY_PSOC4) */

__attribute__((noinline)) /* Workaround for GCC toolchain bug with inlining */
__attribute__((naked))
static void Launch(uint32 appAddr)
{
    __asm volatile("    BX  R0\n");
}

uint32 GetUint32Value(unsigned long fieldPtr)
{
    uint32 result =  (uint32) CY_GET_XTND_REG8((volatile uint8 *)(fieldPtr     ));
    result |= (uint32) CY_GET_XTND_REG8((volatile uint8 *)(fieldPtr + 1u)) <<  8u;
    result |= (uint32) CY_GET_XTND_REG8((volatile uint8 *)(fieldPtr + 2u)) << 16u;
    result |= (uint32) CY_GET_XTND_REG8((volatile uint8 *)(fieldPtr + 3u)) << 24u;        
    return result;    
}

uint16 GetUint16Value(unsigned long fieldPtr)
{
    uint16 result =  (uint16) CY_GET_XTND_REG8((volatile uint8 *)(fieldPtr     ));
    result |= (uint16) CY_GET_XTND_REG8((volatile uint8 *)(fieldPtr + 1u)) <<  8u;
    return result;    
}


void CheckLaunch(void) CYSMALL 
{
    if (0u == (Launcher_RES_CAUSE_REG & Launcher_RES_CAUSE_RESET_SOFT))
    {
        cyBtldrRunType = Launcher_SCHEDULE_BTLDR_INIT_STATE;
    }
    if (Launcher_GET_RUN_TYPE == Launcher_SCHEDULE_BTLDB)
    {
        Launch(GetUint32Value(Launcher_MD_BTLDB_ADDR_OFFSET(0)));
    }
}

// end of bootloader flash address - needed to protect itself
extern void* _etext;

int ProceedUpdate(int isForced)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    FileInfoStruct* fi = &ctx->file_info;
    
    // first - get metadata, to check version, appid and checksum?
	fi->vDisk.current_cluster=fi->vDisk.start_cluster;
	fi->vDisk.ncluster=0;

	faccess_offset(FILE_ACCESS_READ,0x7FC0,0x40); // read metadata
    
    uint16 currVersion = GetUint16Value(Launcher_MD_BTLDB_APP_VERSION_OFFSET(0));
    uint16 currAppID = GetUint16Value(Launcher_MD_BTLDB_APP_ID_OFFSET(0));
    uint32 currCustID = GetUint32Value(Launcher_MD_BTLDB_APP_CUST_ID_OFFSET(0));

    uint16 updatedVersion = GetUint16Value((uint32)(&ctx->atari_sector_buffer[0]) + Launcher_MD_BTLDB_APP_VERSION_OFFSET(0) - Launcher_MD_BASE_ADDR(0));
    uint16 updatedAppID = GetUint16Value((uint32)(&ctx->atari_sector_buffer[0]) + Launcher_MD_BTLDB_APP_ID_OFFSET(0) - Launcher_MD_BASE_ADDR(0));
    uint32 updatedCustID = GetUint32Value((uint32)(&ctx->atari_sector_buffer[0]) + Launcher_MD_BTLDB_APP_CUST_ID_OFFSET(0) - Launcher_MD_BASE_ADDR(0));

    dprint("Found update with version 0x%04X, AppID: 0x%04X, CustID: 0x%08X\r\n", updatedVersion, updatedAppID, updatedCustID);

    // skip update if forced but same and if id's are differ or version lower than existing one
    if( ((isForced>0) && ((currAppID != updatedAppID) || (currCustID != updatedCustID) || (updatedVersion != currVersion)))  || 
        ((currAppID == updatedAppID) && (currCustID == updatedCustID) && (updatedVersion > currVersion))
    )
    {
        dprint("Start update\r\n");
        ctx->LEDREG_Write(0);   // all on
        CyDelay(500);
       
        int startOffset = GetUint32Value(Launcher_MD_BTLDB_ADDR_OFFSET(0));
        // allign by row
             
        int startRowNum = startOffset / CY_FLASH_SIZEOF_ROW;
        int currentOffset = startRowNum*CY_FLASH_SIZEOF_ROW;
        
        if (currentOffset > (int) _etext)
        {
            unsigned int i;
            unsigned int ledState = 0;

            for(i = startRowNum; i < CY_FLASH_NUMBER_ROWS; i++)
            {
              	fi->vDisk.current_cluster=fi->vDisk.start_cluster;
                fi->vDisk.ncluster=0;
                if(faccess_offset(FILE_ACCESS_READ,currentOffset,CY_FLASH_SIZEOF_ROW))
                {
                    ledState = 1 - ledState;
                    if(ledState)
                        ctx->LEDREG_Write(~0x24);
                    else
                        ctx->LEDREG_Write(~0x18);
                        
                    CySysFlashWriteRow(i, &ctx->atari_sector_buffer[0]);
                    currentOffset+=CY_FLASH_SIZEOF_ROW;
                    dprint(".");
                }
                else
                {
                    dprint("\r\nUnable to read update file at address %d\r\n",currentOffset);
                    ctx->LEDREG_Write(~2);  // red on
                    CyDelay(300);
                    return 0;
                }
            }
            dprint("\r\n");
            ctx->LEDREG_Write(~0x1); // green;
            CyDelay(500);
            
            return 1;        
        }
        else
        {
            ctx->LEDREG_Write(~2);  // red on
            CyDelay(500);
            dprint("Unable to apply update - illegal start address\r\n");
        }
    }
    else
    {
        ctx->LEDREG_Write(~3);  // red + green on
        CyDelay(100);
        dprint("Skip update\r\n");
    }
    return 0;
}


int main(void)
{
    // bootloader or bootloadable?
    CheckLaunch();
    
    InitLoaderCTX();
    CyGlobalIntEnable; /* Enable global interrupts. */
#ifdef DEBUG    
    uint16 appVersion = GetUint16Value(Launcher_MD_BTLDB_APP_VERSION_OFFSET(0));
#endif
    HWContext* ctx = (HWContext*)__hwcontext;
   
    // switch on red - start of bootloader
    ctx->LEDREG_Write(~0x2);
    int ledState = 1;
    while (CardPresent_Read())
    { 
        CyDelay(200);
        ledState = 1-ledState;
        if(ledState) ctx->LEDREG_Write(~0x2); 
        else ctx->LEDREG_Write(~0); 
    }
    ctx->LEDREG_Write(~0x2);
    
    dprint("Bootloader start, app version 0x%04X\r\n", appVersion);
    
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

    // switch on green - card initialized and fat readed
    ctx->LEDREG_Write(~0x1);

	unsigned short i;

    // search for update first.
	i=0;
    int updateType = 0; // no update by default
	while( fatGetDirEntry(i,0) )
	{
        const char* fileName = (char*)&ctx->atari_sector_buffer[0];
        if(!strncmp("UPD", &fileName[8],3))
        {
            int j = (fileName[0] == '!')?1:0;       // forced update?
            if(!strncmp("SDRIVE", &fileName[j], 6))
            {
                updateType = j+1;
                break;
            }            
        }
        i++;
    }
    dprint("FAT readed with resulting updateType=%d\r\n", updateType);
    
    if(updateType > 0)
        ProceedUpdate(updateType-1);
        
    ctx->LEDREG_Write(~0);
    
    // start application
    cyBtldrRunType = Launcher_SCHEDULE_BTLDB;
    CySoftwareReset();
    
    return 0;
}

/* [] END OF FILE */
