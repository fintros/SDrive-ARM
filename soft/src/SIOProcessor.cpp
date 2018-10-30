#include "project.h"
#include "SIOProcessor.h"
#include "hal.h"

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
    HAL::GetSIO().ClearRx();
    HAL::GetSIO().ClearTx();
#if DEBUG_SIO    
    dprint("Change UART speed %d: [%d, %d] to %d\r\n", value, IntDivider, FractDivider, (int)(48000000*32/(IntDivider*32 + FractDivider)/8));    
#endif
}

static unsigned char get_checksum(unsigned char* buffer, int len)
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
    HAL::GetSIO().WriteBuffer(ptr, len);

    HAL::GetSIO().Write(get_checksum(ptr,len));
    while(HAL::GetSIO().GetTXFIFOSize());
}


/**
 * @brief receive buffer with len + 1 byte for checksum
 * 
 * @param buff pointer to output buffer
 * @param len length of received data
 * @param cmd_state proceed while CMD line in given state, otherwise - quit with error
 * @return true - if error happens
 * @return false - everything ok
 * 
 * TODO: break by key press?
 */
bool CSIOProcessor::RecvBufferWithState(unsigned char *buff, int len, CommadLineState cmd_state)
{
   for(int i = 0; i < len+1; i++)
    {
		// Wait for data to be received
 		while(!HAL::GetSIO().GetRxBufferSize())
		{
			// wait until state will not be changed
			if ( HAL::GetGPIO().GetCMDLineState() != (cmd_state==CLS_High) ) return true;
        }
        unsigned char value = HAL::GetSIO().Read();

        if(i == len)
            return value != get_checksum(buff,len) 

        buff[i] = value;
    }
    return true;
}

bool CSIOProcessor::GetCommand(SIOCommand& command)
{
    bool result = RecvBufferWithState(&command, sizeof(SIOCommand), CLS_Low); 
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
   	HAL::DelayUs(800u);
	WaitCmdHigh();
	HAL::DelayUs(100u);


}

bool CSIOProcessor::Recive(unsigned char *buff, int len)
{
	bool result = RecvBufferWithState(buff,len, HAL::GetGPIO().GetCMDLineState());
	
    HAL::DelayUs(1000u);

	if(result)
        SendNACK();
    else
        SendACK();
    
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
    SendByteAndWait('A'); 
#if DEBUG_SIO
    dprint("Send ACK\r\n");
#endif
}

void CSIOProcessor::SendNACK()
{
    SendByteAndWait('N');  
#if DEBUG_SIO
    dprint("Send NACK\r\n");
#endif
}

void CSIOProcessor::SendCMPL()
{
    SendByteAndWait('C');
#if DEBUG_SIO  
    dprint("Send CMPL\r\n");
#endif
}

void CSIOProcessor::SendERR()
{
    SendByteAndWait('E');
#if DEBUG_SIO  
    dprint("Send ERR\r\n");    
#endif
}

bool CSIOProcessor::WaitCmdLow()
{
    while ( HAL::GetGPIO().GetCMDLineState() );
    return true;
}

bool CSIOProcessor::WaitCmdHigh()
{
    while ( ! HAL::GetGPIO().GetCMDLineState() );
    return true;
}

void CSIOProcessor::SendByteAndWait(unsigned char byte)
{
    HAL::GetSIO().Write(byte);
    while(HAL::GetSIO().GetTXFIFOSize());
}
