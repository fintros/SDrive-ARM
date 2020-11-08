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
#undef DEBUG
#include "dprint.h"
#include "sio.h"
#include "gpio.h"
#include "helpers.h"
#include "hwcontext.h"
#include "sdrive.h"

const unsigned long _freq = 48000000;

void USART_Set_Baud (unsigned int baud) {

    // fixed point 27.5
    unsigned long TargetDivider = _freq * 32 / 8 / baud;  // fixed point 27.5
    
    int IntDivider = TargetDivider / 32 ;
    int FractDivider = TargetDivider - IntDivider * 32;
    
    Clock_1_SetFractionalDividerRegister(IntDivider, FractDivider);
    UART_1_ClearRxBuffer();
    UART_1_ClearTxBuffer();
}


void USART_Init( unsigned char value )
{         
    unsigned long TargetSpeed = 0;
    
    switch(value-6)
    {
        case 0x28:
            TargetSpeed = 19040;       
            break;
        case 0x10:
            TargetSpeed = 38908;       
            break;
        case 0xa:
            TargetSpeed = 52641;       
            break;
        case 0x09:
            TargetSpeed = 55931;       
            break;
        case 0x08:
            TargetSpeed = 59660;       
            break;
        case 0x07:
            TargetSpeed = 63921;       
            break;
        case 0x06:
            TargetSpeed = 68838;       
            break;
        case 0x05:
            TargetSpeed = 74575;
            break;
        default:
            TargetSpeed = _freq / 8 * value / 19040 / 46;
    }

    USART_Set_Baud(TargetSpeed);
//    dprint("Change UART speed %d: [%d, %d] to %d\r\n", value, IntDivider, FractDivider, (int)(48000000*32/(IntDivider*32 + FractDivider)/8));    
}

void USART_Transmit_Byte( unsigned char data )
{
    UART_1_WriteTxData(data);
    while(UART_1_GetTxBufferSize());
}

unsigned char USART_Receive_Byte( void )
{
	/* Wait for data to be received */
    unsigned short value = 0;
    while(!UART_1_GetRxBufferSize());
    value = UART_1_ReadRxData();   
	return value & 0xff;
}


void USART_Send_Buffer(unsigned char *buff, unsigned short len)
{
    unsigned short bufIndex = 0u;

    while(bufIndex < len)
    {
        UART_1_PutChar(buff[bufIndex]);
        while(UART_1_GetTxBufferSize() > 2);
        bufIndex++;
    }
    while(UART_1_GetTxBufferSize());
}

unsigned char USART_Get_Buffer_And_Check(unsigned char *buff, unsigned short len, unsigned char cmd_state)
{
	unsigned char *ptr, b;
	unsigned short n;
	ptr=buff;
	n=len;
	while(1)
	{ 
		// Wait for data to be received
        unsigned short value = 0;
 		while(!UART_1_GetRxBufferSize())
		{
			// if command state changed - return
			if ( get_cmd_H()!=cmd_state ) return 0x01;
    		//if ( read_key()!=last_key ) return 0x02;
        }
        
        value = UART_1_ReadRxData();
            
		// Get and return received data from buffer
		b = value & 0xff;
		if (!n)
		{
			//v b je checksum (n+1 byte)
			if ( b!=get_checksum(buff,len) ) return 0x80;	// checksum error
			return 0x00; //ok
		}
		*ptr++=b;
		n--;
	}
}

unsigned char USART_Get_buffer_and_check_and_send_ACK_or_NACK(unsigned char *buff, unsigned short len)
{
	unsigned char err;
	err = USART_Get_Buffer_And_Check(buff,len,CMD_STATE_H);

	CyDelayUs(1000u);	//t4
	if(err)
	{
		send_NACK();
		return 0xff;
	}
	send_ACK();
	return 0;
}

void USART_Send_status_and_buffer_and_check_sum(unsigned char *buff, unsigned short len, unsigned char is_error, unsigned char do_wait)
{
	unsigned char check_sum;

    if(do_wait)
	    CyDelayUs(800u);	//t5
        
    if(!is_error)
    {
	    send_CMPL();
    }
    else
    {
	    send_ERR();
    }
        
	CyDelayUs(200u);

	USART_Send_Buffer(buff, len);
	check_sum = get_checksum(buff, len);
	USART_Transmit_Byte(check_sum);
    while(UART_1_GetTxBufferSize());
}
