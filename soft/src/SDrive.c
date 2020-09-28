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
#include "diskemu.h"
#include "sdrivecmd.h"

SharedParameters shared_parameters;
Settings settings;

int wait_for_command()
{
    static unsigned long autowritecounter = 0;
    static unsigned char last_key = 0;
    HWContext* ctx = (HWContext*)__hwcontext;

    while( get_cmd_H() )	//dokud je Atari signal command na H
    {           
        unsigned char actual_key = get_actual_key();
           
        if(actual_key != last_key)
        {
            dprint("Key: %02X\r\n", actual_key);

            switch(actual_key)
        	{
        		case 6: ///BOOT KEY
        			if(InitBootDrive(&ctx->atari_sector_buffer[0]))
                        return -1;
                    shared_parameters.actual_drive_number = 0;
        			break;;
        		case 5: //LEFT
        			if(shared_parameters.actual_drive_number>1)
        				shared_parameters.actual_drive_number--;
        			else
        				shared_parameters.actual_drive_number = DEVICESNUM-1;
        			break;
        		case 3: //RIGHT
        			if(shared_parameters.actual_drive_number < ( DEVICESNUM-1))
        				shared_parameters.actual_drive_number++;
        			else
        				shared_parameters.actual_drive_number=1;
        			break;
        	}
        	set_display(shared_parameters.actual_drive_number);
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


int get_command(unsigned char* command)
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
            return 1;

        // if not standard rate - change fastsio flag
		if (shared_parameters.fastsio_pokeydiv!=US_POKEY_DIV_STANDARD)
			shared_parameters.fastsio_active = !shared_parameters.fastsio_active;
        
        USART_Init(6 + (shared_parameters.fastsio_active?shared_parameters.fastsio_pokeydiv:US_POKEY_DIV_STANDARD));
        
        return 1;
	}

    return 0;
}


//----- Begin Code ------------------------------------------------------------
int sdrive(void)
{
    dprint("SDrive started\r\n");

    HWContext* ctx = (HWContext*)__hwcontext;
    unsigned char* atari_sector_buffer = &ctx->atari_sector_buffer[0];
    
    // todo setting read from settings file
    settings.emulated_drive_no = 1; // Emulate D1 for a while
    settings.default_pokey_div = US_POKEY_DIV_DEFAULT; 

	unsigned char command[5];

    LED_OFF;
	//Parameters

    init_keys();
    
    Debug_B_Start();

    LED_ON;
    
    dprint("SDrive initialized\r\n");

	for(;;)
	{
    	shared_parameters.fastsio_active=0;
        shared_parameters.bootloader_relocation=0;
    	shared_parameters.fastsio_pokeydiv = settings.default_pokey_div;
           
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

        ResetDrives();

        // set boot drive to active D1:
        shared_parameters.actual_drive_number = 0;
    	if(InitBootDrive(&ctx->atari_sector_buffer[0]))
            continue;
        
    	set_display(shared_parameters.actual_drive_number);

    	for(;;)
    	{
            int res;
            
    		LED_GREEN_OFF;
            if((res = wait_for_command()) > 0)
                continue;
            
            if((res = get_command(command)) > 0)
                continue;

    		LED_GREEN_OFF;

    		if( command[0]>=0x31 && command[0]<(0x30+DEVICESNUM) ) //D1: to DX: X is DEVICESNUM
    		{
                if((res = SimulateDiskDrive(command, &atari_sector_buffer[0])) > 0)
                    continue;
    		} 
    		else if( command[0] == (unsigned char)0x70 + settings.emulated_drive_no)
    		{
                if((res = DriveCommand(command, &atari_sector_buffer[0])) > 0)
                    continue;
            }
            dprint("res: %d\r\n", res);
            // to do process cassete and printer
            if(res < 0)
                break;
        }
    }

	return 0;
}


/* [] END OF FILE */
