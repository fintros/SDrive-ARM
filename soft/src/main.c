/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "project.h"
#include "HWContext.h"

int sdrive(void);

int main(void)
{
    
    //CyDelay(5000);
    CyGlobalIntEnable; /* Enable global interrupts. */
    UART_1_Start();    

    InitLoadableCTX();
    sdrive();
    return 0;
}

/* [] END OF FILE */
