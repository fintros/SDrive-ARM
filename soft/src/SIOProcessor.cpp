#include "project.h"
#include "SIOProcessor.h"

CSIOProcessor::CSIOProcessor(HWContext& context): _context(context);
{
    _hiSpeedDiv = US_POKEY_DIV_DEFAULT;
    _hiSpeedEnabled = false;
    Init(ATARI_SPEED_STANDARD);
}
CSIOProcessor::~CSIOProcessor()
{

}

void CSIOProcessor::Init(int pokeyDiv)
{
    const unsigned long freq = 48000000;
    
    unsigned long TargetSpeed = 0;
    
    switch(pokeyDiv-6)
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
            TargetSpeed = freq / 8 * value / 19040 / 46;
    }

    unsigned long TargetDivider = freq * 32 / 8 / TargetSpeed;  // fixed point 27.5
    int IntDivider = TargetDivider / 32 ;
    int FractDivider = TargetDivider - IntDivider * 32;
    Clock_1_SetFractionalDividerRegister(IntDivider, FractDivider);
    UART_1_ClearRxBuffer();
    UART_1_ClearTxBuffer();
#if DEBUG_SIO    
    dprint("Change UART speed %d: [%d, %d] to %d\r\n", value, IntDivider, FractDivider, (int)(48000000*32/(IntDivider*32 + FractDivider)/8));    
#endif
}

unsigned char get_checksum(unsigned char* buffer, int len)
{
	int i;
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

void CSIOProcessor::Send(unsigned char* ptr, int len)
{
#if DEBUG_SIO
    dprint("Send CMPL and %d bytes with checksum\r\n", len);
    for(int i = 0; i < len;i+=16)
    {
        dprint("%02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
            ptr[i], ptr[i+1], ptr[i+2], ptr[i+3],  ptr[i+4],
            ptr[i+5], ptr[i+6], ptr[i+7], ptr[i+8],
            ptr[i+9], ptr[i+10], ptr[i+11], ptr[i+12],
            ptr[i+13], ptr[i+14], ptr[i+15]                        
        );
    }
#endif  
	CyDelayUs(800u);
	SendCMPL();
	CyDelayUs(100u);
    UART_1_PutArray(atari_sector_buffer, len);
	USART_Transmit_Byte(get_checksum(atari_sector_buffer,len));
}



bool CSIOProcessor::RecvBufferWithState(unsigned char *buff, int len, bool cmd_state)
{
   for(int i = 0; i < len+1; i++)
    {
		// Wait for data to be received
 		while(!UART_1_GetRxBufferSize())
		{
			// wait until state will not be changed
			if ( get_cmd_H()!=(cmd_state?1:0) ) return false;
        }
        unsigned char value = UART_1_ReadRxData();

        if(i == len)
            return value == get_checksum(buff,len) 

        buff[i] = value;
    }
    return false;
}

bool CSIOProcessor::GetCommand(SIOCommand& command)
{
    bool result = RecvBufferWithState(&command, sizeof(SIOCommand), false); 
#if DEBUG_SIO
    if(!result)
        dprint("Get command error\r\n");
    else
        dprint("Command: [%02X %02X %02X %02X]\r\n", 
                command.device, 
                command.command, 
                command.aux1,
                command.aux2);
#endif        
   	CyDelayUs(800u);
	WaitCmdHigh();
	CyDelayUs(100u);


}

bool CSIOProcessor::Recive(unsigned char *buff, int len)
{
	bool result = USART_Get_Buffer_And_Check(buff,len);
	
    CyDelayUs(1000u);

	if(result)
        SendACK();
    else
        SendNACK();
    
    return result;
}

bool CSIOProcessor::ToggleSpeed()
{
    _hiSpeedEnabled = !_hiSpeedEnabled;
    Init(_hiSpeedEnabled?_hiSpeedDiv:ATARI_SPEED_STANDARD);
    return _hiSpeedEnabled;
}

bool CSIOProcessor::SetFastIO(int pokeyDiv)
{
    if(_hiSpeedDiv != (pokeyDiv+6))
    {
        _hiSpeedDiv = pockeyDiv + 6;
        if(_hiSpeedEnabled)
            Init(_hiSpeedDiv);
    }
    return _hiSpeedEnabled;
}

void CSIOProcessor::SendACK()
{
    USART_Transmit_Byte('A'); 
#if DEBUG_SIO
    dprint("Send ACK\r\n");
#endif
}

void CSIOProcessor::SendNACK()
{
    USART_Transmit_Byte('N');  
#if DEBUG_SIO
    dprint("Send NACK\r\n");
#endif
}

void CSIOProcessor::SendCMPL()
{
    USART_Transmit_Byte('C');
#if DEBUG_SIO  
    dprint("Send CMPL\r\n");
#endif
}

void CSIOProcessor::SendERR()
{
    USART_Transmit_Byte('E');
#if DEBUG_SIO  
    dprint("Send ERR\r\n");    
#endif
}

bool CSIOProcessor::WaitCmdLow()
{
    while ( Command_Read() );
    return true;
}

bool CSIOProcessor::WaitCmdHigh()
{
    while ( ! Command_Read() );
    return true;
}
