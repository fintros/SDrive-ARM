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
#include "helpers.h"

#define UART_SAMPLES 40
static unsigned char _captured_vals[UART_SAMPLES];
unsigned int _captured_no;

// RX edge changing handler
// Timer conts 4x speed to solve racing issues
CY_ISR(SlowUartISR)
{
    if (_captured_no < UART_SAMPLES)
    {
        unsigned int count = SlowUARTCounter_COUNTER_REG & SlowUARTCounter_16BIT_MASK;
        SlowUARTCounter_COUNTER_REG = 0;
        _captured_vals[_captured_no++] = (count << 1) | (SWBUT_Read() & 1);
    }
}

enum
{
    EStateWaitForStartBit = 0,
    EStateDataRecv,
    EStateWaitForStopBit,
    EStateError
};

void ResetSlowUART()
{
    _captured_no = 0;
    SlowUARTCounter_WriteCounter(0);
    SlowUARTCounter_Start();
    SlowUartISR_StartEx(SlowUartISR);
}

void StopSlowUART()
{
    SlowUartISR_Stop();
    SlowUARTCounter_Stop();
}

// array of captued values decoded to bytes sequence. Each value of _captured_vals is
// length of input level in timer ticks shifted by 1 to the left and last bit - level itself 0 or 1
int DecodeSlowUART(unsigned char *command, int max_len)
{
    int state = EStateWaitForStartBit;
    int byte_no = 0;
    int bit_in_byte = 0;
    unsigned char current_byte = 0;

    memset(command, 0x00, max_len);

#ifdef DEBUG
    dprint("Catched: ");
    for (unsigned int i = 0; i < _captured_no; i++)
        dprint("[%d]%d,", 1 - (_captured_vals[i] & 1), _captured_vals[i] >> 1);
    dprint("\r\n");
#endif

    for (unsigned int i = 0; i < _captured_no; i++)
    {
        unsigned int bit = 1 - (_captured_vals[i] & 1);

        // fix racing errors
        unsigned int realcount = _captured_vals[i] >> 1;
        unsigned int count = realcount >> 2;
        unsigned int rest = realcount - (count << 2);
        if (rest > 2)
            count++;

        while (count--)
        {
            switch (state)
            {
            case EStateWaitForStartBit:
                if (bit) // wait for '0'
                    break;
                state = EStateDataRecv;
                current_byte = 0;
                bit_in_byte = 0;
                break;
            case EStateDataRecv:
                current_byte |= bit << bit_in_byte++;
                if (bit_in_byte == 8)
                    state = EStateWaitForStopBit;
                break;
            case EStateWaitForStopBit:
                if (bit) // wait for '1'
                {
                    if (byte_no < max_len)
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
    if (bit_in_byte < 8)
    {
        for (; bit_in_byte < 8; bit_in_byte++)
            current_byte |= 1 << bit_in_byte;
    }
    if (byte_no < max_len)
        command[byte_no++] = current_byte;

    unsigned char checksum = get_checksum(command, 4);

#ifdef DEBUG
    dprint("Decoded CMD %d: [%02X %02X %02X %02X %02X==%02X]\r\n", byte_no, command[0], command[1], command[2], command[3], command[4], checksum);
#endif

    if (command[4] != checksum)
        return 1;

    return 0;
}