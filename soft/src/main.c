/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
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
