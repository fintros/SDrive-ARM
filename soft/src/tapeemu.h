/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _TAPEEMU_H
#define _TAPEEMU_H

#include "hwcontext.h"
#include "sdrive2.h"

int MountTape(HWContext *ctx, file_t *folder, const char *file_name, Settings *settings, unsigned char *buffer);
void ProceedTape(HWContext *ctx, unsigned char *buffer);
file_t * GetMountedTape();

#endif /* _TAPEEMU_H */
