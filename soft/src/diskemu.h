/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _DISKEMU_H
#define _DISKEMU_H

#include "project.h"
#include <CyLib.h>
#include "hwcontext.h"
#include "sdrive2.h"

typedef enum
{
    CmdFormat = 0x21,
    CmdFormatMedium = 0x22,
    CmdGetHighSpeedIndex = 0x3f,
    CmdReadPercom = 0x4e,
    CmdWritePercom = 0x4f,
    CmdWrite = 0x50,
    CmdRead = 0x52,
    CmdStatus = 0x53,
    CmdWriteVerify = 0x57
} EDriveEmu;

file_t *GetVDiskPtr(int no);

// don't attach motor state to disk no for a while
void StartMotor();
void StopMotor();

int SimulateDiskDrive(HWContext *ctx, unsigned char *pCommand, unsigned char *buffer);
int Read(HWContext *ctx, file_t *pDisk, unsigned char *buffer, unsigned short sector);
int Write(HWContext *ctx, file_t *pDisk, unsigned char *buffer, unsigned short sector);
int GetStatus(file_t *pDisk);

void ResetDrives();

#endif /* _DISKEMU_H  */