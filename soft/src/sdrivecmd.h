/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
    
#ifndef _SDRIVEOPS_H
#define _SDRIVEOPS_H

#include "hwcontext.h"

typedef enum
{
   SDRGet20FileNames = 0xc0,
   SDRSetGastIO = 0xc1,
   SDRSetBLReloc = 0xc2,
   SDREmuInfo = 0xe0,
   SDRInitDrive = 0xe1,
   SDRDeactivateDrive = 0xe2,
   SDRSetDir = 0xe3,
   SDRGetFileDetails = 0xe5,
   SDRGetLongFileName256 = 0xe6,
   SDRGetLongFileName128 = 0xe7,
   SDRGetLongFileName64 = 0xe8,
   SDRGetLongFileName32 = 0xe9,
   SDRGetFilesCount = 0xea,
   SDRGetPath = 0xeb,
   SDRFindFirst = 0xec,
   SDRFindNext = 0xed,
   SDRChangeActualDrive = 0xee,
   SDRGetSharedParams = 0xef,
   SDRSetD00 = 0xf0,
   SDRSetD01 = 0xf1,
   SDRSetD02 = 0xf2,
   SDRSetD03 = 0xf3,
   SDRSetD04 = 0xf4,
   SDRSetD05 = 0xf5,
   SDRSetD06 = 0xf6,
   SDRSetD07 = 0xf7,
   SDRSetD08 = 0xf8,
   SDRSetD09 = 0xf9,
   SDRSetD10 = 0xfa,
   SDRSetD11 = 0xfb,
   SDRSetD12 = 0xfc,
   SDRDirUp  = 0xfd,
   SDRRootDir = 0xfe,
   SDRSetBoot = 0xff,
} EEmuCommand;

int InitBootDrive(HWContext* ctx, unsigned char* pBuffer);

int DriveCommand(HWContext* ctx, unsigned char* command, unsigned char* buffer);


#endif /* _SDRIVEOPS_H */