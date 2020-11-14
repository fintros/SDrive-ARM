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

#include "hwcontext.h"

// functions

#ifdef __cplusplus
extern "C"
{
#endif

    //! Initialize ARM<->MMC hardware interface.
    /// Prepares hardware for MMC access.
    void mmcInit(HWContext *ctx);

    //! Initialize the card and prepare it for use.
    /// Returns zero if successful.
    int mmcReset(HWContext *ctx);

    //! Read 512-byte sector from card to buffer
    /// Returns zero if successful.
    int mmcRead(HWContext *ctx, unsigned long sector);

    //! Write 512-byte sector from buffer to card
    /// Returns zero if successful.
    int mmcWrite(HWContext *ctx, unsigned long sector);

    int mmcWriteCached(HWContext *ctx, unsigned char force);

    void mmcWriteCachedFlush(HWContext *ctx);

    void mmcReadCached(HWContext *ctx, unsigned long sector);

#ifdef __cplusplus
}
#endif

#endif
