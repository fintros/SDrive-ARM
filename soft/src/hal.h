#pragma once

#include "project.h"

// no virtual inheritance here for access optimization
// but it is possible in future if will be needed
class CSIOUSART
{
public:
    static inline void Write(unsigned char byte) const 
    { 
        UART_1_WriteTxData(byte); 
    }
    static inline void WriteBuffer(unsigned char *buff, unsigned short len) const 
    { 
        UART_1_PutArray(buff, len); 
    }
    static inline unsigned char Read() const 
    {
        return UART_1_ReadRxData();
    }
    static inline unsigned char GetTXFIFOSize() const 
    {
        return UART_1_GetTxBufferSize(); 
    }
    static inline unsigned char GetRXFIFOSize() const 
    {
        return UART_1_GetRxBufferSize(); 
    }
    static inline void ClearRX() const
    {
        UART_1_ClearRxBuffer();
    }
    static inline void ClearTX() const
    {
        UART_1_ClearTxBuffer();
    }
};

class CGPIO
{
public:
    inline bool GetCMDLineState() { return Command_Read() != 0; }
}

class CSDrive2HAL
{
public:
    static inline const CSIOUSART& GetSIO() { return _sioUSART; }
    static inline const CGPIO& GetGPIO() { return _gpio; }
    static inline DelayUs(unsigned short microSeconds) 
    { 
        CyDelayUs(microSeconds); 
    } 

private:
    static CSIOUSART _sioUSART;
    static CGPIO _gpio;
};

#define HAL CSDrive2HAL
