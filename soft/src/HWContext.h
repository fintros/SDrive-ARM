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

#ifdef __cplusplus
extern "C"
{
#endif

    typedef unsigned char u08;
    typedef unsigned short u16;
    typedef unsigned long u32;

    typedef struct _GlobalSystemValues //4+4+2+2+1=13 bytes
    {
        unsigned long FirstFATSector;
        unsigned long FirstDataSector;
        unsigned short RootDirSectors;
        unsigned short BytesPerSector;
        unsigned char SectorsPerCluster;
    } GlobalSystemValues;

    typedef struct _FatData
    {
        unsigned long last_dir_start_cluster;
        unsigned char last_dir_valid;
        unsigned short last_dir_entry;
        unsigned long last_dir_sector;
        unsigned char last_dir_sector_count;
        unsigned long last_dir_cluster;
        unsigned char last_dir_index;
        unsigned char fat32_enabled;
        unsigned long dir_cluster;
    } FatData;

#define ATARI_BUFFER_SIZE 256
#define MMC_BUFFER_SIZE 512

    typedef struct _HWContext
    {
        // SDCard clock

        void (*SPI_CLK_SetFractionalDividerRegister)(uint16 clkDivider, uint8 clkFractional);

        // SDCard SPI
        void (*SPIM_Start)(void);
        void (*SPIM_WriteTxData)(uint32 txData);
        uint32 (*SPIM_GetRxBufferSize)(void);
        uint32 (*SPIM_ReadRxData)(void);
        void (*SPIM_SS)(uint8 val);

        // data processing
        unsigned long n_actual_mmc_sector;
        unsigned char atari_sector_buffer[ATARI_BUFFER_SIZE] __attribute__((aligned(4)));
        unsigned char mmc_sector_buffer[MMC_BUFFER_SIZE] __attribute__((aligned(4)));
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
        void (*LEDREG_Write)(uint8 control);
        uint8 (*LEDREG_Read)(void);

        // readonly switch functions
        uint8 (*ReadOnly_Read)(void);

        // lib functions
        void (*Delay)(uint32 milliseconds);

        // global data
        GlobalSystemValues gsv;

        FatData fat_data;

        unsigned int _StaticVarsSize;

    } HWContext;

    extern void *__hwcontext;

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
