/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
#ifndef _SIOPROCESSOR_H
#define _SIOPROCESSOR_H

#include "HWContext.h"

#ifndef DEBUG_SIO
//#undef DEBUG_SIO
#define DEBUG_SIO 1
#endif

#define US_POKEY_DIV_STANDARD	0x28		//#40  => 19040 bps
#define ATARI_SPEED_STANDARD	(US_POKEY_DIV_STANDARD+6)	//#46 (o sest vic)
#define US_POKEY_DIV_DEFAULT	0x06		//#6   => 68838 bps
#define US_POKEY_DIV_MAX		(255-6)		//pokeydiv 249 => avrspeed 255 (vic nemuze)

struct SIOCommand
{
    unsigned char device;
    unsigned char command;
    unsigned char aux1;
    unsigned char aux2;
};

class CSIOProcessor
{
public:
    CSIOProcessor(HWContext& context);
    ~CSIOProcessor();

    void Send(unsigned char* ptr, int len);
    
    bool GetCommand(SIOCommand& command);
    bool Recive(unsigned char *buff, int len);

    bool ToggleSpeed();         // return fastsio status
    bool SetFastIO(int pokeyDiv);
    int GetFastIO() {return _hiSpeedDiv; }
 
 private:
    void Init(int pokeyDiv);
    bool RecvBufferWithState(unsigned char *buff, int len, bool cmd_state = true);
    void SendACK();
    void SendNACK();
    void SendCMPL();
    void SendERR();
    bool WaitCmdLow();
    bool WaitCmdHigh();
    
    HWContext& _context;
    int _hiSpeedDiv;
    bool _hiSpeedEnabled;
};


#endif