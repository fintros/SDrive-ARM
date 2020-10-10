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

int wait_for_command(HWContext* ctx)
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
        			if(InitBootDrive(ctx, &ctx->atari_sector_buffer[0]))
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
         	mmcWriteCachedFlush(ctx);
        	autowritecounter=0;
        }
        else
         autowritecounter++;
    }
    return 0;
}

#define UART_SAMPLES 40
unsigned char vals[UART_SAMPLES];
unsigned int no;

CY_ISR(SlowUartISR)
{
    if(no < UART_SAMPLES)
    {        
        unsigned int count = SlowUARTCounter_COUNTER_REG & SlowUARTCounter_16BIT_MASK;
        SlowUARTCounter_COUNTER_REG = 0;
        vals[no++] = count | (SWBUT_Read()&1);  // timer already shift value by one bit right
    }
}

enum
{
    EStateWaitForStartBit = 0,
    EStateDataRecv,
    EStateWaitForStopBit,
    EStateError
};


int DecodeSlowUART(unsigned char* command, int max_len)
{
    int state = EStateWaitForStartBit;
    int byte_no = 0;
    int bit_in_byte = 0;
    unsigned char current_byte = 0;
    
    for(unsigned int i = 0; i < no; i++)
    {
        unsigned int bit = 1 - (vals[i] & 1);
        unsigned int count = vals[i] >> 1;

        while(count--)
        {
            switch(state)
            {
                case EStateWaitForStartBit:
                    if(bit)    // wait for '0'
                        break;
                    state = EStateDataRecv;
                    current_byte = 0;
                    bit_in_byte = 0;
                    break;
                case EStateDataRecv:
                   current_byte |= bit << bit_in_byte++;
                   if(bit_in_byte == 8)
                       state = EStateWaitForStopBit;
                   break;
                case EStateWaitForStopBit:
                    if(bit) // wait for '1'
                    {
                        if(byte_no < max_len)
                            command[byte_no++] = current_byte;
                        state = EStateWaitForStartBit;                   
                    }
                    break;
                case EStateError:
                    break;
            }   
        }
    }
    // fill out remaining bits in case '1' ended sequence
    if(bit_in_byte < 8)
    {        
        for(;bit_in_byte < 8; bit_in_byte++)
           current_byte |= 1 << bit_in_byte;
    
        if(byte_no < max_len)
            command[byte_no++] = current_byte;
    }

    #ifdef DEBUG
        dprint("Decoded CMD %d: [%02X %02X %02X %02X %02X]\r\n", byte_no, command[0], command[1], command[2], command[3], command[4]);        
    #endif
    
    if( command[4] != get_checksum(command, 4) )
        return 1;

    return 0;
}

int get_command(unsigned char* command)
{
    LED_OFF;
    memset(command, 0, 4);
    
    // try to catch on ordinary speed
    no = 0;
    SlowUARTCounter_WriteCounter(0);
    SlowUARTCounter_Start();
    SlowUartISR_StartEx(SlowUartISR);
        
	// end get as ususal
    unsigned char err = USART_Get_Buffer_And_Check(command,4,CMD_STATE_L);

	CyDelayUs(800u);	//t1 (650-950us) (Bez tehle pauzy to cele nefunguje!!!)
	wait_cmd_LH();	//ceka az se zvedne signal command na H
	CyDelayUs(100u);	//T2=100   (po zvednuti command a pred ACKem)
    
#ifdef DEBUG
    if(err)
        dprint("Command error: [%02X]\r\n", err);
    else
        dprint("Command: [%02X %02X %02X %02X], CS: [%02X]\r\n", command[0], command[1], command[2], command[3], command[4]);
#endif  

    SlowUartISR_Stop();
    SlowUARTCounter_Stop();
    
#ifdef DEBUG
    dprint("Catched: ");
    for(int i = 0; i < no; i++)
        dprint("[%d]%d,", 1 - (vals[i] & 1),  vals[i]>>1);
    dprint("\r\n");
#endif  
    if(err)
        err = DecodeSlowUART(command,5);
    
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
    		mmcInit(ctx);
    	}
    	while(mmcReset(ctx));	//Wait for SD card ready

        LED_GREEN_OFF;
        dprint("SD card inititialized\r\n");

        ResetDrives();

        // set boot drive to active D1:
        shared_parameters.actual_drive_number = 0;
    	if(InitBootDrive(ctx, &ctx->atari_sector_buffer[0]))
            continue;
        
    	set_display(shared_parameters.actual_drive_number);

    	for(;;)
    	{
            int res;
            
    		LED_GREEN_OFF;
            if((res = wait_for_command(ctx)) > 0)
                continue;
                        
            if((res = get_command(command)) > 0)
                continue;
                       
    		LED_GREEN_OFF;

    		if( command[0]>=0x31 && command[0]<(0x30+DEVICESNUM) ) //D1: to DX: X is DEVICESNUM
    		{
                if((res = SimulateDiskDrive(ctx, command, &atari_sector_buffer[0])) > 0)
                    continue;
    		} 
    		else if( command[0] == (unsigned char)0x70 + settings.emulated_drive_no)
    		{
                if((res = DriveCommand(ctx, command, &atari_sector_buffer[0])) > 0)
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
