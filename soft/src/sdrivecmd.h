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
   // = 0xd4,
   // = 0xd5,
   // = 0xd6,
   SDRSaveConfig = 0xd8,
   SDRLoadConfig = 0xd9,
   // = 0xda,
   // = 0xdb,
   // = 0xdc,
   // SDRDirUp = 0xdd,          // todo replace commands
   // SDRRootDir = 0xde,
   SDRSetTape = 0xdf,
   SDRGet20FileNames = 0xc0,
   SDRSetGastIO = 0xc1,
   SDRSetBLReloc = 0xc2,
   SDREmuInfo = 0xe0,
   SDRInitDrive = 0xe1,
   SDRDeactivateDrive = 0xe2,
   SDRGetCurrentFileName = 0xe3,    // p3 - 0 - 15 - disk drive, 16 - tape
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
   //SDRSetD13 = 0xfd,
   //SDRSetD14 = 0xfe,
   SDRDirUp  = 0xfd,
   SDRRootDir = 0xfe,
   SDRSetD15 = 0xff,
} EEmuCommand;

// Mount file in current directory
int MountFile(HWContext *ctx, file_t *pDisk, const char *pFileName, unsigned char *pBuffer);

// Execute drive command
int DriveCommand(HWContext *ctx, unsigned char *command, unsigned char *buffer);

// get file path to buffer
int GetPathString(HWContext *ctx, file_t* pFile, unsigned char *pBuffer, unsigned char max_no, unsigned char separator);

#endif /* _SDRIVEOPS_H */