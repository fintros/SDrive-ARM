/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "project.h"
#include <CyLib.h>
#include "hwcontext.h"
#include "helpers.h"
#include "dprint.h"
#include "sdrive.h"
#include "gpio.h"

unsigned char get_checksum(unsigned char* buffer, unsigned short len)
{
	unsigned short i;
	unsigned char sumo,sum;
	sum=sumo=0;
	for(i=0;i<len;i++)
	{
		sum+=buffer[i];
		if(sum<sumo) sum++;
		sumo = sum;
	}
	return sum;
}

// DISPLAY
void set_display(unsigned char n)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    dprint("Set display to %d, %02X\r\n", n, ctx->LEDREG_Read());
    // D0 is boot disk - all LEDs on
    if(!n)  
        n = 15;
    if(n <= 0x0F)
    {
       unsigned char set = ~(n << 2); 
       ctx->LEDREG_Write((ctx->LEDREG_Read() | 0x3C) & set);
    }
}


void StartReadOperation()
{
    LED_GREEN_ON;    
}

void StopReadOperation()
{
    LED_GREEN_OFF;    
}

void StartWriteOperation()
{
    LED_RED_ON;    
}

void StopWriteOperation()
{
    LED_RED_OFF;    
}
