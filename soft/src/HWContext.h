/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _HW_CONTEXT_H_
#define _HW_CONTEXT_H_

#include <cytypes.h>
#include "atari.h"
#include "fat.h"

#ifdef __cplusplus
extern "C" {    
#endif

    
typedef struct _GlobalSystemValues			//4+4+2+2+1=13 bytes
{
	unsigned long FirstFATSector;
	unsigned long FirstDataSector;
	unsigned short RootDirSectors;
	unsigned short BytesPerSector;
	unsigned char SectorsPerCluster;
} GlobalSystemValues;


typedef struct _HWContext
{
    // SDCard clock
    
    void (*SPI_CLK_SetFractionalDividerRegister)(uint16 clkDivider, uint8 clkFractional);
    
    
    // SDCard SPI
    void (*SPIM_Start)(void); 
    void  (*SPIM_WriteTxData)(uint32 txData);
    uint32 (*SPIM_GetRxBufferSize)(void);
    uint32 (*SPIM_ReadRxData)(void);
    
    // data processing 
    unsigned long n_actual_mmc_sector;
    unsigned char atari_sector_buffer[256] __attribute__ ((aligned (4)));
    unsigned char mmc_sector_buffer[512] __attribute__ ((aligned (4)));
    unsigned char n_actual_mmc_sector_needswrite;

    unsigned long sd_init_freq;
    unsigned long sd_work_freq;
    
    // SDAccess object
    void *sdAccess;
    
    // debug UART references
    char *print_buffer;
    int print_buffer_len;
    void (*Debug_PutArray)(const uint8 wrBuf[], uint32 count);
    
    // led access
    void    (*LEDREG_Write)(uint8 control);
    uint8   (*LEDREG_Read)(void);
    
    // readonly switch functions
    uint8   (*ReadOnly_Read)(void);
     
    // lib functions
    void  (*Delay)(uint32 milliseconds);
    
    // global data
    GlobalSystemValues gsv;
    
    FatData fat_data;
    
    FileInfoStruct file_info;			//< file information for last file accessed
    
    unsigned int _StaticVarsSize;
    
} HWContext;

extern void* __hwcontext;

#ifdef LOADER
void InitLoaderCTX(void);
#else
void InitLoadableCTX(void);
#endif

#ifdef __cplusplus
}    
#endif

#endif

/* [] END OF FILE */
