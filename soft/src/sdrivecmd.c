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
#include "dprint.h"
#include "diskemu.h"
#include "fat.h"
#include "helpers.h"
#include "mmcsd.h"


static int GetNext20FileNames(unsigned char* pBuffer, unsigned short usStartIndex)
{
    // Get 20 filenames from xhxl. 8.3 + attribute (11+1)*20+1 [<241] +1st byte of filename 21 (+1)
    
    StartReadOperation();
    dprint("Read 20 file names\r\n");
    send_ACK();       

    memset(pBuffer, 0, ATARI_BUFFER_SIZE);
    
    file_t* pBase = GetVDiskPtr(0);
    unsigned char* pNext = pBuffer;
    for(int i=0; i < 21; i++)
    {
        if(fatGetDirEntry(pBase , pNext, usStartIndex++,0))
        {   
            pNext[11] = pBase->fi.Attr; // Atribute in +11 byte
            pNext += 12;
        }
        else            
            break;
    }
    
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 241);    // to check - should be pNext - pBuffer
    return 0;
}

static int SetBLReloc(unsigned char reloc)
{   
    dprint("Set reloc\r\n");
    send_ACK();
    shared_parameters.bootloader_relocation = reloc;
	CyDelayUs(800u);	//t5
    send_CMPL();
    return 0;
}


uint8_t system_info[]="SDrive-ARM-02 20200912 by AlSp based on Bob!k & Raster, C.P.U.";	
//                                VV YYYYMMDD

static int GetEmuInfo(unsigned char* pBuffer, unsigned char count)
{   
    StartReadOperation();
    dprint("Get emu info\r\n");
    send_ACK();       

    if(!count)
    {
		CyDelayUs(800u);	//t5
        send_ERR();
        return 1;                   
    }
    
    memcpy(pBuffer, system_info, count);    
    
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, count);
    
    return 0;
}

int SetATRFlags(file_t* pDisk, unsigned char* pBuffer)
{
	if(faccess_offset(pDisk, FILE_ACCESS_READ, 0 , pBuffer, 16)) //ATR header read
    {
        pDisk->flags = FLAGS_DRIVEON;
    	if ( ( pBuffer[4] | pBuffer[5] ) == 0x01 ) {
    		pDisk->flags|=FLAGS_ATRDOUBLESECTORS;
    	} else {
    		if (pDisk->size == (((unsigned long)1040)*128 + 16)) {
    			pDisk->flags|=FLAGS_ATRMEDIUMSIZE;
    		}
    	}
        return 0;        
    }
    return 1;
}

// Mount file in current directory
int MountFile(file_t* pDisk, const char* pFileName, unsigned char* pBuffer)
{
    int i = 0;
    while(fatGetDirEntry(pDisk, pBuffer, i++, 0))
    {
        if(!strncmp((const char*) pBuffer, pFileName, 11))
        {
            dprint("%s found\r\n", pFileName);
			pDisk->current_cluster=pDisk->start_cluster;
			pDisk->ncluster=0;
            if(!strncmp(pFileName+8, "ATR", 3)) // if mount ATR
                return SetATRFlags(pDisk, pBuffer);
            return 0;                        
        }        
    }
    pDisk->flags = 0;   // not found
    
    return 0;
}

char DefaultBootFile[]="SDRIVE  ATR";

int InitBootDrive(unsigned char* pBuffer)
{
    LED_OFF;
	if ( !fatInit() ) 
	{
		LED_GREEN_OFF;
        CyDelayUs(1000u);
        return 1;
	}
    
    dprint("FAT FS inititialized\r\n");    
    
    return MountFile(GetVDiskPtr(0), DefaultBootFile, pBuffer);
}

static int InitEmu(unsigned char soft, unsigned char* pBuffer)
{
    dprint("Init emulator\r\n");
    send_ACK();       
    
    mmcWriteCachedFlush();

    CyDelayUs(800u);	//t5
	send_CMPL();
    
    if(!soft || InitBootDrive(pBuffer))
        return -1;      // full reinit cycle

    return 0;
}

int SetFastIO(unsigned char divider)
{
    send_ACK();       
    if (divider>US_POKEY_DIV_MAX)
    {
		CyDelayUs(800u);	//t5
        send_ERR();
        return 1;        
    }
    
    shared_parameters.fastsio_active = 0;
    shared_parameters.fastsio_pokeydiv = divider;

    CyDelayUs(800u);	//t5
	send_CMPL();    
    
    USART_Init(ATARI_SPEED_STANDARD);

    return 0;
}

int DeactivateDrive(unsigned char drive_no)
{
    send_ACK();       
    if (drive_no > DEVICESNUM)
    {
		CyDelayUs(800u);	//t5
        send_ERR();
        return 1;        
    }
    
    GetVDiskPtr(drive_no)->flags &= ~FLAGS_DRIVEON; 

    CyDelayUs(800u);	//t5
	send_CMPL();    

    return 0;
}

void PutCharAsDecimalNumber(unsigned char* pBuffer, unsigned char num)
{
	*pBuffer++=0x30|(num/10);
	*pBuffer=0x30|(num%10);
}


int FileSizeDateTimeToString(unsigned char* pBuffer, file_t* pDisk)
{
    //1234567890 YYYY-MM-DD HH:MM:SS
    int len = 30;
    memset(pBuffer, ' ', len); // all to spaces
    
    if(!(pDisk->fi.Attr & (ATTR_DIRECTORY|ATTR_VOLUME)))
        pBuffer[9] = '0';

    // fill size
    unsigned long size=pDisk->size;    
    for(int i = 9; i >=0; i--)
    {
        pBuffer[i] = '0' + (size % 10);
        size /= 10;
        if(!size)
            break;
    }
    
    // fill date
    unsigned short year = (pDisk->fi.Date >> 9) + 1980;
    pBuffer[15] = pBuffer[18] = '-';
    PutCharAsDecimalNumber(pBuffer+11, year/100);
    PutCharAsDecimalNumber(pBuffer+13, year%100);
    PutCharAsDecimalNumber(pBuffer+16, (pDisk->fi.Date>>5) & 0x0f);
    PutCharAsDecimalNumber(pBuffer+19, pDisk->fi.Date & 0x1f );

    // fill time
    pBuffer[24] = pBuffer[27] = ':';
    PutCharAsDecimalNumber(pBuffer+22, pDisk->fi.Time >> 11 );
    PutCharAsDecimalNumber(pBuffer+25, (pDisk->fi.Time >> 5) & 0x3f );
    PutCharAsDecimalNumber(pBuffer+28, (pDisk->fi.Time & 0x1f) << 1 ); //double seconds
    
    return len;
}

int GetFileDetails(unsigned char* pBuffer, unsigned short item)
{
    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);    
    int size = 50;
    send_ACK();
    if((pDisk->flags & FLAGS_DRIVEON) && fatGetDirEntry(pDisk, pBuffer, item, 0))
    {
        pBuffer[11] = pDisk->fi.Attr;
        *(unsigned long*)(pBuffer+12) = pDisk->size;
        *(unsigned short*)(pBuffer+16) = pDisk->fi.Date;
        *(unsigned short*)(pBuffer+18) = pDisk->fi.Time;
        FileSizeDateTimeToString(pBuffer + 20, pDisk);
    }
    else
        memset(pBuffer, 0, size);   // clean up
        
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int GetCurrentFileName(unsigned char* pBuffer, unsigned char drive_no)
{
    int size = 14;
    send_ACK();
    if (drive_no > DEVICESNUM)
    {
		CyDelayUs(800u);	//t5
        send_ERR();
        return 1;        
    }
    file_t* pDisk = GetVDiskPtr(drive_no);
    if((pDisk->flags & FLAGS_DRIVEON) && fatGetDirEntry(pDisk, pBuffer, pDisk->file_index, 0))
    {
        pBuffer[11] = pDisk->fi.Attr;
        *(unsigned short*)(pBuffer+12) = pDisk->file_index;        
    }
    else
        memset(pBuffer, 0, size); // clean up
        
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int GetLongFileName(unsigned char* pBuffer, unsigned short item, int size)
{
    send_ACK();
    
    memset(pBuffer, 0, ATARI_BUFFER_SIZE);
    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);
    if(! fatGetDirEntry(pDisk, pBuffer, item, 1))
    {
		CyDelayUs(800u);	//t5
        send_ERR();
        return 1;         
    }
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;
}

int GetFileCount(unsigned char* pBuffer)
{
    send_ACK();
   	unsigned short i = 0;
    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);
	while (fatGetDirEntry(pDisk, pBuffer, i, 0)) i++;    
    *(unsigned short*)(pBuffer) = i;
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 2);
    return 0;    
}

int GetPath(unsigned char* pBuffer, unsigned char max_no)
{
    send_ACK();

    if((max_no == 0) || (max_no > 20))
        max_no = 20;

    unsigned char count = max_no;
    
    memset(pBuffer, 0, ATARI_BUFFER_SIZE);
    
    unsigned char* pCurrent = pBuffer + count*12;    
    *pCurrent = 0;
    pCurrent -= 12;
 
    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);
    unsigned long old_cluster = pDisk->dir_cluster;
    unsigned short old_index = pDisk->file_index;
    while(fatGetDirEntry(pDisk, pCurrent, 0, 0) &&          // while we found directory
        (pDisk->fi.Attr & ATTR_DIRECTORY) &&
        (pCurrent[0] == '.') && (pCurrent[1] == '.'))
    {
        if(!count)
        {
            pCurrent[0] = '?';
            break;  
        }
        count--;
        unsigned long cur_cluster = pDisk->dir_cluster;
        
        pDisk->dir_cluster = pDisk->start_cluster;  // switch to up directory
       	unsigned short i = 0;
        for(i=0; fatGetDirEntry(pDisk, pCurrent+1, i,0); i++)
        {
            if(cur_cluster == pDisk->start_cluster) // name found
            {
                *pCurrent = '/';
                pCurrent -= 12;
                break;                
            }            
        }                
    }
    // restore
    pDisk->dir_cluster = old_cluster;
    pDisk->file_index = old_index;
    
    size_t len = (max_no - count) * 12;
    if(len)
        memcpy(pBuffer, pCurrent, len);
    memset(pBuffer + len, 0 , ATARI_BUFFER_SIZE-len);
    
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, max_no*12);
    
    return 0;
}

unsigned char _pFindPattern[11];

int FindNext(unsigned char* pBuffer, unsigned short index)
{
    int size = 14;
    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);
    while(fatGetDirEntry(pDisk, pBuffer, index, 0))
    {
        int j;
        for(j = 0; j < 11; j++)
        {
           if (!_pFindPattern[j]) continue; // if zero - any symbol is possible
           if(pBuffer[j] != _pFindPattern[j])
               break;
        }
        
        if(j == 11) // found
        {
            pBuffer[11] = pDisk->fi.Attr;
            *(unsigned short*)(pBuffer+12) = pDisk->file_index;
            USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
            return 0;
        }     
        index++;
    }
    memset(pBuffer, 0, size);
    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, size);
    return 0;    
}

int FindFirst(unsigned char* pBuffer, unsigned char bSkipSearch)
{
    if (USART_Get_buffer_and_check_and_send_ACK_or_NACK(&_pFindPattern[0],11))
    {
    	CyDelayUs(800u);	//t5
        send_ERR();
        return 1;        
    }
    if(!bSkipSearch)
        return FindNext(pBuffer, 0);        // to check different behaviour on exit with findnext
    
	CyDelayUs(800u);	//t5
    send_CMPL();
   
    return 0;
}

int ChangeActualDrive(unsigned char drive_no)
{
    send_ACK();
    
    // (if drive_no>=DEVICESNUM) Set default (all devices with relevant numbers).
    shared_parameters.actual_drive_number = drive_no < DEVICESNUM?drive_no:settings.emulated_drive_no;

    CyDelayUs(800u);	//t5
    send_CMPL();
   
    return 0;
    
}

int GetSharedParams(unsigned char size, unsigned char offset)
{
    send_ACK();
    USART_Send_cmpl_and_buffer_and_check_sum(((unsigned char*)(&shared_parameters)) + offset, size);    
    return 0;
}


int ChangeDirUp(unsigned char* pBuffer, unsigned char bGetName)
{
    send_ACK();

    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);

    if(fatGetDirEntry(pDisk, pBuffer, 0, 0) &&          // while we found directory
        (pDisk->fi.Attr & ATTR_DIRECTORY) &&
        (pBuffer[0] == '.') && (pBuffer[1] == '.'))
    {
        unsigned long cur_cluster = pDisk->dir_cluster;
        pDisk->dir_cluster = pDisk->start_cluster;  // switch to up directory
        
        if(bGetName)
        {
           	unsigned short i = 0;
            for(i=0; fatGetDirEntry(pDisk, pBuffer, i,0); i++)
            {
                if(cur_cluster == pDisk->start_cluster) // name found
                {
                    pBuffer[11] = pDisk->fi.Attr;
                    *(unsigned short*)(pBuffer+12) = pDisk->file_index;
                    USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 14);  // send name + attr + index
                    return 0;
                }            
            }
            memset(pBuffer, 0, 14);
            USART_Send_cmpl_and_buffer_and_check_sum(pBuffer, 14);  // send empty
            return 0;
        }       
    }
    CyDelayUs(800u);	//t5
    send_CMPL();
    
    return 0;
 }

int ChangeDirRoot(unsigned char* pBuffer)
{
    send_ACK();

    file_t* pDisk = GetVDiskPtr(shared_parameters.actual_drive_number);
    pDisk->dir_cluster = MSDOSFSROOT;
    fatGetDirEntry(pDisk, pBuffer, 0, 0);

    CyDelayUs(800u);	//t5
    send_CMPL();
    
    return 0;    
}


int MountDrive(unsigned char* pBuffer, unsigned char drive_no, unsigned short index)
{
    send_ACK();
    file_t* pDisk = GetVDiskPtr(drive_no);

    if(fatGetDirEntry(pDisk, pBuffer, index, 0))
    {
        if(pDisk->flags & ATTR_DIRECTORY)
            pDisk->dir_cluster = pDisk->start_cluster;
        else
        {
            pDisk->current_cluster = pDisk->start_cluster;
            pDisk->ncluster = 0;
			if( pBuffer[8]=='A' && pBuffer[9]=='T' && pBuffer[10]=='R' )        // ATR
                  SetATRFlags(pDisk, pBuffer); 
            else if( pBuffer[8]=='X' && pBuffer[9]=='F' && pBuffer[10]=='D' )   // XFD
            {				
				pDisk->flags=(FLAGS_DRIVEON|FLAGS_XFDTYPE);
				if ( pDisk->size>92160)
				{
				 	if ( pDisk->size<=133120)
						pDisk->flags|=FLAGS_ATRMEDIUMSIZE;
					else
						pDisk->flags |= FLAGS_ATRDOUBLESECTORS;
				}                
            }
            else
                pDisk->flags = FLAGS_DRIVEON|FLAGS_XEXLOADER|FLAGS_ATRMEDIUMSIZE;  // XEX            

            // apply readonly according filesystem
            if (pDisk->fi.Attr & ATTR_READONLY) {
				pDisk->flags |= FLAGS_READONLY;
			}            
        }
        CyDelayUs(800u);	//t5
        send_CMPL();        
        return 0;        
    }
    CyDelayUs(800u);	//t5
    send_ERR();        
    return 1;            
}

int DriveCommand(unsigned char* pCommand, unsigned char* pBuffer)
{   
    switch(pCommand[1])
    {
    case CmdWrite:
        // simulate write for boot disk
        return Write(GetVDiskPtr(0), pBuffer, *((unsigned short*)(pCommand+2)));        
    case CmdRead:
        // simulate read for boot disk
        return Read(GetVDiskPtr(0), pBuffer, *((unsigned short*)(pCommand+2)));
    case CmdStatus:
        // simulate status command for boot disk        
        return GetStatus(GetVDiskPtr(0));
    case SDRGet20FileNames:
        return GetNext20FileNames(pBuffer, *((unsigned short*)(pCommand+2)));
    case SDRSetGastIO:
        return SetFastIO(pCommand[2]);
    case SDRSetBLReloc:        
        return SetBLReloc(pCommand[2]);
    case SDREmuInfo:
        return GetEmuInfo(pBuffer, pCommand[2]);
        break;
    case SDRInitDrive:
        return InitEmu(pCommand[2], pBuffer);
    case SDRDeactivateDrive:
        return DeactivateDrive(pCommand[2]);
    case SDRSetDir:
        return GetCurrentFileName(pBuffer, pCommand[2]);
    case SDRGetFileDetails:
        return GetFileDetails(pBuffer, *((unsigned short*)(pCommand+2)));
    case SDRGetLongFileName256:
        return GetLongFileName(pBuffer, *((unsigned short*)(pCommand+2)), 256);
    case SDRGetLongFileName128:
        return GetLongFileName(pBuffer, *((unsigned short*)(pCommand+2)), 128);
    case SDRGetLongFileName64:
        return GetLongFileName(pBuffer, *((unsigned short*)(pCommand+2)), 64);
    case SDRGetLongFileName32:
        return GetLongFileName(pBuffer, *((unsigned short*)(pCommand+2)), 32);
    case SDRGetFilesCount:
        return GetFileCount(pBuffer);
    case SDRGetPath:
        return GetPath(pBuffer, pCommand[2]);
    case SDRFindFirst:
        return FindFirst(pBuffer, pCommand[3]);
    case SDRFindNext:
        send_ACK();
        return FindNext(pBuffer, *((unsigned short*)(pCommand+2)));
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
        return MountDrive(pBuffer, pCommand[1]-SDRSetD00, *((unsigned short*)(pCommand+2)));
        break;
    case SDRDirUp:
        return ChangeDirUp(pBuffer, pCommand[2]);
    case SDRRootDir:
        return ChangeDirRoot(pBuffer);
        break;
    default:
        CyDelayUs(800u);	//t5
        send_ERR();
        return 1;
    }    
}
