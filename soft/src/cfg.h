/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _CFG_H
#define _CFG_H

#include "hwcontext.h"
#include "sdrive.h"

#define CONFIG_FILE_NAME "SDRIVE2 CFG"    
    
int DefaultSettings(Settings* settings);
int ReadSettings(HWContext* ctx, Settings* settings, unsigned char* buffer);
int WriteSettings(HWContext* ctx, Settings* settings, unsigned char* buffer);    
    
#endif
