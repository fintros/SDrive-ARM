/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _XEXLOADER_H
#define _XEXLOADER_H

#include "atari.h"
#include "hwcontext.h"

#define XEX_SECTOR_SIZE 128

int ReadXEX(HWContext *ctx, file_t *pDisk, unsigned char *buffer, unsigned short sector);

#endif /* _XEXLOADER_H  */