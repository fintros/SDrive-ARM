/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _GPIO_H
#define _GPIO_H

#include "project.h"
#include <CyLib.h>
#include "hwcontext.h"

#define CMD_STATE_H 1
#define CMD_STATE_L 0
#define get_cmd_H() (Command_Read())
#define get_cmd_L() (!(Command_Read()))
#define wait_cmd_HL()          \
    {                          \
        while (Command_Read()) \
            ;                  \
    }
#define wait_cmd_LH()           \
    {                           \
        while (!Command_Read()) \
            ;                   \
    }

#define get_readonly() (ReadOnly_B_Read() == 0)

#define LED_ON
#define LED_OFF

// Activity
#define LED_GREEN_ON ((HWContext *)__hwcontext)->LEDREG_Write(((HWContext *)__hwcontext)->LEDREG_Read() & ~0x1)
#define LED_GREEN_OFF ((HWContext *)__hwcontext)->LEDREG_Write(((HWContext *)__hwcontext)->LEDREG_Read() | 0x1)

// Writing
#define LED_RED_ON ((HWContext *)__hwcontext)->LEDREG_Write(((HWContext *)__hwcontext)->LEDREG_Read() & ~0x2)
#define LED_RED_OFF ((HWContext *)__hwcontext)->LEDREG_Write(((HWContext *)__hwcontext)->LEDREG_Read() | 0x2)

#endif /* _GPIO_H */