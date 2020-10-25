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
#include "sdrive.h"

int mount_tape(HWContext *ctx, file_t * folder, Settings* settings, unsigned char *buffer);
void proceed_tape(HWContext* ctx, unsigned char *buffer);

#endif /* _TAPEEMU_H */
