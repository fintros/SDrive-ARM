#include "project.h"
#include <CyLib.h>
#include "hwcontext.h"
#include "tapeemu.h"
#include "fat.h"
#include "sio.h"
#include "gpio.h"
#include "dprint.h"
#include "helpers.h"
#include "sdrivecmd.h"

#define TAPE_BLOCK_LEN 128

typedef struct
{
	char chunk_type[4];
	unsigned short chunk_length;
	unsigned short irg_length;
	char data[];
} __attribute__((packed)) tape_header_t ;

typedef struct
{
	file_t file;
	unsigned int baud;
	unsigned int block;
	unsigned char run;
	unsigned char is_fuji;
	unsigned int tape_baud;

} tape_file_t;


static tape_file_t tape;

int send_fuji_tape_block(HWContext *ctx, tape_file_t *tape, int offset, unsigned char *buffer)
{
	unsigned short r;
	unsigned short gap, len;
	unsigned short buflen = 256;
	unsigned char first = 1;
	tape_header_t *hdr = (tape_header_t *)buffer;
	char *p = hdr->chunk_type;

	while ((unsigned long) offset < tape->file.size)
	{
		//read header
		faccess_offset(ctx, &tape->file, FILE_ACCESS_READ, offset, buffer, sizeof(tape_header_t));
		len = hdr->chunk_length;

		if (p[0] == 'd' && //is a data header?
			p[1] == 'a' &&
			p[2] == 't' &&
			p[3] == 'a')
		{
			tape->block++;
			break;
		}
		else if (p[0] == 'b' && //is a baud header?
				 p[1] == 'a' &&
				 p[2] == 'u' &&
				 p[3] == 'd')
		{
			if (tape->tape_baud == 600) //ignore baud hdr
            {
    		    tape->baud = hdr->irg_length;
    			USART_Set_Baud(tape->baud);                
            }            
		}
		offset += sizeof(tape_header_t) + len;
	}

	gap = hdr->irg_length; //save GAP
	len = hdr->chunk_length;
	dprint("Baud: %u Length: %u Gap: %u\r\n", tape->baud, len, gap);

	CyDelay(gap); //wait GAP

	if ((unsigned long)offset < tape->file.size)
	{ //data record
		dprint("Block %u\r\n", tape->block);

		//read block
		offset += sizeof(tape_header_t); //skip chunk hdr
		while (len)
		{
			if (len > 256)
				len -= 256;
			else
			{
				buflen = len;
				len = 0;
			}
			r = faccess_offset(ctx, &tape->file, FILE_ACCESS_READ, offset, buffer, buflen);
			offset += r;
            LED_GREEN_ON;            
			USART_Send_Buffer(buffer, buflen);
            LED_GREEN_OFF;
            if (first && buffer[2] == 0xfe)
			{
				//most multi stage loaders starting over by self
				// so do not stop here!
				tape->block = 0;
			}
			first = 0;
		}
		if (tape->block == 0)
			CyDelay(200); //add an end gap to be sure
	}
	else
		offset = -1;    // end of tape

    return offset;
}

int send_basic_tape_block(HWContext *ctx, tape_file_t *tape, int offset, unsigned char *buffer)
{
	unsigned char r;

	if ((unsigned long) offset < tape->file.size)
	{ //data record
		dprint("Block %u / %u\r\n", offset / TAPE_BLOCK_LEN + 1, (tape->file.size - 1) / TAPE_BLOCK_LEN + 1);
		//read block
		r = faccess_offset(ctx, &tape->file, FILE_ACCESS_READ, offset, buffer + 3, TAPE_BLOCK_LEN);
		if (r < TAPE_BLOCK_LEN)
		{					  //no full record?
			buffer[2] = 0xfa; //mark partial record
			buffer[130] = r;  //set size in last byte
		}
		else
			buffer[2] = 0xfc; //mark full record

		offset += r;
	}
	else
	{ //this is the last/end record
		dprint("End\r\n");
		memset(buffer, 0, TAPE_BLOCK_LEN + 3);
		buffer[2] = 0xfe; //mark end record
		offset = -1;
	}
	buffer[0] = 0x55; //sync marker
	buffer[1] = 0x55;
    LED_GREEN_ON;
	USART_Send_Buffer(buffer, TAPE_BLOCK_LEN + 3);
	USART_Transmit_Byte(get_checksum(buffer, TAPE_BLOCK_LEN + 3));
    LED_GREEN_OFF;
	CyDelay(300); //PRG(0-N) + PRWT(0.25s) delay
	return (offset);
    
}

int send_tape_block(HWContext *ctx, int offset, unsigned char *buffer)
{
    if(offset < 0)
        return offset;
    
    if(tape.is_fuji)
        return send_fuji_tape_block(ctx, &tape, offset, buffer);
    else
        return send_basic_tape_block(ctx, &tape, offset, buffer);
}

static int tape_offset = 0;

int MountTape(HWContext *ctx, file_t * folder, const char* file_name, Settings* settings, unsigned char *buffer)
{
    memset(&tape, 0 , sizeof(tape_file_t));
    tape.file.dir_cluster = folder->dir_cluster;
    if(MountFile(ctx, &tape.file, file_name, buffer))
    {
        dprint("Unable to mount %s to tape\r\n", file_name);
        return 1;
    }
    else
    {
    	tape_header_t *hdr = (tape_header_t *)buffer;
    	char *p = hdr->chunk_type;

        tape.tape_baud = settings->tape_baud;

    	faccess_offset(ctx, &tape.file, FILE_ACCESS_READ, 0, buffer, sizeof(tape_header_t));
        
    	tape.is_fuji = (p[0] == 'F' && p[1] == 'U' && p[2] == 'J' &&p[3] == 'I')?1:0;
    		
        tape.baud = settings->tape_baud;

    	USART_Set_Baud(tape.baud);

    	tape.block = 0;
        
        tape_offset = 0;
        
        dprint("Mounted %s to tape\r\n", file_name);
    }
   
    return 0;       
}

unsigned char led_state = 0;

CY_ISR(LedISR)
{
    led_state <<= 1;
    if(led_state > 8)
        led_state = 1;
    set_display(led_state);
}

void ProceedTape(HWContext* ctx, unsigned char *buffer)
{   
    if(( tape_offset >=0 ) && (SWBUT_Read() & 0x10))
    {
        if(tape_offset == 0)
        {
            HelperISR_StartEx(LedISR);
            led_state = 1;
            set_display(led_state);
        }
        
        tape_offset = send_tape_block(ctx, tape_offset, buffer);

        if(tape_offset < 0)
        {
            HelperISR_Stop();
            set_display(0);        
        }
    }        
}

