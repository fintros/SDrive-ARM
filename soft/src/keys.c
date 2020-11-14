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
#include "keys.h"

void init_keys()
{
    CapSense_Start();
    CapSense_InitializeAllBaselines();
    CapSense_ScanAllWidgets();
}

unsigned char get_actual_key()
{
    static unsigned char actual_key = 0;

    if (CapSense_NOT_BUSY == CapSense_IsBusy())
    {
        CapSense_ProcessAllWidgets();
        if (CapSense_IsAnyWidgetActive())
        {
            if (CapSense_IsWidgetActive(CapSense_BUTTON0_WDGT_ID))
                actual_key = 6;
            else if (CapSense_IsWidgetActive(CapSense_BUTTON1_WDGT_ID))
                actual_key = 5;
            else if (CapSense_IsWidgetActive(CapSense_BUTTON2_WDGT_ID))
                actual_key = 3;
        }
        else
        {
            actual_key = 0;
        }
        CapSense_ScanAllWidgets();
    }
    return actual_key;
}
