/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */


#include "diskemu.h"
#include "sio.h"
#include "gpio.h"
#include "dprint.h"
#include "xexloader.h"
#include "helpers.h"
#include "fat.h"
#include "atx.h"

static void FormatInternal(HWContext* ctx, file_t* pDisk, int size, unsigned char* buffer)
{   
    int formaterror = 0;
	pDisk->fi.percomstate=0; // after the format, percom loses its value

	// Skip formatting XEX and readonly images
	if (pDisk->flags & (FLAGS_XEXLOADER | FLAGS_READONLY))
	{
    	send_NACK();
        return;
	}
    
    send_ACK();	
    StartWriteOperation();
    memset(buffer, 0, ATARI_BUFFER_SIZE); // clear buffer

    unsigned long offset = 0, psize;
    if ( !(pDisk->flags & FLAGS_XFDTYPE) ) 
        offset += 16;
    while( (psize = (pDisk->size - offset))>0 )
    {
        if (psize>ATARI_BUFFER_SIZE) 
            psize=ATARI_BUFFER_SIZE;
        if (!faccess_offset(ctx, pDisk, FILE_ACCESS_WRITE, offset, buffer, psize))
        {
            formaterror=1;
            break;
        }
        offset += psize;
    }
	
	// fill buffer with 0xff - in return of format command, zeroes first two bytes in case of error
    memset(buffer, 0xff, size);
    if (formaterror)
    {
        buffer[0]= buffer[1] = 0x00; //err
        pDisk->flags |= FLAGS_WRITEERROR;
    }
    
    USART_Send_cmpl_and_buffer_and_check_sum(buffer, size);
    
    StopWriteOperation();
}


// format single + PERCOM
// 	Single for 810 and stock 1050 or as defined by CMD $4F for upgraded drives (Speedy, Happy, HDI, Black Box, XF 551).
// Returns 128 bytes (SD) or 256 bytes (DD) after format. Format ok requires that the first two bytes are $FF. It is said to return bad sector list (never seen one).
static int Format(HWContext* ctx, file_t* pDisk, unsigned char* buffer)
{
    unsigned long singleSize = 92160;
    
    // In case of XFD type move to 16 bytes to be the same like ATR
    if ( !(pDisk->flags & FLAGS_XFDTYPE) ) 
        singleSize+=16;

    if (( pDisk->fi.percomstate == 3 ) // tried to write a mismatched percom
           || ( (pDisk->fi.percomstate==0) && (pDisk->size != singleSize) ) ) // non single size disk
    {
        pDisk->flags |= FLAGS_WRITEERROR;
        send_NACK();
    }
    else
        FormatInternal(ctx, pDisk, pDisk->fi.percomstate==2 ? 256 : 128, buffer); 
        
    return 0;
}

// 	Formats medium density on an Atari 1050. Format medium density cannot be achieved via PERCOM block settings!
static int FormatMedium(HWContext* ctx, file_t* pDisk, unsigned char* buffer)
{
    // error in case of non medium or readonly disk
    if ((pDisk->flags & (FLAGS_ATRMEDIUMSIZE | FLAGS_READONLY) ) != FLAGS_ATRMEDIUMSIZE )
    {
        pDisk->flags |= FLAGS_WRITEERROR;
        send_NACK();
    }
    else
        FormatInternal(ctx, pDisk, 128, buffer);

    return 0;
}

static int GetHighSpeedIndex(file_t* pDisk)
{
    // use standard speed for ATX
    unsigned char divisor = (pDisk->flags & FLAGS_ATXTYPE)?US_POKEY_DIV_STANDARD:shared_parameters.fastsio_pokeydiv;
    StartReadOperation();
    dprint("Ask for speed\r\n");
    send_ACK();
    USART_Send_cmpl_and_buffer_and_check_sum(&divisor, 1);
    return 0;
}


typedef struct _PercomBlock
{
    unsigned char number_of_tracks;
    unsigned char step_rate;
    unsigned char sectors_per_track_msb;
    unsigned char sectors_per_track_lsb;
    unsigned char number_of_sides;
    unsigned char density;      /* 0 - FM, 4 - MFM */
    unsigned char sectors_size_msb;
    unsigned char sectors_size_lsb;
    unsigned char drive_present;
    unsigned char unused[3];    
} __attribute__((packed)) PercomBlock;

typedef struct _PercomItem
{
    unsigned int size;
    PercomBlock block;
} PercomItem;


static PercomItem percom_table[] = {
// 720*128=92160	   (  90,000KB)
  { .size = 92160, 
    .block = {
        .number_of_tracks = 40,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 18,
        .number_of_sides = 0,
        .density = 0,
        .sectors_size_msb = 0,
        .sectors_size_lsb = 128
    }   
  },
// 1040*128=133120         ( 130,000KB)
  { .size = 133120, 
    .block = {
        .number_of_tracks = 40,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 26,
        .number_of_sides = 0,
        .density = 4,
        .sectors_size_msb = 0,
        .sectors_size_lsb = 128
    }   
  },
// 3*128+717*256=183936    ( 179,625KB)
  { .size = 183936, 
    .block = {
        .number_of_tracks = 40,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 18,
        .number_of_sides = 0,
        .density = 4,
        .sectors_size_msb = 1,
        .sectors_size_lsb = 0
    }   
  },
// 3*128+1437*256=368256   ( 359,625KB)
  { .size = 368256, 
    .block = {
        .number_of_tracks = 40,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 18,
        .number_of_sides = 1,
        .density = 4,
        .sectors_size_msb = 1,
        .sectors_size_lsb = 0
    }   
  },
// 3*128+2877*256=736896   ( 719,625KB)
  { .size = 736896, 
    .block = {
        .number_of_tracks = 80,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 18,
        .number_of_sides = 1,
        .density = 4,
        .sectors_size_msb = 1,
        .sectors_size_lsb = 0
    }   
  },
// 3*128+5757*256=1474176  (1439,625KB)
  { .size = 1474176, 
    .block = {
        .number_of_tracks = 80,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 36,
        .number_of_sides = 1,
        .density = 4,
        .sectors_size_msb = 1,
        .sectors_size_lsb = 0
    }   
  },
// 3*128+11517*256=2948736 (2879,625KB)
  { .size = 2948736, 
    .block = {
        .number_of_tracks = 80,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 72,
        .number_of_sides = 1,
        .density = 4,
        .sectors_size_msb = 1,
        .sectors_size_lsb = 0
    }   
  },
  // IDE
  { .size = 0, 
    .block = {
        .number_of_tracks = 1,
        .step_rate = 1,
        .sectors_per_track_msb = 0,
        .sectors_per_track_lsb = 0,
        .number_of_sides = 0,
        .density = 4,
        .sectors_size_msb = 1,
        .sectors_size_lsb = 0
    }   
  }
};

/*
byte 0:    Tracks per side (0x01 for HDD, one "long" track)
byte 1:    Interface version (0x10)
byte 2:    Sectors/Track - high byte
byte 3:    Sectors/Track - low byte
byte 4:    Side Code (0 - single sided)
byte 5:    Record method (0=FM, 4=MFM double)
byte 6:    High byte of Bytes/Sector (0x01 for double density)
byte 7:    Low byte of Bytes/Sector (0x00 for double density)
byte 8:    Translation control (0x40)
          bit 6: Always 1 (to indicate drive present)
bytes 9-11 Drive interface type string "IDE"
*/
static int ReadPercom(file_t* pDisk, unsigned char* buffer)
{
    unsigned long size = pDisk->size;
    unsigned long search_size = size & 0xffffff80; // skip ATR headers
    unsigned int secsize = pDisk->flags & FLAGS_ATRDOUBLESECTORS? 0x100:0x80;
    int i;

    StartReadOperation();
    dprint("Ask for Percom\r\n");
    send_ACK();   
    
    if(pDisk->flags & FLAGS_XEXLOADER)
        i = sizeof(percom_table)/sizeof(PercomItem) - 1;
    else
        for(i = 0; percom_table[i].size != 0; i++)
            if((percom_table[i].size == search_size) && 
                percom_table[i].block.sectors_size_msb == (secsize>>8))
                break;
    
    memcpy(buffer, &percom_table[i].block, sizeof(PercomItem));
    
    if(percom_table[i].size == 0)
    {
        if ( pDisk->flags & FLAGS_XEXLOADER )
        {
         secsize = 125; //-=3; //=125
         size += 46125;		//((u32)0x171*(u32)125);
        }
        if ( pDisk->flags & FLAGS_ATRDOUBLESECTORS )
        {
         size+=3*128; //+384 bytu za prvni tri sektory s velikosti 128
        }

        //FOURBYTESTOLONG(atari_sector_buffer+8)=fs; //DEBUG !!!!!!!!!!!

        size /= secsize; //prepocet filesize na pocet sektoru

        buffer[2] = ((size>>8) & 0xff);	//hb sectors
        buffer[3] = (size & 0xff );		//lb sectors
        buffer[4] = ((size>>16) & 0xff); // sides
    }
    
    buffer[8]  = 0x01;	//0x01; //0x40; // drive exist
    buffer[9]  = 'S';	//0x41;	//0xc0; //'I'; 
    buffer[10] = 'D';	//0x00;	//0xf2; //'D'; 
    buffer[11] = 'r';	//0x00;	//0x42; //'E'; 

    USART_Send_cmpl_and_buffer_and_check_sum(buffer, 12);
    
    return 0;
}

static int WritePercom(file_t* pDisk, unsigned char* buffer)
{
    StartWriteOperation();
    dprint("Write Percom\r\n");
    send_ACK();       
    
    if ( ! USART_Get_buffer_and_check_and_send_ACK_or_NACK(buffer,12) )
    {
        // restore size from received Percom block
        unsigned int size = ((unsigned int)buffer[0])
        * ( (unsigned int) ( (((unsigned int)buffer[2])<<8) + ((unsigned int)buffer[3]) ) )
        * (((unsigned int)buffer[4])+1)
        * ( (unsigned int) ( (((unsigned int)buffer[6])<<8) + ((unsigned int)buffer[7]) ) );
        
        if ( !(pDisk->flags & FLAGS_XFDTYPE) ) size += 16; // ATR 16 byte header
        if ( pDisk->flags & FLAGS_ATRDOUBLESECTORS ) size -=384; // first 3 sektors in double dencity disks only 128 bytes
        if (size == pDisk->size)
        {
            pDisk->fi.percomstate=1;		//ok
            if ( (buffer[6]|buffer[7])==0x01) 
                pDisk->fi.percomstate++;  //=2 ok (double)
        }
        else
            pDisk->fi.percomstate=3;	//percom write bad

		CyDelayUs(800u);	//t5
		send_CMPL();        
    }    
    return 0;
}

static unsigned char motor_state = 0;

int GetStatus(file_t* pDisk)
{
    unsigned char buffer[4];
    StartReadOperation();
    dprint("Ask for status\r\n");
    send_ACK();      
    
    buffer[0] = motor_state>0?0x10:0;	 //0x00 motor off	0x10 motor on
    if (pDisk->flags & FLAGS_ATRMEDIUMSIZE) buffer[0]|=0x80;	// medium/single
    if (pDisk->flags & FLAGS_ATRDOUBLESECTORS) buffer[0]|=0x20;	// double/normal sector size
    if (pDisk->flags & FLAGS_WRITEERROR)
    {
        buffer[0]|=0x04;			                            //Write error status bit        
        // clear write error flag after reporting
        pDisk->flags &= ~FLAGS_WRITEERROR;
    }
    if (get_readonly() || (pDisk->flags & FLAGS_READONLY)) buffer[0]|=0x08;	//write protected bit

    buffer[1] = pDisk->status;
    buffer[2] = 0xe0; 		//(244s) timeout
    buffer[3] = 0x00;		// 

    USART_Send_cmpl_and_buffer_and_check_sum(buffer, 4);

    dprint("Send status: [%02X %02X %02X %02X]\r\n", 
        buffer[0], 
        buffer[1],
        buffer[2],
        buffer[3]);
   
    return 0;   
}


// ATR or XFD
static unsigned int GetATROffset(file_t* pDisk, unsigned short sector, unsigned int* pSectorSize)
{
    unsigned int offset;
    if(sector<4)
    {
        // sectors 1 to 3
        *pSectorSize = 128;
        offset = (sector-1) * 128;
    }
    else
    {
        // sectors 4 and next
        *pSectorSize = (pDisk->flags & FLAGS_ATRDOUBLESECTORS)? 256:128;
        offset = (sector-4) * *pSectorSize + 384;
    }

    //ATR header
    if (! (pDisk->flags & FLAGS_XFDTYPE) ) 
        offset += 16;
    
    return offset;         
}

static void DumpBuffer(unsigned char* buffer, unsigned int len)
{
    for(unsigned int i = 0; i < len; i+=16)
    {
        dprint("%02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
            buffer[i],
            buffer[i+1],
            buffer[i+2],
            buffer[i+3],
            buffer[i+4],
            buffer[i+5],
            buffer[i+6],
            buffer[i+7],
            buffer[i+8],
            buffer[i+9],
            buffer[i+10],
            buffer[i+11],
            buffer[i+12],
            buffer[i+13],
            buffer[i+14],
            buffer[i+15]                        
        );
    }            
}

static int ReadATR(HWContext* ctx, file_t* pDisk, unsigned char* buffer, unsigned short sector)
{
    unsigned int sector_size;
    unsigned int offset = GetATROffset(pDisk, sector, &sector_size);    
    unsigned int proceeded_bytes = faccess_offset(ctx, pDisk, FILE_ACCESS_READ, offset, buffer, sector_size);
    dprint("Read result %d, offset=%d, secsize=%d\r\n", proceeded_bytes,offset, sector_size);
       
    if(proceeded_bytes)
        USART_Send_cmpl_and_buffer_and_check_sum(buffer, sector_size);
    else
    {
        CyDelayUs(800u);	//t5
        send_ERR();
    }        

#if 0
    DumpBuffer(buffer, proceeded_bytes);
#endif
    
    return 0;
}

static int ReadATX(HWContext* ctx, file_t* pDisk, unsigned char* buffer, unsigned short sector)
{
    unsigned short sector_size;
    unsigned int proceeded_bytes = loadAtxSector(ctx, pDisk, buffer, sector, &sector_size);

   
    if(proceeded_bytes)
        USART_Send_status_and_buffer_and_check_sum(buffer, sector_size, 0, 0);
    else
        USART_Send_status_and_buffer_and_check_sum(buffer, sector_size, 1, 0);

#if 0
    DumpBuffer(buffer, proceeded_bytes);
#endif
    
    return 0;    
}

int Read(HWContext* ctx, file_t* pDisk, unsigned char* buffer, unsigned short sector)
{
    StartReadOperation();
    StartMotor();
    send_ACK();       
    dprint("Read sector\r\n");
    
    pDisk->status = 0xff;   // reset status
    if(pDisk->flags & FLAGS_XEXLOADER)
        return ReadXEX(ctx, pDisk, buffer, sector);
    else if(pDisk->flags & FLAGS_ATXTYPE)
        return ReadATX(ctx, pDisk, buffer, sector);
    else
        return ReadATR(ctx, pDisk, buffer, sector);
 
    return 0;
}

static int WriteATR(HWContext* ctx, file_t* pDisk, unsigned char* buffer, unsigned short sector)
{
    unsigned int sector_size;
    unsigned int offset = GetATROffset(pDisk, sector, &sector_size);

	//write do image
	if (!USART_Get_buffer_and_check_and_send_ACK_or_NACK(buffer, sector_size))
	{
        CyDelayUs(800u);	//t5
    	if ( get_readonly() || 
             (pDisk->flags & FLAGS_READONLY) || 
             faccess_offset(ctx, pDisk, FILE_ACCESS_WRITE, offset, buffer, sector_size) == 0)
        {
            send_ERR();
        }
        else
        {
            send_CMPL();
        }
	}           
    return 0;
}


int Write(HWContext* ctx, file_t* pDisk, unsigned char* buffer, unsigned short sector)
{
    StartWriteOperation();
    StartMotor();
    dprint("Write sector\r\n");
    send_ACK();       

    pDisk->status = 0xff;   // reset status
    if(pDisk->flags & (FLAGS_XEXLOADER | FLAGS_ATXTYPE))
    {
        if ( ! USART_Get_buffer_and_check_and_send_ACK_or_NACK(buffer, XEX_SECTOR_SIZE) )
        {
            // not support ATX or XEX writing
            CyDelayUs(800u);	//t5
            send_ERR();
        }                
    }
    else
        return WriteATR(ctx, pDisk, buffer, sector);
 
    return 0;
}

static file_t virtual_disk[DEVICESNUM];

file_t* GetVDiskPtr(int no)
{
    if(no >= DEVICESNUM)
        return 0;
    
    return &virtual_disk[no];
}

void ResetDrives()
{    
    FatData* f = &((HWContext*)__hwcontext)->fat_data;
    
    memset(&virtual_disk[0], 0, sizeof(file_t)*DEVICESNUM);
    for(int i = 0; i < DEVICESNUM; i++)
    {
        virtual_disk[i].dir_cluster = f->dir_cluster;
        virtual_disk[i].status = 0xff;
    }
}

CY_ISR(MotorISR)
{
#if 0    
    dprint("MotorISR\r\n");
#endif 
    if(motor_state)
        motor_state++;
    
    if(motor_state > 20)
        StopMotor();   
}

unsigned char motor_just_started = 0;

void StartMotor()
{
    if(motor_state == 0)
    {
        dprint("Start motor\r\n");
        motor_just_started = 1;
        SpinTimer_WriteCounter(0);
        SpinTimer_Start();
        SpinTimerISR_StartEx(MotorISR);            
    }
    motor_state = 1;
}

void StopMotor()
{
    SpinTimerISR_Stop();
    SpinTimer_Stop();
    motor_state = 0;    
    dprint("Stop motor\r\n");
}

int SimulateDiskDrive(HWContext* ctx, unsigned char* pCommand, unsigned char* buffer)
{    
  	unsigned int virtual_drive_number;
    
    // emulated drive number replaced to choosen drive 
    if(pCommand[0] == (0x30 + settings.emulated_drive_no))
        virtual_drive_number = shared_parameters.actual_drive_number;
    // and vise a versa
    else if (pCommand[0] == (0x30 + shared_parameters.actual_drive_number))
        virtual_drive_number = settings.emulated_drive_no;
    else
        virtual_drive_number = pCommand[0] & 0xf;
        
    // skip if we do not serve this drive
    if(virtual_drive_number >= DEVICESNUM)
       return 0;
           
    file_t* pDisk = &virtual_disk[virtual_drive_number];
    
    // skip if drive not mounted
    if(!(pDisk->flags & FLAGS_DRIVEON))
        return 0;
    
    // reset write error flag if non get status command
    if(pCommand[1] != CmdStatus)
         pDisk->flags &= ~FLAGS_WRITEERROR;    
    
    switch(pCommand[1])
    {
    case CmdFormat:
        return Format(ctx, pDisk, buffer);
    case CmdFormatMedium:
        return FormatMedium(ctx, pDisk, buffer);
    case CmdGetHighSpeedIndex:
        return GetHighSpeedIndex(pDisk);
    case CmdReadPercom:
        return ReadPercom(pDisk, buffer);
    case CmdWritePercom:
        return WritePercom(pDisk, buffer);
    case CmdRead:
        return Read(ctx, pDisk, buffer, *((unsigned short*)(pCommand+2)));
    case CmdWrite:
    case CmdWriteVerify:
        return Write(ctx, pDisk, buffer, *((unsigned short*)(pCommand+2)));
    case CmdStatus:
        return GetStatus(pDisk);
    default:
		send_NACK();        
    }
    
    return 0;
}