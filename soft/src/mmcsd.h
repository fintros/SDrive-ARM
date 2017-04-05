/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef MMCSD_H
#define MMCSD_H

typedef unsigned char u08;
typedef unsigned short u16;
typedef unsigned int u32;
// functions
    
#ifdef __cplusplus
extern "C" {    
#endif

//! Initialize ARM<->MMC hardware interface.
/// Prepares hardware for MMC access.
void mmcInit(void);

//! Initialize the card and prepare it for use.
/// Returns zero if successful.
u08 mmcReset(void);

//! Read 512-byte sector from card to buffer
/// Returns zero if successful.
u08 mmcRead(u32 sector);

//! Write 512-byte sector from buffer to card
/// Returns zero if successful.
u08 mmcWrite(u32 sector);


u08 mmcWriteCached(unsigned char force);

void mmcWriteCachedFlush();

void mmcReadCached(u32 sector);

#ifdef __cplusplus
}
#endif


#endif
