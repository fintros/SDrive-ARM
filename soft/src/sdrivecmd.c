/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "sdrivecmd.h"
#include "sio.h"
#include "gpio.h"
#undef DEBUG
#include "dprint.h"
#include "diskemu.h"
#include "fat.h"
#include "helpers.h"
#include "mmcsd.h"
#include "atx.h"

static file_t _browse;

static int GetNext20FileNames(HWContext *ctx, unsigned char *pBuffer, unsigned short usStartIndex)
{
    // Get 20 filenames from xhxl. 8.3 + attribute (11+1)*20+1 [<241] +1st byte of filename 21 (+1)

    StartReadOperation();
    dprint("Read 20 file names\r\n");
    send_ACK();

    memset(pBuffer, 0, ATARI_BUFFER_SIZE);

    unsigned char *pNext = pBuffer;
    for (int i = 0; i < 21; i++)
    {
        if (fatGetDirEntry(ctx, &_browse, pNext, usStartIndex++, 0))
        {
            pNext[11] = _browse.fi.Attr; // Atribute in +11 byte
            pNext += 12;
        }
        else
            break;
    }

    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 241); // to check - should be pNext - pBuffer
    return 0;
}

static int SetBLReloc(unsigned char reloc)
{
    dprint("Set reloc\r\n");
    send_ACK();
    settings.shared_parameters.bootloader_relocation = reloc;
    CyDelayUs(800u); //t5
    send_CMPL();
    return 0;
}

static const uint8_t system_info[] = "SDrive-ARM-02 20200912 by AlSp based on Bob!k & Raster, C.P.U.";
//                                VV YYYYMMDD

static int GetEmuInfo(unsigned char *pBuffer, unsigned char count)
{
    StartReadOperation();
    dprint("Get emu info\r\n");
    send_ACK();

    if (!count)
    {
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }

    memcpy(pBuffer, system_info, count);

    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, count);

    return 0;
}

static int InitEmu(HWContext *ctx, unsigned char soft, unsigned char *pBuffer)
{
    dprint("Init emulator\r\n");
    send_ACK();

    mmcWriteCachedFlush(ctx);

    CyDelayUs(800u); //t5
    send_CMPL();

    //    if(!soft || InitBootDrive(ctx, pBuffer))
    //        return -1;      // full reinit cycle

    return 0;
}

int SetFastIO(unsigned char divider)
{
    send_ACK();
    if (divider > US_POKEY_DIV_MAX)
    {
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }

    settings.shared_parameters.fastsio_active = 0;
    settings.shared_parameters.fastsio_pokeydiv = divider;

    CyDelayUs(800u); //t5
    send_CMPL();

    USART_Init(ATARI_SPEED_STANDARD);

    return 0;
}

int DeactivateDrive(unsigned char drive_no)
{
    send_ACK();
    if (drive_no > DEVICESNUM)
    {
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }

    GetVDiskPtr(drive_no)->flags &= ~FLAGS_DRIVEON;

    CyDelayUs(800u); //t5
    send_CMPL();

    return 0;
}

void PutCharAsDecimalNumber(unsigned char *pBuffer, unsigned char num)
{
    *pBuffer++ = 0x30 | (num / 10);
    *pBuffer = 0x30 | (num % 10);
}

int FileSizeDateTimeToString(unsigned char *pBuffer, file_t *pDisk)
{
    //1234567890 YYYY-MM-DD HH:MM:SS
    int len = 30;
    memset(pBuffer, ' ', len); // all to spaces

    if (!(pDisk->fi.Attr & (ATTR_DIRECTORY | ATTR_VOLUME)))
        pBuffer[9] = '0';

    // fill size
    unsigned long size = pDisk->size;
    for (int i = 9; i >= 0; i--)
    {
        pBuffer[i] = '0' + (size % 10);
        size /= 10;
        if (!size)
            break;
    }

    // fill date
    unsigned short year = (pDisk->fi.Date >> 9) + 1980;
    pBuffer[15] = pBuffer[18] = '-';
    PutCharAsDecimalNumber(pBuffer + 11, year / 100);
    PutCharAsDecimalNumber(pBuffer + 13, year % 100);
    PutCharAsDecimalNumber(pBuffer + 16, (pDisk->fi.Date >> 5) & 0x0f);
    PutCharAsDecimalNumber(pBuffer + 19, pDisk->fi.Date & 0x1f);

    // fill time
    pBuffer[24] = pBuffer[27] = ':';
    PutCharAsDecimalNumber(pBuffer + 22, pDisk->fi.Time >> 11);
    PutCharAsDecimalNumber(pBuffer + 25, (pDisk->fi.Time >> 5) & 0x3f);
    PutCharAsDecimalNumber(pBuffer + 28, (pDisk->fi.Time & 0x1f) << 1); //double seconds

    return len;
}

int GetFileDetails(HWContext *ctx, unsigned char *pBuffer, unsigned short item)
{
    int size = 50;
    send_ACK();
    if ((_browse.flags & FLAGS_DRIVEON) && fatGetDirEntry(ctx, &_browse, pBuffer, item, 0))
    {
        pBuffer[11] = _browse.fi.Attr;
        *(unsigned long *)(pBuffer + 12) = _browse.size;
        *(unsigned short *)(pBuffer + 16) = _browse.fi.Date;
        *(unsigned short *)(pBuffer + 18) = _browse.fi.Time;
        FileSizeDateTimeToString(pBuffer + 20, &_browse);
    }
    else
        memset(pBuffer, 0, size); // clean up

    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int GetCurrentFileName(HWContext *ctx, unsigned char *pBuffer, unsigned char drive_no)
{
    int size = 14;
    send_ACK();
    if (drive_no > DEVICESNUM)
    {
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }
    file_t *pDisk = GetVDiskPtr(drive_no);
    if ((pDisk->flags & FLAGS_DRIVEON) && fatGetDirEntry(ctx, pDisk, pBuffer, pDisk->file_index, 0))
    {
        pBuffer[11] = pDisk->fi.Attr;
        *(unsigned short *)(pBuffer + 12) = pDisk->file_index;
    }
    else
        memset(pBuffer, 0, size); // clean up

    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int GetLongFileName(HWContext *ctx, unsigned char *pBuffer, unsigned short item, int size)
{
    send_ACK();

    memset(pBuffer, 0, ATARI_BUFFER_SIZE);
    if (!fatGetDirEntry(ctx, &_browse, pBuffer, item, 1))
    {
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int GetFileCount(HWContext *ctx, unsigned char *pBuffer)
{
    send_ACK();
    unsigned short i = 0;
    while (fatGetDirEntry(ctx, &_browse, pBuffer, i, 0))
        i++;
    *(unsigned short *)(pBuffer) = i;
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 2);
    return 0;
}

// return length of path
int GetPathString(HWContext *ctx, file_t* pFile, unsigned char *pBuffer, unsigned char max_no, unsigned char separator)
{

    if ((max_no == 0) || (max_no > 20))
    max_no = 20;

    unsigned char count = max_no;

    memset(pBuffer, separator, count*12);

    unsigned char *pCurrent = pBuffer + count * 12;

    unsigned long old_cluster = pFile->dir_cluster;
    unsigned short old_index = pFile->file_index;
    unsigned char file_name[12];
    size_t len = 0;
    while (fatGetDirEntry(ctx, pFile, &file_name[0], 0, 0) && // while we found directory
           (pFile->fi.Attr & ATTR_DIRECTORY) &&
           (file_name[0] == '.') && (file_name[1] == '.'))
    {
        if (!count)
        {
            pCurrent[0] = '?';
            break;
        }
        count--;
        unsigned long cur_cluster = pFile->dir_cluster;

        pFile->dir_cluster = pFile->start_cluster; // switch to up directory
        unsigned short i = 0;
        for (i = 0; fatGetDirEntry(ctx, pFile, &file_name[0], i, 0); i++)
        {
            if (cur_cluster == pFile->start_cluster) // name found
            {
                pCurrent -= 12;
                memcpy(pCurrent + 1, &file_name[0], 11);
                *pCurrent = '/';
                len += 12;
                break;
            }
        }
    }
    // restore
    pFile->dir_cluster = old_cluster;
    pFile->file_index = old_index;

    if (len)
        memcpy(pBuffer, pCurrent, len);
    
    return len;
}

int GetPath(HWContext *ctx, unsigned char *pBuffer, unsigned char max_no)
{
    send_ACK();
    
    int len = GetPathString(ctx, &_browse, pBuffer, max_no, 0);
    memset(pBuffer + len, 0, ATARI_BUFFER_SIZE - len);

    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, max_no * 12);

    return 0;
}

unsigned char _pFindPattern[11];

int FindNext(HWContext *ctx, unsigned char *pBuffer, unsigned short index)
{
    int size = 14;
    while (fatGetDirEntry(ctx, &_browse, pBuffer, index, 0))
    {
        int j;
        for (j = 0; j < 11; j++)
        {
            if (!_pFindPattern[j])
                continue; // if zero - any symbol is possible
            if (pBuffer[j] != _pFindPattern[j])
                break;
        }

        if (j == 11) // found
        {
            pBuffer[11] = _browse.fi.Attr;
            *(unsigned short *)(pBuffer + 12) = _browse.file_index;
            USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
            return 0;
        }
        index++;
    }
    memset(pBuffer, 0, size);
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int FindFirst(HWContext *ctx, unsigned char *pBuffer, unsigned char bSkipSearch)
{
    send_ACK();
    if (USART_Get_buffer_and_check_and_send_ACK_or_NACK(&_pFindPattern[0], 11))
    {
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }
    if (!bSkipSearch)
        return FindNext(ctx, pBuffer, 0); // to check different behaviour on exit with findnext

    CyDelayUs(800u); //t5
    send_CMPL();

    return 0;
}

int ChangeActualDrive(unsigned char drive_no)
{
    send_ACK();

    // (if drive_no>=DEVICESNUM) Set default (all devices with relevant numbers).
    settings.shared_parameters.actual_drive_number = drive_no < DEVICESNUM ? drive_no : settings.emulated_drive_no;
    set_display(settings.shared_parameters.actual_drive_number);

    CyDelayUs(800u); //t5
    send_CMPL();

    return 0;
}

int GetSharedParams(unsigned char size, unsigned char offset)
{
    send_ACK();
    USART_Send_cmpl_and_buffer_and_check_sum(((unsigned char *)(&settings.shared_parameters)) + offset, size);
    return 0;
}

int ChangeDirUp(HWContext *ctx, unsigned char *pBuffer, unsigned char bGetName)
{
    send_ACK();

    if (fatGetDirEntry(ctx, &_browse, pBuffer, 0, 0) && // while we found directory
        (_browse.fi.Attr & ATTR_DIRECTORY) &&
        (pBuffer[0] == '.') && (pBuffer[1] == '.'))
    {
        unsigned long cur_cluster = _browse.dir_cluster;
        _browse.dir_cluster = _browse.start_cluster; // switch to up directory

        if (bGetName)
        {
            unsigned short i = 0;
            for (i = 0; fatGetDirEntry(ctx, &_browse, pBuffer, i, 0); i++)
            {
                if (cur_cluster == _browse.start_cluster) // name found
                {
                    pBuffer[11] = _browse.fi.Attr;
                    *(unsigned short *)(pBuffer + 12) = _browse.file_index;
                    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 14); // send name + attr + index
                    return 0;
                }
            }
            memset(pBuffer, 0, 14);
            USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 14); // send empty
            return 0;
        }
    }
    CyDelayUs(800u); //t5
    send_CMPL();

    return 0;
}

int ChangeDirRoot(HWContext *ctx, unsigned char *pBuffer)
{
    send_ACK();

    _browse.dir_cluster = GetRootDirCluster();
    fatGetDirEntry(ctx, &_browse, pBuffer, 0, 0);
    _browse.flags |= FLAGS_DRIVEON;

    CyDelayUs(800u); //t5
    send_CMPL();

    return 0;
}

// prepare file name in 8+3 format and return next folder/file pointer
const char *ConvertFileName(const char *pFileName, char *pConvFileName)
{
    memset(&pConvFileName[0], ' ', 11);
    pConvFileName[11] = 0;    
    int i = 0, j = 0;
    if(pFileName[i] == '/')
        pFileName++;
    for (; j < 11; i++, j++)
    {
        if (pFileName[i] == '.')
        {
            i++;
            j = 8;
        }
        if (pFileName[i] == '/' || pFileName[i] == 0)
            break;
        pConvFileName[j] = pFileName[i];
    }
    if (pFileName[i] == '/')
        pFileName += i + 1;
    else
        pFileName = 0;

    return pFileName;
}

int SetATRFlags(HWContext *ctx, file_t *pDisk, unsigned char *pBuffer)
{
    if (faccess_offset(ctx, pDisk, FILE_ACCESS_READ, 0, pBuffer, 16)) //ATR header read
    {
        pDisk->flags = FLAGS_DRIVEON;
        if ((pBuffer[4] | pBuffer[5]) == 0x01)
        {
            pDisk->flags |= FLAGS_ATRDOUBLESECTORS;
        }
        else
        {
            if (pDisk->size == (((unsigned long)1040) * 128 + 16))
            {
                pDisk->flags |= FLAGS_ATRMEDIUMSIZE;
            }
        }
        return 0;
    }
    return 1;
}

// pBuffer Should contain file name
int FileFlagsSettings(HWContext *ctx, file_t *pDisk, unsigned char *pBuffer, int delay_enabled)
{
    pDisk->status = 0xff; // reset status
    pDisk->current_cluster = pDisk->start_cluster;
    pDisk->ncluster = 0;
    if (pBuffer[8] == 'A' && pBuffer[9] == 'T' && pBuffer[10] == 'R') // ATR
        SetATRFlags(ctx, pDisk, pBuffer);
    else if (pBuffer[8] == 'X' && pBuffer[9] == 'F' && pBuffer[10] == 'D') // XFD
    {
        pDisk->flags = (FLAGS_DRIVEON | FLAGS_XFDTYPE);
        if (pDisk->size > 92160)
        {
            if (pDisk->size <= 133120)
                pDisk->flags |= FLAGS_ATRMEDIUMSIZE;
            else
                pDisk->flags |= FLAGS_ATRDOUBLESECTORS;
        }
    }
    else if (pBuffer[8] == 'A' && pBuffer[9] == 'T' && pBuffer[10] == 'X') // ATX
    {
        if (delay_enabled)
        {
            CyDelayUs(800u); //t5
            send_CMPL();
        }
        loadAtxFile(ctx, pDisk, pBuffer);
        pDisk->flags |= FLAGS_DRIVEON | FLAGS_ATXTYPE;

        return 1;
    }
    else
        pDisk->flags = FLAGS_DRIVEON | FLAGS_XEXLOADER | FLAGS_ATRMEDIUMSIZE; // XEX

    // apply readonly according filesystem
    if (pDisk->fi.Attr & ATTR_READONLY)
    {
        pDisk->flags |= FLAGS_READONLY;
    }
    return 0;
}

// pFileName should be deffer with pBuffer
int MountFile(HWContext *ctx, file_t *pDisk, const char *pFileName, unsigned char *pBuffer)
{
    // repeat for folders
    while (pFileName)
    {
        char pCurFileName[12];
        int i = 0;
        pFileName = ConvertFileName(pFileName, &pCurFileName[0]);
        while (fatGetDirEntry(ctx, pDisk, pBuffer, i++, 0))
        {
            if (!strncasecmp((const char *)pBuffer, pCurFileName, 11))
            {
                dprint("%s found\r\n", pCurFileName);
                if (pDisk->fi.Attr & ATTR_DIRECTORY)
                {
                    pDisk->dir_cluster = pDisk->start_cluster;
                    break;
                }
                else
                {
                    FileFlagsSettings(ctx, pDisk, pBuffer, 0);
                    return 0;
                }
            }
        }
    }
    pDisk->flags = 0; // not found

    return 1;
}

int MountDrive(HWContext *ctx, unsigned char *pBuffer, file_t *pDisk, unsigned short index)
{
    send_ACK();

    if (fatGetDirEntry(ctx, &_browse, pBuffer, index, 0))
    {
        if (_browse.fi.Attr & ATTR_DIRECTORY)
            _browse.dir_cluster = _browse.start_cluster;
        else
        {
            *pDisk = _browse;
            if (FileFlagsSettings(ctx, pDisk, pBuffer, 1))
                return 0;
        }
        CyDelayUs(800u); //t5
        send_CMPL();
        return 0;
    }
    CyDelayUs(800u); //t5
    send_ERR();
    return 1;
}

int DriveCommand(HWContext *ctx, unsigned char *pCommand, unsigned char *pBuffer)
{
    switch (pCommand[1])
    {
    case CmdWrite:
        // simulate write for boot disk
        return Write(ctx, GetVDiskPtr(0), pBuffer, *((unsigned short *)(pCommand + 2)));
    case CmdRead:
        // simulate read for boot disk
        return Read(ctx, GetVDiskPtr(0), pBuffer, *((unsigned short *)(pCommand + 2)));
    case CmdStatus:
        // simulate status command for boot disk
        return GetStatus(GetVDiskPtr(0));
    case SDRGet20FileNames:
        return GetNext20FileNames(ctx, pBuffer, *((unsigned short *)(pCommand + 2)));
    case SDRSetGastIO:
        return SetFastIO(pCommand[2]);
    case SDRSetBLReloc:
        return SetBLReloc(pCommand[2]);
    case SDREmuInfo:
        return GetEmuInfo(pBuffer, pCommand[2]);
        break;
    case SDRInitDrive:
        return InitEmu(ctx, pCommand[2], pBuffer);
    case SDRDeactivateDrive:
        return DeactivateDrive(pCommand[2]);
    case SDRSetDir:
        return GetCurrentFileName(ctx, pBuffer, pCommand[2]);
    case SDRGetFileDetails:
        return GetFileDetails(ctx, pBuffer, *((unsigned short *)(pCommand + 2)));
    case SDRGetLongFileName256:
        return GetLongFileName(ctx, pBuffer, *((unsigned short *)(pCommand + 2)), 256);
    case SDRGetLongFileName128:
        return GetLongFileName(ctx, pBuffer, *((unsigned short *)(pCommand + 2)), 128);
    case SDRGetLongFileName64:
        return GetLongFileName(ctx, pBuffer, *((unsigned short *)(pCommand + 2)), 64);
    case SDRGetLongFileName32:
        return GetLongFileName(ctx, pBuffer, *((unsigned short *)(pCommand + 2)), 32);
    case SDRGetFilesCount:
        return GetFileCount(ctx, pBuffer);
    case SDRGetPath:
        return GetPath(ctx, pBuffer, pCommand[2]);
    case SDRFindFirst:
        return FindFirst(ctx, pBuffer, pCommand[3]);
    case SDRFindNext:
        send_ACK();
        return FindNext(ctx, pBuffer, *((unsigned short *)(pCommand + 2)));
    case SDRChangeActualDrive:
        return ChangeActualDrive(pCommand[2]);
    case SDRGetSharedParams:
        return GetSharedParams(pCommand[2], pCommand[3]);
    case SDRSetD00:
    case SDRSetD01:
    case SDRSetD02:
    case SDRSetD03:
    case SDRSetD04:
    case SDRSetD05:
    case SDRSetD06:
    case SDRSetD07:
    case SDRSetD08:
    case SDRSetD09:
    case SDRSetD10:
    case SDRSetD11:
    case SDRSetD12:
    case SDRSetBoot:
    {
        unsigned short drive_no = pCommand[1] - SDRSetD00;
        if (drive_no > DEVICESNUM)
            drive_no = 0;
        return MountDrive(ctx, pBuffer, GetVDiskPtr(drive_no), *((unsigned short *)(pCommand + 2)));
    }
    break;
    case SDRDirUp:
        return ChangeDirUp(ctx, pBuffer, pCommand[2]);
    case SDRRootDir:
        return ChangeDirRoot(ctx, pBuffer);
        break;
    default:
        dprint("!!!!Unknown command %X\r\n", pCommand[1]);
        CyDelayUs(800u); //t5
        send_ERR();
        return 1;
    }
}
