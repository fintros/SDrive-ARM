/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- Based on Bob!k & Raster, C.P.U., SDrive 2008 project
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "project.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include <CyLib.h>
#include "hwcontext.h"

#include "fat.h"
#include "mmcsd.h"
#include "dprint.h"
#include "atari.h"
#include "sio.h"
#include "gpio.h"
#include "keys.h"
#include "helpers.h"
#include "sdrive.h"

#define CAPSENSE 1

typedef unsigned char uint8_t;

char _driveShift = 3; // 3 D1 = 0 drive

//
// XEX Bootloader
// Bob!k & Raster, C.P.U., 2008
//
uint8_t boot_xex_loader[179] = {
	0x72,0x02,0x5f,0x07,0xf8,0x07,0xa9,0x00,0x8d,0x04,0x03,0x8d,0x44,0x02,0xa9,0x07,
	0x8d,0x05,0x03,0xa9,0x70,0x8d,0x0a,0x03,0xa9,0x01,0x8d,0x0b,0x03,0x85,0x09,0x60,
	0x7d,0x8a,0x48,0x20,0x53,0xe4,0x88,0xd0,0xfa,0x68,0xaa,0x8c,0x8e,0x07,0xad,0x7d,
	0x07,0xee,0x8e,0x07,0x60,0xa9,0x93,0x8d,0xe2,0x02,0xa9,0x07,0x8d,0xe3,0x02,0xa2,
	0x02,0x20,0xda,0x07,0x95,0x43,0x20,0xda,0x07,0x95,0x44,0x35,0x43,0xc9,0xff,0xf0,
	0xf0,0xca,0xca,0x10,0xec,0x30,0x06,0xe6,0x45,0xd0,0x02,0xe6,0x46,0x20,0xda,0x07,
	0xa2,0x01,0x81,0x44,0xb5,0x45,0xd5,0x43,0xd0,0xed,0xca,0x10,0xf7,0x20,0xd2,0x07,
	0x4c,0x94,0x07,0xa9,0x03,0x8d,0x0f,0xd2,0x6c,0xe2,0x02,0xad,0x8e,0x07,0xcd,0x7f,
	0x07,0xd0,0xab,0xee,0x0a,0x03,0xd0,0x03,0xee,0x0b,0x03,0xad,0x7d,0x07,0x0d,0x7e,
	0x07,0xd0,0x8e,0x20,0xd2,0x07,0x6c,0xe0,0x02,0x20,0xda,0x07,0x8d,0xe0,0x02,0x20,
	0xda,0x07,0x8d,0xe1,0x02,0x2d,0xe0,0x02,0xc9,0xff,0xf0,0xed,0xa9,0x00,0x8d,0x8e,
	0x07,0xf0,0x82 };
//  relokacni tabulka neni potreba, meni se vsechny hodnoty 0x07
//  (melo by byt PRESNE 20 vyskytu! pokud je jich vic, pak bacha!!!)


#define    TWOBYTESTOWORD(ptr)           *((u16*)(ptr))
#define    FOURBYTESTOLONG(ptr)           *((u32*)(ptr))

#define US_POKEY_DIV_STANDARD	0x28		//#40  => 19040 bps
#define ATARI_SPEED_STANDARD	(US_POKEY_DIV_STANDARD+6)	//#46 (o sest vic)

#define US_POKEY_DIV_DEFAULT	0x06		//#6   => 68838 bps

#define US_POKEY_DIV_MAX		(255-6)		//pokeydiv 249 => avrspeed 255 (vic nemuze)

static virtual_disk_t vDisk[DEVICESNUM];


uint8_t system_info[]="SDrive-ARM-02 20200912 by AlSp based on Bob!k & Raster, C.P.U.";	//SDriveVersion info
//                                VV YYYYMMDD

//                        |filenameext|
uint8_t system_atr_name[]="SDRIVE  ATR";  //8+3 zamerne deklarovano za system_info,aby bylo pripadne v dosahu pres get status
//
uint8_t system_fastsio_pokeydiv_default=US_POKEY_DIV_DEFAULT;

/*
1) 720*128=92160	   (  90,000KB)
2) 1040*128=133120         ( 130,000KB)
3) 3*128+717*256=183936    ( 179,625KB)
4) 3*128+1437*256=368256   ( 359,625KB)
5) 3*128+2877*256=736896   ( 719,625KB)
6) 3*128+5757*256=1474176  (1439,625KB)
7) 3*128+11517*256=2948736 (2879,625KB)
8) 3*128+...
*/
#define IMSIZE1	92160
#define IMSIZE2	133120
#define IMSIZE3	183936
#define IMSIZE4	368256
#define IMSIZE5	736896
#define	IMSIZE6	1474176
#define IMSIZE7	2948736
#define IMSIZES 7

uint8_t system_percomtable[]= {
	0x28,0x01,0x00,0x12,0x00,0x00,0x00,0x80, IMSIZE1&0xff,(IMSIZE1>>8)&0xff,(IMSIZE1>>16)&0xff,(IMSIZE1>>24)&0xff,
	0x28,0x01,0x00,0x1A,0x00,0x04,0x00,0x80, IMSIZE2&0xff,(IMSIZE2>>8)&0xff,(IMSIZE2>>16)&0xff,(IMSIZE2>>24)&0xff,
	0x28,0x01,0x00,0x12,0x00,0x04,0x01,0x00, IMSIZE3&0xff,(IMSIZE3>>8)&0xff,(IMSIZE3>>16)&0xff,(IMSIZE3>>24)&0xff,
	0x28,0x01,0x00,0x12,0x01,0x04,0x01,0x00, IMSIZE4&0xff,(IMSIZE4>>8)&0xff,(IMSIZE4>>16)&0xff,(IMSIZE4>>24)&0xff,
	0x50,0x01,0x00,0x12,0x01,0x04,0x01,0x00, IMSIZE5&0xff,(IMSIZE5>>8)&0xff,(IMSIZE5>>16)&0xff,(IMSIZE5>>24)&0xff,
	0x50,0x01,0x00,0x24,0x01,0x04,0x01,0x00, IMSIZE6&0xff,(IMSIZE6>>8)&0xff,(IMSIZE6>>16)&0xff,(IMSIZE6>>24)&0xff,
	0x50,0x01,0x00,0x48,0x01,0x04,0x01,0x00, IMSIZE7&0xff,(IMSIZE7>>8)&0xff,(IMSIZE7>>16)&0xff,(IMSIZE7>>24)&0xff,
	0x01,0x01,0x00,0x00,0x00,0x04,0x01,0x00, 0x00,0x00,0x00,0x00
	};

virtual_disk_t* GetvDiskPtr(u08 devicenum)
{
 return &vDisk[devicenum];
}

void Clear_atari_sector_buffer(u16 len)
{
	//Maze atari_sector_buffer
    HWContext* ctx = (HWContext*)__hwcontext;
	u08 *ptr;
	ptr=&ctx->atari_sector_buffer[0];
	do { *ptr++=0; len--; } while (len);
}

//#define Clear_atari_sector_buffer_256()	Clear_atari_sector_buffer(256)

void Clear_atari_sector_buffer_256()
{
	Clear_atari_sector_buffer(256); //0 => maze 256 bytu
}

void PutDecimalNumberToAtariSectorBuffer(unsigned char num, unsigned char pos)
{
    HWContext* ctx = (HWContext*)__hwcontext;
	u08 *ptr;
	ptr = &ctx->atari_sector_buffer[pos];
	*ptr++=0x30|(num/10);
	*ptr=0x30|(num%10);
}

struct SDriveParameters
{
	u08 p0;
	u08 p1;
	u08 p2;
	u08 p3;
	u32 p4_5_6_7;
} __attribute__((packed));

//sdrparams je nadeklarovano uvnitr main() (hned na zacatku)
#define actual_drive_number sdrparams.p0
#define fastsio_active		sdrparams.p1
#define fastsio_pokeydiv	sdrparams.p2
#define bootloader_relocation sdrparams.p3
#define extraSDcommands_readwritesectornumber	sdrparams.p4_5_6_7

void setAtrVdiskFlags()
{
    HWContext* ctx = (HWContext*)__hwcontext;
    FileInfoStruct* fi = &ctx->file_info;
    unsigned char* atari_sector_buffer = &ctx->atari_sector_buffer[0];
    
    
	faccess_offset(FILE_ACCESS_READ,0,16); //ATR hlavicka vzdy

	fi->vDisk.flags=FLAGS_DRIVEON;
	if ( (atari_sector_buffer[4]|atari_sector_buffer[5])==0x01 ) {
		fi->vDisk.flags|=FLAGS_ATRDOUBLESECTORS;
	} else {
		if (fi->vDisk.size == (((unsigned long)1040)*128 + 16)) {
			fi->vDisk.flags|=FLAGS_ATRMEDIUMSIZE;
		}
	}
}


// TODO optiomize - it seems we should do fill D0 only once at the begining?

int set_boot_drive(struct SDriveParameters* sdrparams, unsigned char* p_last_drive, int skip_drive_change)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    FileInfoStruct* fi = &ctx->file_info;
    unsigned char* atari_sector_buffer = &ctx->atari_sector_buffer[0];
        
    if(skip_drive_change == 0)
	    sdrparams->p0 = 0;	// set to the boot drive - sdrive.atr

	// save state of current drive if it is not already there
    if ( *p_last_drive != 0 )
	{
		if ( *p_last_drive < DEVICESNUM )
            vDisk[*p_last_drive] = fi->vDisk;

        *p_last_drive=0; // switch it to 0:
	}

    LED_OFF;
	if ( !fatInit() ) //Inicializace FATky
	{
		//ta karta co je tam zastrcena neni zadny
		//filesystem co umime
		//takze zhasneme zelenou diodu
		LED_GREEN_OFF;	// LED OFF
		//a pocka az vytahnes kartu ven
		//while (!(inb(PIND)&0x04)); //cekani az vysune kartu ven
		//a jde cekat na jinou
        CyDelayUs(1000u);
        return -1;
	}
    
    dprint("FAT FS inititialized\r\n");

	//Vyhledani a nastaveni image SDRIVE.ATR do vD0:
	{
		unsigned short i;
		unsigned char m;

		i=0;
		while( fatGetDirEntry(i,0) )
		{
            if(!strncmp((const char*)&atari_sector_buffer[0], (const char*) &system_atr_name[0], 11))
            {
                dprint("SDRIVE.ATR found\r\n");
    			fi->vDisk.current_cluster=fi->vDisk.start_cluster;
    			fi->vDisk.ncluster=0;
    			setAtrVdiskFlags();                           
                return 0;
            }
            i++;
		}
        // not found
		fi->vDisk.flags=0; // set to non active
		
        // set active drive to default one
        // TODO: drive shift
		if (sdrparams->p0 == 0)
           sdrparams->p0 = 1;
	}
    return 0;    
}


int wait_for_command(struct SDriveParameters* sdrparams, unsigned char* p_last_drive)
{
    static unsigned long autowritecounter = 0;
    static unsigned char last_key = 0;

    while( get_cmd_H() )	//dokud je Atari signal command na H
    {           
        unsigned char actual_key = get_actual_key();
           
        if(actual_key != last_key)
        {
            dprint("Key: %02X\r\n", actual_key);

            switch(actual_key)
        	{
        		case 6: ///BOOT KEY
        			if(set_boot_drive(sdrparams, p_last_drive, 0))
                        return -1;
        			break;;
        		case 5: //LEFT
        			if(sdrparams->p0>1)
        				sdrparams->p0--;
        			else
        				sdrparams->p0 = DEVICESNUM-1;
        			break;
        		case 3: //RIGHT
        			if(sdrparams->p0 < ( DEVICESNUM-1))
        				sdrparams->p0++;
        			else
        				sdrparams->p0=1;
        			break;
        	}
        	set_display(sdrparams->p0);
        	last_key = actual_key;
        	CyDelay(50); //sleep for 50ms
        }

        if (autowritecounter>1700000)
        {
         	mmcWriteCachedFlush();
        	autowritecounter=0;
        }
        else
         autowritecounter++;
    }
    return 0;
}


void change_fast_sio(struct SDriveParameters* sdrparams)
{
	 u08 as;
	 as=ATARI_SPEED_STANDARD;	//default speed
     // if fastsio
	 if (sdrparams->p2) as=sdrparams->p3+6;	

	 USART_Init(as);    
}


int get_command(struct SDriveParameters* sdrparams, unsigned char* command)
{
    LED_OFF;
    memset(command, 0, 4);
	u08 err = USART_Get_Buffer_And_Check(command,4,CMD_STATE_L);

#ifdef DEBUG
    if(err)
        dprint("Command error: [%02X]\r\n", err);
    else
        dprint("Command: [%02X %02X %02X %02X]\r\n", command[0], command[1], command[2], command[3]);
#endif

	CyDelayUs(800u);	//t1 (650-950us) (Bez tehle pauzy to cele nefunguje!!!)
	wait_cmd_LH();	//ceka az se zvedne signal command na H
	CyDelayUs(100u);	//T2=100   (po zvednuti command a pred ACKem)

    LED_GREEN_ON;
    LED_ON;
    
	if(err)
	{
        // if key was pressed
		if (err&0x02) 
            return -1;

        // if not standard rate - cheche fastsio flag
		if (sdrparams->p3!=US_POKEY_DIV_STANDARD)
			sdrparams->p2 = !sdrparams->p2;

        change_fast_sio(sdrparams);
        return -1;
	}

    return 0;
}


int process_drive_emulation(struct SDriveParameters* sdrparams, unsigned char* command)
{

    
    return 0;
}


//----- Begin Code ------------------------------------------------------------
int sdrive(void)
{
    dprint("SDrive started\r\n");

    HWContext* ctx = (HWContext*)__hwcontext;
    unsigned char* atari_sector_buffer = &ctx->atari_sector_buffer[0];
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
    FileInfoStruct* fi = &ctx->file_info;

	unsigned char command[5];
	unsigned char last_used_virtual_drive;

    LED_OFF;
	//Parameters
	struct SDriveParameters sdrparams;

    init_keys();
    
    Debug_B_Start();

    LED_ON;
    
    dprint("SDrive initialized\r\n");

SD_CARD_EJECTED:

//	last_key=0;
	last_used_virtual_drive=0;	//inicializace
	fastsio_active=0;
	bootloader_relocation=0;
	//actual_drive_number=0;	//nastavovano kousek dal (za SET_BOOT_DRIVE)
	fi->percomstate=0;	//inicializace
	fastsio_pokeydiv= system_fastsio_pokeydiv_default; //definovano v EEPROM
    
	USART_Init(ATARI_SPEED_STANDARD);
    
    char ledState = 0;
    //CEKACI SMYCKA!!!
	while (SWBUT_Read() & 0x40)
    { 
        CyDelay(200);
        ledState = 1-ledState;
        if(ledState) LED_RED_ON; 
        else LED_RED_OFF; 
    }

	//pozor, v tomto okamziku nesmi byt nacacheovan zadny sektor
	//pro zapis, protoze init karty inicializuje i mmc_sector atd.!!!
    LED_RED_OFF; 
    LED_GREEN_ON;
	do
	{
        dprint("Retry\r\n");
		mmcInit();
	}
	while(mmcReset());	//Wait for SD card ready

    LED_GREEN_OFF;
    dprint("SD card inititialized\r\n");

	{
	 u08 i;
	 for(i=0; i<DEVICESNUM; i++) GetvDiskPtr(i)->flags=0;	//vDisk[i].flags=0;
	}
	fi->vDisk.flags=0;		//special_xex_loader=0;

    if(set_boot_drive(&sdrparams, &last_used_virtual_drive, 0))
        goto SD_CARD_EJECTED;
    
	set_display(actual_drive_number);

/////////////////////

	for(;;)
	{
ST_IDLE:

		LED_GREEN_OFF;	// LED OFF
        if(wait_for_command(&sdrparams, &last_used_virtual_drive))
            goto SD_CARD_EJECTED;            
        
        LED_GREEN_OFF;
        
        if(get_command(&sdrparams, command))
            continue;

		if( command[0]>=0x31 && command[0]<(0x30+DEVICESNUM) ) //D1: az D4: (ano, az od D1: !!!)
		{
			// dostupne "z venci" je jen D1: az D4: (pres 0x31 az 0x34)
			// ale pres zarizeni SDrive (0x71 az 0x74 podle zvoleneho cisla sdrive number)
			// se da pristupovat vzdy primo k vD0:

		   	// DISKETOVE OPERACE D0: az D4:
		  	unsigned char virtual_drive_number;

			if( command[0] == (unsigned char)0x31 + 3 - ((unsigned char)(_driveShift&0x03)))
			{
				//chce cist z drive Dx (D1: az D4:) ktere je prehazovaci
			    virtual_drive_number = actual_drive_number;
			}
			else
			if ( command[0] == (unsigned char)0x30+actual_drive_number )
			{
				//chce cist z drive Dx (D0: az D4:) ktery je prehozen s drive number
				virtual_drive_number = 4-((unsigned char)(_driveShift&0x03));
			}
			else
			{
				//chce cist z nektereho jineho Dx:
disk_operations_direct_d0_d4:
				virtual_drive_number = command[0]&0xf;
			}
            
            //dprint("Disk op luvd:%d, vd:%d\r\n",last_used_virtual_drive, virtual_drive_number);

			if ( last_used_virtual_drive != virtual_drive_number )
			{
				if ( last_used_virtual_drive < DEVICESNUM )
                    vDisk[last_used_virtual_drive] = fi->vDisk;
                
                fi->vDisk = vDisk[virtual_drive_number];

                last_used_virtual_drive=virtual_drive_number; //zmena
                //dprint("[0]Set luvd to %d\r\n", last_used_virtual_drive);
			}

			if (!(fi->vDisk.flags & FLAGS_DRIVEON))
			{
				//do daneho Dx neni vlozen disk
				goto ST_IDLE;
			}

			//LED_GREEN_ON;	// LED on

			//pro cokoli jineho nez 0x53 (get status) nuluje FLAGS_WRITEERROR
			if (command[1]!=0x53) fi->vDisk.flags &= (~FLAGS_WRITEERROR); //vynuluje FLAGS_WRITEERROR bit

			//formatovaci povely zpracovava v predsunu tady kvuli ACK/NACK
			switch(command[1])
			{
			case 0x21:	// format single + PERCOM
				// 	Single for 810 and stock 1050 or as defined by CMD $4F for upgraded drives (Speedy, Happy, HDI, Black Box, XF 551).
				//Returns 128 bytes (SD) or 256 bytes (DD) after format. Format ok requires that the first two bytes are $FF. It is said to return bad sector list (never seen one).

			   { //zacatek sekce pres dva case, 0x21,0x22
			    u08 formaterror;

				{
				 u32 singlesize;
				 singlesize = (u32)92160;
				 if ( !(fi->vDisk.flags & FLAGS_XFDTYPE) ) singlesize+=(u32)16; //je to ATRko

				 if (   ( fi->percomstate==3 ) //snazil se zapsat neodpovidajici percom
					|| ( (fi->percomstate==0) && (fi->vDisk.size!=singlesize) ) //neni single disketa
				   )
				 {
					goto Send_NACK_and_set_FLAGS_WRITEERROR_and_ST_IDLE;
				 }
				}

				{
					unsigned short i,size;
					if ( fi->percomstate == 2)
					{
						size=256;
					}
					else
					{
format_medium:
						size=128;
					}

					fi->percomstate=0; //po prvnim formatu pozbyva percom ucinnost

					//XEX formatovat nelze
					if (fi->vDisk.flags & (FLAGS_XEXLOADER | FLAGS_READONLY))
					{
						goto Send_NACK_and_set_FLAGS_WRITEERROR_and_ST_IDLE;
					}

					//OK, bude se formatovat
					send_ACK();

					formaterror=0;

					{
					 //vynuluje formatovany soubor
					 Clear_atari_sector_buffer_256();
					 {
					  unsigned long maz,psize;
					  maz=0;
					  if ( !(fi->vDisk.flags & FLAGS_XFDTYPE) ) maz+=16;
					  while( (psize=(fi->vDisk.size-maz))>0 )
					  {
					   if (psize>256) psize=256;
					   if (!faccess_offset(FILE_ACCESS_WRITE,maz,(u16)psize))
					   {
					   	formaterror=1; //nepodaril se zapis
						break;
					   }
					   maz+=psize;
					  }
					 }
					}
					
					//predchysta hodnoty 0xff nebo 0x00 v navratovem sektoru (co vraci povel format)
					//a v pripade chyby nastavi FLAGS_WRITEERROR
					{
					 u08 *ptr;
					 ptr=atari_sector_buffer;
					 i=size;
					 do { *ptr++=0xff; i--; } while(i>0);
					 if (formaterror)
					 {
					  atari_sector_buffer[0]=0x00; //err
					  atari_sector_buffer[1]=0x00; //err
					  fi->vDisk.flags |= FLAGS_WRITEERROR;	//nastavi FLAGS_WRITEERROR
					 }
					}

					//vrati je Atarku

					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(size);
					goto ST_IDLE; //aby nepokracoval
				}
				break;

			case 0x22: // format medium
				// 	Formats medium density on an Atari 1050. Format medium density cannot be achieved via PERCOM block settings!

				if (! (fi->vDisk.flags & (FLAGS_ATRMEDIUMSIZE | FLAGS_READONLY) ) != FLAGS_ATRMEDIUMSIZE )
				{
					//fi->percomstate=0;
					//goto Send_ERR_and_Delay;
Send_NACK_and_set_FLAGS_WRITEERROR_and_ST_IDLE:
					fi->vDisk.flags |= FLAGS_WRITEERROR;	//nastavi FLAGS_WRITEERROR
					goto Send_NACK_and_ST_IDLE;
				}

				goto format_medium;
			   } //konec sekce pres dva case, 0x21,0x22
			   break;

		   case 0x50: //(write)
		   case 0x57: //(write verify)
		   		//pro tyhle commandy nerozina LED_GREEN_ON !
		   		goto device_command_accepted;

		   default:
				//pro vsechny ostatni rozina zelenou LED
   				LED_GREEN_ON;	// LED on

			}
			//je to nejaky jiny nez podporovany command?
			if(
			     (command[1]!=0x52) 
				 //&& (command[1]!=0x50) //uz porovnano predtimto (switch), skace na device_command_accepted;
				 && (command[1]!=0x53) 
				 //&& (command[1]!=0x57) //uz porovnano predtimto (switch), skace na device_command_accepted;
				 //&& (command[1]!=0x3f) //zpracovava se uvnitr teto podminky
				 && (command[1]!=0x4e) && (command[1]!=0x4f)
				 //&& (command[1]!=0x21) && (command[1]!=0x22) //uz zpracovany predtimto
			  )
			{
				if (command[1]==0x3f && fastsio_pokeydiv!=US_POKEY_DIV_STANDARD) goto device_command_accepted;
Send_NACK_and_ST_IDLE:
				send_NACK();
				goto ST_IDLE;
			}
device_command_accepted:

			send_ACK();
//			Delay1000us();	//delay_us(COMMAND_DELAY);

			switch(command[1])
			{

			case 0x3f:	// read speed index
				{
                    dprint("Ask for speed\r\n");
					atari_sector_buffer[0]=fastsio_pokeydiv;	//primo vraci Pokey divisor
					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(1);
				}
				break;

			case 0x4e:	// read cfg (PERCOM)

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

/*
				atari_sector_buffer[0] = 40; //0x01; // HDD, one track
				atari_sector_buffer[1] =  0; //0x01; // version 0.1
				{
					unsigned long s;
					// 16 bytu za ATR neni potreba odecitat, nebot pri deleni 40 to vyjde stejne!
					s= fi->vDisk.size / 40;
					if (fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS)
					 s=(s>>8)+1; //s=s/256; +1, protoze prvni 3 sektory jsou 128 a tudiz by se melo k size pripocitat 384 a to pak delit 40 a 256, ale vyjde nastejno pricist 1 k vyslednym sektorum
					else
					 s=(s>>7); //s=s/128;
					atari_sector_buffer[2] = (s>>8) & 0xff;   //hb sectors
					atari_sector_buffer[3] = ( s & 0xff ); //lb sectors
					atari_sector_buffer[4] = (s>>16) & 0xff; //pocet stran - 1  (0=> jedna strana)
				}
				//[5] =0 pro single, =4 pro mediumtype nebo doublesize sectors
				atari_sector_buffer[5] = (fi->vDisk.flags & (FLAGS_ATRMEDIUMSIZE|FLAGS_ATRDOUBLESECTORS))? 0x04 : 0x00;
				if (fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS)
				{
					atari_sector_buffer[6]=0x01;	//sector size HB
					atari_sector_buffer[7]=0x00;	//sector size LB
				}
				else
				{
					atari_sector_buffer[6]=0x00;	//sector size HB
					atari_sector_buffer[7]=0x80;	//sector size LB
				}
				atari_sector_buffer[8]  = 0x01;	//0x01; //0x40; // drive exist
				atari_sector_buffer[9]  = 'S';	//0x41;	//0xc0; //'I'; 
				atari_sector_buffer[10] = 'D';	//0x00;	//0xf2; //'D'; 
				atari_sector_buffer[11] = 'r';	//0x00;	//0x42; //'E'; 
*/
				//XXXXXXXXXXXXXXXXXXXXXX
			{
				u08 *ptr;
				u08 isxex;
				u16 secsize;
				u32 fs;

				isxex = ( fi->vDisk.flags & FLAGS_XEXLOADER );

				fs=fi->vDisk.size;

				secsize=(fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS)? 0x100:0x80;
				
				ptr = system_percomtable;

				//if ( isxex ) ptr+=(IMSIZES*12);

				do
				{
				 u08 m;
				 //12 hodnot na radku v system_percomtable
				 //0x28,0x01,0x00,0x12,0x00,0x00,0x00,0x80, IMSIZE1&0xff,(IMSIZE1>>8)&0xff,(IMSIZE1>>16)&0xff,(IMSIZE1>>24)&0xff,
				 //...
				 //0x01,0x01,0x00,0x00,0x00,0x04,0x01,0x00, 0x00,0x00,0x00,0x00
				 for(m=0;m<12;m++) atari_sector_buffer[m]=*ptr++;
				 if (    (!isxex) 
				 	  && ( FOURBYTESTOLONG(atari_sector_buffer+8)==(fs & 0xffffff80) )  // &0xff... kvuli vyruseni pripadnych 16bytu ATR hlavicky
					  && ( atari_sector_buffer[6]==(secsize>>8) ) //sectorsize hb
					)
				 {
				  //velikost souboru i sektoru souhlasi
				   goto percom_prepared;
				 }
				} while (atari_sector_buffer[0]!=0x01);

				//zadna znama disketa, nastavit velikost a pocet sektoru
				atari_sector_buffer[6]=(secsize>>8);	//hb
				atari_sector_buffer[7]=(secsize&0xff);	//db

 				if ( isxex )
				{
				 secsize=125; //-=3; //=125
				 fs+=(u32)46125;		//((u32)0x171*(u32)125);
				}
				if ( fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS )
				{
				 fs+=(u32)(3*128); //+384 bytu za prvni tri sektory s velikosti 128
				}

				//FOURBYTESTOLONG(atari_sector_buffer+8)=fs; //DEBUG !!!!!!!!!!!

				fs/=((u32)secsize); //prepocet filesize na pocet sektoru

				atari_sector_buffer[2] = ((fs>>8) & 0xff);	//hb sectors
				atari_sector_buffer[3] = (fs & 0xff );		//lb sectors
				atari_sector_buffer[4] = ((fs>>16) & 0xff);	//pocet stran - 1  (0=> jedna strana)

percom_prepared:
//*
				atari_sector_buffer[8]  = 0x01;	//0x01; //0x40; // drive exist
				atari_sector_buffer[9]  = 'S';	//0x41;	//0xc0; //'I'; 
				atari_sector_buffer[10] = 'D';	//0x00;	//0xf2; //'D'; 
				atari_sector_buffer[11] = 'r';	//0x00;	//0x42; //'E'; 
// */
			}

				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(12);

				break;


			case 0x4f:	// write cfg (PERCOM)

				if (USART_Get_atari_sector_buffer_and_check_and_send_ACK_or_NACK(12))
				{
					break;
				}

				//propocitam, zda velikost zapsana do percomu odpovida
				//velikosti vlozene diskety
				{
					u32 s;
					s = ((u32)atari_sector_buffer[0])
						 * ( (u32) ( (((u16)atari_sector_buffer[2])<<8) + ((u16)atari_sector_buffer[3]) ) )
						 * (((u32)atari_sector_buffer[4])+1)
						 * ( (u32) ( (((u16)atari_sector_buffer[6])<<8) + ((u16)atari_sector_buffer[7]) ) );
					if ( !(fi->vDisk.flags & FLAGS_XFDTYPE) ) s+=16; //ATRko 16 bytu
					if ( fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS ) s-=384;	//prvni 3 sektory v double jsou jen 128bytu => odpocitat 3*128
					if (s!=fi->vDisk.size)
					{
						fi->percomstate=3;	//percom write bad
						//goto Send_ERR_and_Delay;
						goto Send_CMPL_and_Delay; //percom zapis je ok, ale hodnoty neodpovidaji
					}
				}

				fi->percomstate=1;		//ok
				if ( (atari_sector_buffer[6]|atari_sector_buffer[7])==0x01) fi->percomstate++; //=2 ok (double)

				goto Send_CMPL_and_Delay;
				break;


			case 0x50:	//write
			case 0x57:	//write (verify)
			case 0x52:	//read
				{
				unsigned short proceeded_bytes;
				unsigned short atari_sector_size;
				u32 n_data_offset;
				u16 n_sector;
				u16 file_sectors;

				fi->percomstate=0;

				n_sector = TWOBYTESTOWORD(command+2);	//2,3

                if(n_sector == 0x169)
                   n_sector=0x169;
                
				if(n_sector==0)
					goto Send_ERR_and_Delay;

				if( !(fi->vDisk.flags & FLAGS_XEXLOADER) )
				{
					//ATRko nebo XFDcko
					if(n_sector<4)
					{
						//sektory 1 az 3
						atari_sector_size = (unsigned short)0x80;	//128
						//optimalizace: n_sector = 1 az 3 a sector size je napevno 128!
						//stary zpusob: n_data_offset = (u32) ( ((u32)(((u32)n_sector)-1) ) * ((u32)atari_sector_size));
						n_data_offset = (u32) ( (n_sector-1) << 7 ); //*128;
					}
					else
					{
						//sektor 4 nebo dalsi
						atari_sector_size = (fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS)? ((unsigned short)0x100):((unsigned short)0x80);	//fi->vDisk.atr_sector_size;
						n_data_offset = (u32) ( ((u32)(((u32)n_sector)-4) ) * ((u32)atari_sector_size)) + ((u32)384);
					}

					//ATR nebo XFD?
					if (! (fi->vDisk.flags & FLAGS_XFDTYPE) ) n_data_offset+= (u32)16; //ATR header;

					if(command[1]==0x52)
					{
						//read
						proceeded_bytes = faccess_offset(FILE_ACCESS_READ,n_data_offset,atari_sector_size);
                        dprint("Read result %d, offset=%d, secsize=%d\r\n", proceeded_bytes,n_data_offset, atari_sector_size);
						if(proceeded_bytes==0)
						{
							goto Send_ERR_and_Delay;
						}
					}
					else
					{
						//write do image
						if (USART_Get_atari_sector_buffer_and_check_and_send_ACK_or_NACK(atari_sector_size))
						{
							break;
						}

						if ( get_readonly() || (fi->vDisk.flags & FLAGS_READONLY) )
							 goto Send_ERR_and_Delay; //READ ONLY

						proceeded_bytes = faccess_offset(FILE_ACCESS_WRITE,n_data_offset,atari_sector_size);
						if(proceeded_bytes==0)
						{
							goto Send_ERR_and_Delay;
						}

						goto Send_CMPL_and_Delay;

					}
					//atari_sector_size= (bud 128 nebo 256 bytu)
				}
				else
				{
					//XEX
#define	XEX_SECTOR_SIZE	128

					if (command[1]!=0x52)
					{
						//snaha o zapis do XEXu
						//prebere data
						if (USART_Get_atari_sector_buffer_and_check_and_send_ACK_or_NACK(XEX_SECTOR_SIZE))
						{
							//neslouhlasil chcecksum (poslal uz NACK)
							break;
						}
						//a vrati error
						//protoze do XEXu zapisovat nejde (beee, beee... ;-)))
						goto Send_ERR_and_Delay;
					}

					//promaze buffer
					Clear_atari_sector_buffer(XEX_SECTOR_SIZE);

					if(n_sector<=2)		//n_sector>0 && //==0 se overuje hned na zacatku
					{
						//sektory xex bootloaderu, tj. 1 nebo 2
						u08 i,b;
						u08 *spt, *dpt;
						spt= &boot_xex_loader[(u16)(n_sector-1)*((u16)XEX_SECTOR_SIZE)];
						dpt= atari_sector_buffer;
						i=XEX_SECTOR_SIZE;
						do 
						{
						 b=*spt++;
 						 //relokace bootloaderu z $0700 na jine misto
						 if (b==0x07) b+=bootloader_relocation;
						 *dpt++=b;
						 i--;
						} while(i);
					}

					//file_sectors se pouzije pro sektory $168 i $169 (optimalizace)
					//zarovnano nahoru, tj. =(size+124)/125
					file_sectors = ((fi->vDisk.size+(u32)(XEX_SECTOR_SIZE-3-1))/((u32)XEX_SECTOR_SIZE-3));

					if(n_sector==0x168)
					{
						//vrati pocet sektoru diskety
						//byty 1,2
						goto set_number_of_sectors_to_buffer_1_2;
					}
					else
					if(n_sector==0x169)
					{
						//fatGetDirEntry(fi->vDisk.file_index,5,0);
						fatGetDirEntry(fi->vDisk.file_index,0); //ale musi to posunout o 5 bajtu doprava
			
						{
							u08 i,j;
							for(i=j=0;i<8+3;i++)
							{
								if( ((atari_sector_buffer[i]>='A' && atari_sector_buffer[i]<='Z') ||
									(atari_sector_buffer[i]>='0' && atari_sector_buffer[i]<='9')) )
								{
								  //znak je pouzitelny na Atari
								  atari_sector_buffer[j]=atari_sector_buffer[i];
								  j++;
								}
								if ( (i==7) || (i==8+2) )
								{
									for(;j<=i;j++) atari_sector_buffer[j]=' ';
								}
							}
							//posune nazev z 0-10 na 5-15 (0-4 budou systemova adresarova data)
							//musi pozpatku
							for(i=15;i>=5;i--) atari_sector_buffer[i]=atari_sector_buffer[i-5];
							//a pak uklidi cely zbytek tohoto sektoru
							for(i=5+8+3;i<XEX_SECTOR_SIZE;i++)
								atari_sector_buffer[i]=0x00;
						}
						//teprve ted muze pridat prvnich 5 bytu na zacatek nulte adresarove polozky (pred nazev)
						//atari_sector_buffer[0]=0x42;							//0
						//jestlize soubor zasahuje do sektoru cislo 1024 a vic,
						//status souboru je $46 misto standardniho $42
						atari_sector_buffer[0]=(file_sectors>(0x400-0x171))? 0x46 : 0x42; //0

						TWOBYTESTOWORD(atari_sector_buffer+3)=0x0171;			//3,4
set_number_of_sectors_to_buffer_1_2:
						TWOBYTESTOWORD(atari_sector_buffer+1)=file_sectors;		//1,2
					}
					else
					if(n_sector>=0x171)
					{
						proceeded_bytes = faccess_offset(FILE_ACCESS_READ,((u32)n_sector-0x171)*((u32)XEX_SECTOR_SIZE-3),XEX_SECTOR_SIZE-3);

						if(proceeded_bytes<(XEX_SECTOR_SIZE-3))
							n_sector=0; //je to posledni sektor
						else
							n_sector++; //ukazatel na dalsi

						atari_sector_buffer[XEX_SECTOR_SIZE-3]=((n_sector)>>8); //nejdriv HB !!!
						atari_sector_buffer[XEX_SECTOR_SIZE-2]=((n_sector)&0xff); //pak DB!!! (je to HB,DB)
						atari_sector_buffer[XEX_SECTOR_SIZE-1]=proceeded_bytes;
					}

					//sector size vzdy 128 bytu
					atari_sector_size=XEX_SECTOR_SIZE;
				}
#ifdef DEBUG
               if(1){
                    dprint("Put Atari sector: \r\n");
                    int i = 0;
                    for(i = 0; i < atari_sector_size;i+=16)
                    {
                        dprint("%02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
                            atari_sector_buffer[i],
                            atari_sector_buffer[i+1],
                            atari_sector_buffer[i+2],
                            atari_sector_buffer[i+3],
                            atari_sector_buffer[i+4],
                            atari_sector_buffer[i+5],
                            atari_sector_buffer[i+6],
                            atari_sector_buffer[i+7],
                            atari_sector_buffer[i+8],
                            atari_sector_buffer[i+9],
                            atari_sector_buffer[i+10],
                            atari_sector_buffer[i+11],
                            atari_sector_buffer[i+12],
                            atari_sector_buffer[i+13],
                            atari_sector_buffer[i+14],
                            atari_sector_buffer[i+15]                        
                        );
                    }
                } 
#endif
                
				//posle bud 128 nebo 256 (atr/xfd) nebo 128 (xex)
				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(atari_sector_size);

				}
				break;

			case 0x53:	//get status

				//fi->percomstate=0;

				atari_sector_buffer[0] = 0x10;	//0x00 motor off	0x10 motor on
				if (fi->vDisk.flags & FLAGS_ATRMEDIUMSIZE) atari_sector_buffer[0]|=0x80;		//(fi->vDisk.atr_medium_size);	// medium/single
				if (fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS) atari_sector_buffer[0]|=0x20;	//((fi->vDisk.atr_sector_size==256)?0x20:0x00); //	double/normal sector size
				if (fi->vDisk.flags & FLAGS_WRITEERROR)
				{
				 atari_sector_buffer[0]|=0x04;			//Write error status bit
				 //vynuluje FLAGS_WRITEERROR bit
				 fi->vDisk.flags &= (~FLAGS_WRITEERROR);
				}
				if (get_readonly() || (fi->vDisk.flags & FLAGS_READONLY)) atari_sector_buffer[0]|=0x08;	//write protected bit

				atari_sector_buffer[1] = 0xff;
				atari_sector_buffer[2] = 0xe0; 		//(244s) timeout pro nejdelsi operaci
				atari_sector_buffer[3] = 0x00;		//timeout hb


				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(4);

                dprint("Send status: [%02X %02X %02X %02X]\r\n", 
                    atari_sector_buffer[0], 
                    atari_sector_buffer[1],
                    atari_sector_buffer[2],
                    atari_sector_buffer[3]);
                
				break;
			} //switch
		} // disketove operace
		else
		if( command[0] == (unsigned char)0x71 +3-((unsigned char)(_driveShift&0x03)))
		{
			// SDRIVE OPERACE 
            
            char FileFindBuffer[11];

			//if ( command[1]==0x50 || command[1]==0x52 || command[1]==0x53)
			if ( command[1]<=0x53 && command[1]>=0x50 )	//schvalne prehozeno poradi kvuli rychlosti (pri normalnich operacich s SDrive nebude platit hned prvni cast podminky)
			{
				//(0x50,x52,0x53)
				//pro povely readsector,writesector a status
				//pristup na D0: pres SDrive zarizeni 0x71-0x74
				command[0]=0x30;	//zmeni pozadovane zarizeni na D0:
				goto disk_operations_direct_d0_d4; //vzdy napevno vD0: bez prehazovani
			}


			send_ACK();
//			Delay1000us();	//delay_us(COMMAND_DELAY);

			if ( last_used_virtual_drive != 0x0f )
			{
				//predtim pracoval s nejakym Dx:
				//musi uschovat jeho stav do vDisk[]
				if ( last_used_virtual_drive < DEVICESNUM )
                    vDisk[last_used_virtual_drive] = fi->vDisk;

                //zmena last_used_virtual_drive
				last_used_virtual_drive=0x0f; //zmena
                //dprint("[1]Set luvd to %d\r\n", last_used_virtual_drive);
			}


			switch(command[1])
			{
/*
			case 0xXX:
				{
				 unsigned char* p = 0x060;
				 //check_sum = get_checksum(p,1024);	//nema smysl - checksum cele pameti by se ukladal zase do pameti a tim by to zneplatnil
				 Delay800us();	//t5
				 send_CMPL();
				 Delay800us();	//t6

				 USART_Send_Buffer(p,1024);
				 //USART_Transmit_Byte(check_sum);
				}
				break;

			case 0xXX:	//STACK POINTER+DEBUG_ENDOFVARIABLES
				{
				 Clear_atari_sector_buffer_256();
				 atari_sector_buffer[0]= SPL;	//inb(0x3d);
				 atari_sector_buffer[1]= SPH;	//inb(0x3e);
				 *((u32*)&(atari_sector_buffer[2]))=debug_endofvariables;

				 //check_sum = get_checksum(atari_sector_buffer,256);
				 //send_CMPL();
				 //Delay1000us();	//delay_us(COMMAND_DELAY);
				 //USART_Send_Buffer(atari_sector_buffer,256);
				 //USART_Transmit_Byte(check_sum);
 				 USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(6);
				}
				break;

			case 0xXX:	//$XX nn mm	 config EEPROM par mm = value nn
				{
					eeprom_write_byte((&system_atr_name)+command[3],command[2]);
					goto Send_CMPL_and_Delay;
				}
				break;
*/

			//--------------------------------------------------------------------------

			case 0xC0:	//$C0 xl xh	Get 20 filenames from xhxl. 8.3 + attribute (11+1)*20+1 [<241]
						//		+1st byte of filename 21 (+1)
				{
					u08 i,j;
					u08 *spt,*dpt;
					u16 fidx;
					Clear_atari_sector_buffer_256();
					fidx=TWOBYTESTOWORD(command+2); //command[2],[3]
					dpt=atari_sector_buffer+12;
					i=21;
					while(1)
					{	
						i--;
						if (!fatGetDirEntry(fidx++,0)) break;
						//8+3+ukonceni0
						spt=atari_sector_buffer;
						if (!i)
						{
							*dpt++=*spt; //prvni znak jmena 21 souboru
							break;
						}
					 	j=11; //8+3znaku=11
						do { *dpt++=*spt++; j--; } while(j);
						*dpt++=fi->Attr; //na pozici[11] da atribut
					}// while(i);
					//a ted posunout 241 bajtu o 12 dolu [12-252] => [0-240]
					spt=atari_sector_buffer+12;
					dpt=atari_sector_buffer;
					i=241;
					do { *dpt++=*spt++; i--; } while(i);
					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(241); //241! bajtu
				}
				break;


			case 0xC1:	//$C1 nn ??	set fastsio pokey divisor
				
				if (command[2]>US_POKEY_DIV_MAX) goto Send_ERR_and_Delay;
				// temp
                fastsio_pokeydiv = command[2];
				fastsio_active=0;	//zmenila se rychlost, musi prejit na standardni
				
				/*
				//takhle to zlobilo
				//vzdy to zabrucelo a vratio error #$8a (i kdyz se to provedlo)
				USART_Init(ATARI_SPEED_STANDARD); //a zinicializovat standardni
				goto Send_CMPL_and_Delay;
				*/

				CyDelayUs(800u);	//t5
				send_CMPL();
                
                change_fast_sio(&sdrparams);
				break;

			case 0xC2:	//$C2 nn ??	set bootloader relocation = $0700+$nn00

				bootloader_relocation = command[2];
				goto Send_CMPL_and_Delay;

			//--------------------------------------------------------------------------

			case 0xDA:	//get system values
				{
					u08 *sptr,*dptr;
					u08 i;
					dptr=atari_sector_buffer;
					//
					//Globalni sada promennych
					sptr=(u08*)&ctx->gsv;
					i=sizeof(GlobalSystemValues);	//13
					do { *dptr++=*sptr++; i--; } while(i>0);
					//tzv. lokalni vDisk
					sptr=(u08*)&fi->vDisk;
					i=sizeof(virtual_disk_t);				//21
					do { *dptr++=*sptr++; i--; } while(i>0);
                    
                    dprint("!!!Unable to be displayed!\r\n");

					//celkem 26 bytu
					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum( (sizeof(GlobalSystemValues)+sizeof(virtual_disk_t)) );
				}				
				break;

			case 0xDB:	//get vDisk flag [<1]
				{
					//atari_sector_buffer[0] = (command[2]<DEVICESNUM)? GetvDiskPtr(command[2])->flags : fi->vDisk.flags;
					atari_sector_buffer[0] = fi->vDisk.flags;
					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(1);
				}
				break;

			case 0xDC:	//$DC xl xh	fatClusterToSector xl xh [<4]
				{
					FOURBYTESTOLONG(&atari_sector_buffer) = fatClustToSect((u32) TWOBYTESTOWORD(command+2));  // todo check!!! should be 4 bytes long
					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(4);
				}
				break;

			case 0xDD:	//set SDsector number

				if (USART_Get_atari_sector_buffer_and_check_and_send_ACK_or_NACK(4))
				{
					break;
				}

				extraSDcommands_readwritesectornumber = *((u32*)&atari_sector_buffer);

				//uz si to hned ted nacte do cache kvuli operaci write SDsector
				//ve ktere Atarko zacne posilat data brzo po ACKu a nemuselo by se to stihat
				mmcReadCached(extraSDcommands_readwritesectornumber);

				goto Send_CMPL_and_Delay;
				break;

			case 0xDE:	//read SDsector

				mmcReadCached(extraSDcommands_readwritesectornumber);

				CyDelayUs(800u);	//t5
				send_CMPL();
				CyDelayUs(800u);	//t6
				{
				 u08 check_sum;
				 check_sum = get_checksum(mmc_sector_buffer,512);
				 USART_Send_Buffer(mmc_sector_buffer,512);
				 USART_Transmit_Byte(check_sum);
				}
				//USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(atari_sector_size); //nelze pouzit protoze checksum je 513. byte (prelezl by ven z bufferu)
				break;

			case 0xDF:	//write SDsector
				
				//Specificky problem:
				//Atarko zacina predavat data za t3(min)=1000us,
				//ale tak rychle nestihneme nacist (Flushnout) mmcReadCached
				//RESENI => mmcReadCached(extraSDcommands_readwritesectornumber);
				//          se vola uz pri nastavovani cisla SD sektoru, takze uz je to
				//          nachystane. (tedy pokud si nezneplatnil cache jinymi operacemi!)
			
				//nacte ho do mmc_sector_bufferu
				//tim se i Flushne pripadny odlozeny write
				//a nastavi se n_actual_mmc_sector
				mmcReadCached(extraSDcommands_readwritesectornumber);

				//prepise mmc_buffer daty z Atarka
				{
				 u08 err;
				 err=USART_Get_Buffer_And_Check(mmc_sector_buffer,512,CMD_STATE_H);
				 CyDelayUs(1000u);
				 if(err)
				 {
					//obnovi puvodni stav mc_sector_bufferu z SDkarty!!!
					mmcReadCached(extraSDcommands_readwritesectornumber);
					send_NACK();
					break;
				 }
				}

				send_ACK();

				//zapise zmenena data v mmc_sector_bufferu do prislusneho SD sektoru
				if (mmcWriteCached(0)) //klidne s odlozenym zapisem
				{
					goto Send_ERR_and_Delay; //nepovedlo se (zakazany zapis)
				}

				goto Send_CMPL_and_Delay;
				break;

			//--------------------------------------------------------------------------

			case 0xE0: //get status
				//$E0  n ??	Get status. n=bytes of infostring "SDriveVV YYYYMMDD..."

				if (!command[2]) goto Send_CMPL_and_Delay;

				{
				 u08 i;
				 for(i=0; i<command[2]; i++) atari_sector_buffer[i]=system_info[i];
				}

				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(command[2]);

				break;


			case 0xE1: //init drive
				//$E1  n ??	Init drive. n=0 (komplet , jako sdcardejected), n<>0 (jen setbootdrive d0: bez zmeny actual drive)

				//protoze muze volat prvni cast s mmcReset,
				//musi ulozit pripadny nacacheovany sektor
			 	mmcWriteCachedFlush(); //pokud ceka nejaky sektor na zapis, zapise ho

				CyDelayUs(800u);	//t5
				send_CMPL();
				
				if ( command[2]==0 ) goto SD_CARD_EJECTED;
				//goto SET_BOOT_DRIVE; //<--tohle zmeni i actual_drive na 0
				if(set_boot_drive(&sdrparams, &last_used_virtual_drive, 1))
                    goto SD_CARD_EJECTED;
				break;


			case 0xE2: //$E2  n ??	Deactivation of device Dn:
				if ( command[2] >= DEVICESNUM ) goto Send_ERR_and_Delay;
				GetvDiskPtr(command[2])->flags &= ~FLAGS_DRIVEON; //vDisk[command[2]].flags &= ~FLAGS_DRIVEON;
				goto Send_CMPL_and_Delay;
				break;

			case 0xE3: //$E3  n ??	Set actual directory by device Dn: and get filename. 8.3 + attribute + fileindex [11+1+2=14]
			   {
				if ( command[2] >= DEVICESNUM) goto Send_ERR_and_Delay;
				//if (!(vDisk[command[2]].flags & FLAGS_DRIVEON))
				if (!(GetvDiskPtr(command[2])->flags & FLAGS_DRIVEON))
				{
					//v teto jednotce neni nic, takze vrati 14 bytu prazdny buffer
					//a NEBUDE menit adresar!

Command_FD_E3_empty:	//sem skoci z commandu FD kdyz chce vratit
						//prazdnych 14 bytu jakoze neuspesny vysledek misto nalezeneho
						//fname 8+3+atribut+fileindex, tj. jako command E3

Command_ED_notfound:	//sem skoci z commandu ED kdyz nenajde hledany nazev souboru
						//vraci 14 prazdnych bytu jakoze neuspesny vysledek

					Clear_atari_sector_buffer(14);

					USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(14);
					break;
				}
				// vDisk <- vDisk[ command[2] ]
                fi->vDisk = vDisk[ command[2] ];
			   }

				// VOLANI GET DETAILED FILENAME
				command[2]=(unsigned char)(fi->vDisk.file_index & 0xff);
				command[3]=(unsigned char)(fi->vDisk.file_index >> 8);

				// a pokracuje dal!!!

			case 0xE4: 	//$E4 xl xh	Get filename xhxl. 8.3 + attribute [11+1=12]
			case 0xE5:	//$E5 xl xh	Get more detailed filename xhxl. 8.3 + attribute + size/date/time [11+1+8=20]
			   {
				unsigned char ret,len;
				//ret=fatGetDirEntry((command[2]+(command[3]<<8)),0);
				ret=fatGetDirEntry(TWOBYTESTOWORD(command+2),0);

				if (0)
				{
Command_FD_E3_ok:	//sem skoci z commandu FD kdyz chce vratit
					//fname 8+3+atribut+fileindex, tj. stejne jako vraci command E3
Command_ED_found:	//sem skoci z commandu ED kdyz najde hledane filename a chce vratit
					//fname 8+3+atribut+fileindex, tj. stejne jako vraci command E3
					command[1]=0xE3;
					ret=1;
				}

				atari_sector_buffer[11]=(unsigned char)fi->Attr;
				if (command[1] == 0xE3)
				{
					//command 0xE3
					/*
					atari_sector_buffer[12]=(unsigned char)(fi->vDisk.file_index&0xff);
					atari_sector_buffer[13]=(unsigned char)(fi->vDisk.file_index>>8);
					*/
					TWOBYTESTOWORD(atari_sector_buffer+12)=fi->vDisk.file_index; //12,13
					len=14;
				}
				else
				if (command[1] == 0xE5)
				{
					unsigned char i;
					//command 0xE5
					/*
					atari_sector_buffer[12]=(unsigned char)(fi->vDisk.Size&0xff);
					atari_sector_buffer[13]=(unsigned char)(fi->vDisk.Size>>8);
					atari_sector_buffer[14]=(unsigned char)(fi->vDisk.Size>>16);
					atari_sector_buffer[15]=(unsigned char)(fi->vDisk.Size>>24);
					*/
					FOURBYTESTOLONG(atari_sector_buffer+12)=fi->vDisk.size; //12,13,14,15
					//atari_sector_buffer[16]=(unsigned char)(fi->Date&0xff);
					//atari_sector_buffer[17]=(unsigned char)(fi->Date>>8);
					TWOBYTESTOWORD(atari_sector_buffer+16)=fi->Date; //16,17
					//atari_sector_buffer[18]=(unsigned char)(fi->Time&0xff);
					//atari_sector_buffer[19]=(unsigned char)(fi->Time>>8);
					TWOBYTESTOWORD(atari_sector_buffer+18)=fi->Time; //18,19

					//1234567890 YYYY-MM-DD HH:MM:SS
					//20-49
					//20-29 string decimal size
					for(i=20; i<=49; i++) atari_sector_buffer[i]=' ';
					{
					 unsigned long size=fi->vDisk.size;
					 if ( !(fi->Attr & (ATTR_DIRECTORY|ATTR_VOLUME)) )
					 {
					 	//pokud to neni directory nebo nazev disku (volume to ani nemuze byt - getFatDirEntry VOLUME vynechava)
						//zapise na posledni pozici "0" (jakoze velikost souboru = 0 bytes)
						//(bud ji nasledne prepise skutecnou velikosti, nebo tam zustane)
						atari_sector_buffer[29]='0';
					 }
					 for(i=29; (i>=20); i--)
					 {
					  if (size) //>0
					  {
					   atari_sector_buffer[i]= 0x30 | ( size % 10 );	//cislice '0'-'9'
					   size/=10;
					  }
					 }
				    }
//					atari_sector_buffer[32]=' '; //je promazane ve smycce predtim
//					atari_sector_buffer[43]=' '; //je promazane ve smycce predtim
					//33-42 string date YYYY-MM-DD
					atari_sector_buffer[35]='-';
					atari_sector_buffer[38]='-';
					//44-51 string time HH:MM:SS
					atari_sector_buffer[44]=':';
					atari_sector_buffer[47]=':';
					{
					 unsigned short d;
					 //date
					 d=(fi->Date>>9)+1980;
					 PutDecimalNumberToAtariSectorBuffer(d/100,31);
					 PutDecimalNumberToAtariSectorBuffer(d%100,33);
					 //
					 PutDecimalNumberToAtariSectorBuffer((fi->Date>>5)&0x0f,36);
					 //atari_sector_buffer[36]=0x30|(d/10);
					 //atari_sector_buffer[37]=0x30|(d%10);
					 PutDecimalNumberToAtariSectorBuffer((fi->Date)&0x1f,39);
					 //atari_sector_buffer[39]=0x30|(d/10);
					 //atari_sector_buffer[40]=0x30|(d%10);
					 //time
					 PutDecimalNumberToAtariSectorBuffer((fi->Time>>11),42);
					 //atari_sector_buffer[42]=0x30|(d/10);
					 //atari_sector_buffer[43]=0x30|(d%10);
					 PutDecimalNumberToAtariSectorBuffer((fi->Time>>5)&0x3f,45);
					 //atari_sector_buffer[45]=0x30|(d/10);
					 //atari_sector_buffer[46]=0x30|(d%10);
					 PutDecimalNumberToAtariSectorBuffer(((fi->Time)&0x1f)<<1,48);	//doubleseconds
					 //atari_sector_buffer[48]=0x30|(d/10);
					 //atari_sector_buffer[49]=0x30|(d%10);
					}
					len=50;
				}
				else
					len=12;	//command 0xE4

				//pokud fatGetDirEntry neproslo, promaze buffer
				//(delka len musi zustat, aby posilal prazdny buffer prislusne delky)
				if (!ret)
				{
				 //50 je maximalni delka ze 12,14 a 50
				 //unsigned char i;
				 //for(i=0; i<50; i++) atari_sector_buffer[i]=0x00;	
 				 Clear_atari_sector_buffer(50);
				}

				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(len);
			   }
			   break;

			case 0xE6:	//$E6 xl xh	Get longfilename xhxl. [256]
			case 0xE7:
			case 0xE8:
			case 0xE9:
			   {
				unsigned short i;

				i=0;
				if (fatGetDirEntry(TWOBYTESTOWORD(command+2),1))
				{
				 //nasel, takze se posune i-ckem az na konec longname
				 while(atari_sector_buffer[i++]!=0);	//!!!!!!!!!!!!!!!!! pokud by longname melo 256 znaku, zasekne se to tady!!! Bacha!!!
				}
				while(i<256) atari_sector_buffer[i++]=0x00; //smaze zbytek

				//jakou delku chce z Atarka prebirat?
				i=(0x0100>>(command[1]-0xE6)); //=256,128,64,32

				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(i);
			   }
			   break;

			case 0xEA:	//$EA ?? ??	Get number of items in actual directory [<2]
			   {
			   	unsigned short i;
				i=0;
				while (fatGetDirEntry(i,0)) i++;
				TWOBYTESTOWORD(atari_sector_buffer+0)=i;	//0,1
				USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(2);
			   }
			   break;

			case 0xEB:	//$EB  n ??	GetPath (n=pocet updir skoku, if(n==0||n>20) n=20 )   [<n*12]  pro n==0 [<240]
						//         	Get path (/DIRECTOREXT/../DIRECTOREXT pokracuje 0x00) (nemeni aktualni adresar!)
         				//			Pokud je cesta do rootu delsi nez pozadovany pocet skoku, prvni znak je '?'
			   {
			     unsigned long ocluster, acluster;
				 unsigned char pdi;
				 u08 updirs;
				 if (command[2]==0 || command[2]>20) command[2]=20; //maximum [<240]
				 updirs=command[2]; //pocet updir skoku
				 pdi=0; //index do bufferu kam zapisuje podadresare
				 ocluster=fi->vDisk.dir_cluster; //pro obnoveni na konec
				 while ( fatGetDirEntry(0,0) //0.polozka by mela byt ".." ("." se vynechava)		//fatGetDirEntry(1,0)
				 	  && (fi->Attr & ATTR_DIRECTORY)
					  && atari_sector_buffer[0]=='.' && atari_sector_buffer[1]=='.' )
				 {
					if (!updirs)	//pocet updir skoku
					{
					 //uz udelal tolik updir skoku kolik chtel
					 //a protoze se tim jeste porad nedostal do rootu,
					 // cesta bude ?adresar1/adresar2
					 atari_sector_buffer[pdi]='?';
					 break;
					}
					updirs--;
					//Schova si cluster zacatku nynejsiho adresare
					acluster=fi->vDisk.dir_cluster;
					//Meni adresar
					fi->vDisk.dir_cluster=fi->vDisk.start_cluster;
					//jde najit nazev opusteneho adresare a ulozi ho na konec bufferu "/8+3"
					unsigned short i;
					for(i=0; fatGetDirEntry(i,0); i++)
					{
						if (acluster==fi->vDisk.start_cluster)
						{
						 //nasel ho, zkopiruje na konec bufferu
						 u08 *spt,*dpt;
						 u08 j;
						 pdi-=12;
						 spt=atari_sector_buffer;
						 dpt=atari_sector_buffer+pdi;
						 *dpt++='/';	//zacne '/'
						 j=11; do { *dpt++=*spt++; j--; } while(j); //pokracuje 11 znaku dirname
						 break;
						}
					}
				  }
				  //nastavi zpatky puvodni adresar
				  fi->vDisk.dir_cluster=ocluster;
				  //posune nalezenou cestu na zacatek bufferu
				  {
				   unsigned short k;
				   unsigned char m;
				   for(k=pdi,m=0; k<256; k++,m++) atari_sector_buffer[m]=atari_sector_buffer[k];
				   if (!m) { atari_sector_buffer[0]=0; m++; }
				   for(; m!=0; m++) atari_sector_buffer[m]=0; //ukonceni+promaze zbytek bufferu
				  }
				  USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(command[2]*12);
			    }
			 	break;

			case 0xEC: //$EC  n  m	Find filename 8+3.
			           //(Predava se vzor 11 bytu, nulove znaky jsou ignorovany).
					   // If (m<>0) set fileOrdirectory to vDn:

				//FileFindBuffer <> atari_sector_buffer !!!
				if (USART_Get_buffer_and_check_and_send_ACK_or_NACK(&FileFindBuffer[0],11))
				{
					break;
				}

				if (!command[3]) goto Send_CMPL_and_Delay;

				//zmeni command na 0xf0-0xff (pro nastaveni nalezeneho souboru/adresare)				
				command[1]=(0xf0|command[2]);
				command[2]=command[3]=0; //vyhledavat od indexu 0
				//a pokracuje dal...
				//commandem ED...

			case 0xED: //$ED xl xh       Find filename (8+3) from xhxl index in actual directory, get filename 8.3+attribute+fileindex [11+1+2=14]

			    {
					unsigned short i;
					u08 j;

					i=TWOBYTESTOWORD(command+2);  //zacne hledat od tohoto indexu

					while(fatGetDirEntry(i,0))
					{
					 for(j=0; (j<11); j++)
					 {
					 	if (!FileFindBuffer[j]) continue; //znaky 0x00 ve vzoru ignoruje (mohou byt jakekoliv)
						if (FileFindBuffer[j]!=atari_sector_buffer[j]) goto Different_char; //rozdilny znak
					 }
					 //nalezeny nazev vyhovuje vzoru
					 if (command[1]>=0xf0)
					 {
					  //prisel sem z commandu EC (ktery ho nastavil na $f0-$ff)
					  //takze jde nastavit ten vyhledany soubor
					  goto Command_EC_F0_FF_found;
					 }
					 goto Command_ED_found;
Different_char:
					 i++;
					}

					//nenalezl zadny nazev vyhovujici vzoru

					if (command[1]>=0xf0)
					{
						//prisel sem z commandu EC (ktery ho nastavil na $f0-$ff)
						goto Send_ERR_and_Delay;
					}

					goto Command_ED_notfound;
				}
				break;

			case 0xEE:	//$EE  n ??	Swap vDn: (n=0..4) with D{SDrivenum}:  (if n>=5) Set default (all devices with relevant numbers).

				//Prepnuti prislusneho drive do Dsdrivenum:
				//(if >=5) SetDefault
				actual_drive_number = ( command[2] < DEVICESNUM )? command[2] : (unsigned char)4-((unsigned char)(_driveShift&0x03));
				set_display(actual_drive_number);
				goto Send_CMPL_and_Delay;
				break;

			case 0xEF:	//$EF  n  m	Get SDrive parameters (n=len, m=from) [<n]
				{
				 u08 *spt,*dpt;
				 u08 n;
				 n=command[2];
				 spt=((u08*)&sdrparams)+command[3];	//m
				 dpt=atari_sector_buffer;
				 do { *dpt++=*spt++; n--; } while(n);
				 USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(command[2]);
				}
				break;

			case 0xFD:	//$FD  n ??	Change actual directory up (..). If n<>0 => get dirname. 8+3+attribute+fileindex [<14]
				 if (fatGetDirEntry(0,0))	//0.polozka by mela byt ".." ("." se vynechava)		//fatGetDirEntry(1,0)
				 {
					if ( (fi->Attr & ATTR_DIRECTORY)
						&& atari_sector_buffer[0]=='.' && atari_sector_buffer[1]=='.')
					{
						//Schova si cluster zacatku nynejsiho adresare
						unsigned short acluster;
						acluster=fi->vDisk.dir_cluster;
						//Meni adresar
						fi->vDisk.dir_cluster=fi->vDisk.start_cluster;
						//aux1 ?
						if (command[2])
						{
						 //chce vratit 8+3+atribut+fileindex
						 unsigned short i;
						 for(i=0; fatGetDirEntry(i,0); i++)
						 {
							if (acluster==fi->vDisk.start_cluster)
							{
							 //skoci do casti predavani jmena 8+3+atribut+fileindex
							 goto Command_FD_E3_ok; //nasel ho!
							}
						 }
						 //nenasel ho vubec
						 //tohle nemusime resit, nebot to nikdy nemuze nastat!
						 //(znamenalo by to nekonzistentni FATku)
						 //tj. cluster z ".." by ukazoval na adresar ve kterem by nebyl adresar
						 //s clusterem ukazujicim na tento podadresar.
						}
					}
				 }
				 //nulta polozka neexistuje nebo neni ".."
				 //coz znamena ze jsme uz v rootu
				 if (command[2])
				 {
					  //chce vratit filename 8+3+atribut+fileindex
					  //takze musi dostat 14 prazdnych bajtu
					  //u08 i;
					  //for(i=0; i<14; i++) atari_sector_buffer[i]=0;
					  //USART_Send_cmpl_and_atari_sector_buffer_and_check_sum(14);
					  goto Command_FD_E3_empty;
				 }
				 goto Send_CMPL_and_Delay;
				 break;

			case 0xFE:	//$FE ?? ??	Change actual directory to rootdir.
				fi->vDisk.dir_cluster=MSDOSFSROOT;
				fatGetDirEntry(0,0);
				goto Send_CMPL_and_Delay;
				break;

			case 0xF0:	// set direntry to vD0:
			case 0xF1:	// set direntry to vD1:
			case 0xF2:	// set direntry to vD2:
			case 0xF3:	// set direntry to vD3:
			case 0xF4:	// set direntry to vD4:
			case 0xF5:	// set direntry to vD5:
			case 0xF6:	// set direntry to vD6:
			case 0xF7:	// set direntry to vD7:
			case 0xF8:	// set direntry to vD8:
			case 0xFF:  // set actual directory
				{
				unsigned char ret;

				ret=fatGetDirEntry(TWOBYTESTOWORD(command+2),0); //2,3

				if(ret)
				{
Command_EC_F0_FF_found:
					if( (fi->Attr & ATTR_DIRECTORY) )
					{
						//Meni adresar
						fi->vDisk.dir_cluster=fi->vDisk.start_cluster;
					}
					else
					{
						//Aktivuje soubor
						fi->vDisk.current_cluster=fi->vDisk.start_cluster;
						fi->vDisk.ncluster=0;

						if( atari_sector_buffer[8]=='A' &&
							 atari_sector_buffer[9]=='T' &&
							  atari_sector_buffer[10]=='R' )
						{
							// ATR
                            setAtrVdiskFlags();
							/*unsigned long compute;
							unsigned long tmp;

							faccess_offset(FILE_ACCESS_READ,0,16); //je to ATR
									
							fi->vDisk.flags=FLAGS_DRIVEON;
							//fi->vDisk.atr_sector_size = (atari_sector_buffer[4]+(atari_sector_buffer[5]<<8));
							if ( (atari_sector_buffer[4]|atari_sector_buffer[5])==0x01 )
								fi->vDisk.flags|=FLAGS_ATRDOUBLESECTORS;

							compute = fi->vDisk.size - 16;		//je to ATR
							tmp = (fi->vDisk.flags & FLAGS_ATRDOUBLESECTORS)? 0x100:0x80;		//fi->vDisk.atr_sector_size;
							compute /=tmp;
							if(compute>720) fi->vDisk.flags|=FLAGS_ATRMEDIUMSIZE; //atr_medium_size = 0x80;
                            */
						}
						else
						if( atari_sector_buffer[8]=='X' &&
							 atari_sector_buffer[9]=='F' &&
							  atari_sector_buffer[10]=='D' )
						{
							//XFD
							fi->vDisk.flags=(FLAGS_DRIVEON|FLAGS_XFDTYPE);
							//if ( fi->vDisk.Size == 92160 ) //normal XFD
							if ( fi->vDisk.size>92160)
							{
							 	if ( fi->vDisk.size<=133120)
							 	{
									// Medium size
									fi->vDisk.flags|=FLAGS_ATRMEDIUMSIZE;
							  	}
								else
								{
							   		// Double (fi->vDisk.Size == 183936)
									fi->vDisk.flags|=FLAGS_ATRDOUBLESECTORS;
							  	}
							}
						}
						else
						{
							// XEX
							fi->vDisk.flags=FLAGS_DRIVEON|FLAGS_XEXLOADER|FLAGS_ATRMEDIUMSIZE;
						}
            			if (fi->Attr & ATTR_READONLY) {
        					fi->vDisk.flags |= FLAGS_READONLY;
						}
			
					}

					ret=(command[1]&0x0f);	//0..f
					//0..DEVICESNUM ... nastaveni prislusneho vDx:
					//0x0f ... jen listovani adresari
					if (ret < DEVICESNUM )
					{
						//nastavi vD0: az vD4:
						unsigned char i;
						unsigned char *p1;
						unsigned char *p2;
						p1=((unsigned char *)&vDisk[ret]);
						p2=((unsigned char *)&fi->vDisk);
						// vDisk[ret] <- vDisk
						//for(i=0; i<sizeof(virtual_disk_t); i++) p1[i]=p2[i];
						for(i=0; i<sizeof(virtual_disk_t); i++) *p1++=*p2++;
						//
					}

					goto Send_CMPL_and_Delay;
				}
				else
				{
					goto Send_ERR_and_Delay;
				}
				
				}
				break;

			default:
				goto Send_ERR_and_Delay;	//nerozpoznany SDrive command


			} //switch

			if (0)
			{
Send_CMPL_and_Delay:
				CyDelayUs(800u);	//t5
				send_CMPL();
			}
			if (0)
			{
Send_ERR_and_Delay:
				CyDelayUs(800u);	//t5
				send_ERR();
			}
			//za poslednim cmpl nebo err neni potreba pauza, jde hned cekat na command
		} //sdrive operations

	}

	return 0;
}


/* [] END OF FILE */
