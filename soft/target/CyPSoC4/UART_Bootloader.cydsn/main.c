/*  --------------------------------------------------------------------------
* Copyright 2016, Cypress Semiconductor Corporation.
*
* This software is owned by Cypress Semiconductor Corporation (Cypress)
* and is protected by and subject to worldwide patent protection (United
* States and foreign), United States copyright laws and international
* treaty provisions. Cypress hereby grants to licensee a personal,
* non-exclusive, non-transferable license to copy, use, modify, create
* derivative works of, and compile the Cypress Source Code and derivative
* works for the sole purpose of creating custom software in support of
* licensee product to be used only in conjunction with a Cypress integrated
* circuit as specified in the applicable agreement. Any reproduction,
* modification, translation, compilation, or representation of this
* software except as specified above is prohibited without the express
* written permission of Cypress.
* 
* Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND,EXPRESS OR IMPLIED,
* WITH REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* Cypress reserves the right to make changes without further notice to the
* materials described herein. Cypress does not assume any liability arising
* out of the application or use of any product or circuit described herein.
* Cypress does not authorize its products for use as critical components in
* life-support systems where a malfunction or failure may reasonably be
* expected to result in significant injury to the user. The inclusion of
* Cypress' product in a life-support systems application implies that the
* manufacturer assumes all risk of such use and in doing so indemnifies
* Cypress against all charges.
* 
* Use may be limited by and subject to the applicable Cypress software
* license agreement.
* -----------------------------------------------------------------------------
* Copyright (c) Cypress Semiconductors 2000-2016. All Rights Reserved.
*
*****************************************************************************
*  Project Name: UART_Bootloader
*  Project Revision: 1.00
*  Software Version: PSoC Creator 3.3 SP2
*  Device Tested: CY8C4245AXI-483
*  Compilers Tested: ARM GCC
*  Related Hardware: CY8CKIT-042
*****************************************************************************
***************************************************************************** */

/*
* Project Description:
* This is a sample bootloader program demonstrating PSoC 5LP bootloading PSoC 4.
* The project is tested using two DVKs - One with PSoC 4 on CY8CKIT-042 and 
* the second one with PSoC 5LP on CY8CKIT-050. PSoC 4 must be programmed with
* this Program.
*
* Connections Required
* CY8CKIT-050 (PSoC 5LP DVK) :
*  P0[0] - Rx -  connected to Tx of PSoC4
*  P0[1] - Tx -  connected to Rx of PSoC4 
*  P6.1 is internally connected to SW1 on DVK.
*
* CY8CKIT-042 (PSoC 4 DVK) : PSoC  4 intially programmed with UART_Bootloader program.
*  P0[0] - Rx - Connected to Tx of PSoC 5LP
*  P0[1] - Tx - Connected to Rx of PSoC 5LP
*  P0.7 is internally connected to SW1 on DVK.
*
* Note that the GNDs of both DVKs should be connected together.
*
* The main()function of this program calls the CyBtldr_Start() API. This API
* performs the entire bootload operation. It communicates with the host. 
* After a successful bootload operation, this API passes control to the application
* via a software reset.
*
* There are two bootloadable projects: Bootloadable1 (blinks the Green LED) and 
* Bootloadable2 (blinks the Blue LED). With successive bootload operation, 
* alternate bootloadables will be programmed onto the target. 
*
* The following events happen on each trigger of the switch P6 [1] of 
* CY8CKIT – 050 (PSoC 5LP):
*
* On first switch press, Bootloadable1.cyacd file will be bootloaded onto the 
* target PSoC 4. On successful bootloading the message “Bootloaded - Green” 
* will be displayed on CY8CKIT – 050 LCD and the Green LED starts blinking 
* on the CY8CKIT – 042.
*
* For subsequent bootloading operation press the switch P0[7] on CY8CKIT – 042.
* This makes PSoC 4 to enter bootloader and be ready to bootload a new application.
*
* On next switch press on CY8CKIT – 050, Bootloadable2.cyacd file will be 
* bootloaded onto the target PSoC 4. On successful bootloading the message 
* “Bootloaded - Blue” will be displayed on CY8CKIT – 050 LCD and the Blue LED 
* starts blinking on the CY8CKIT – 042.
***************************************************************************** */

#include <device.h>

int main()
{
   	/* Initialize PWM */
	PWM_Start();
	
	/* This API does the entire bootload operation. After a succesful 
	   bootload operation, this API transfers program control to the 
	   new application via a software reset */
      Bootloader_Start();

	/* CyBtldr_Start() API does not return – it ends with a 
	   software device reset. So, the code after this API call (below)
	   is never executed. */
    for(;;)
    {
        
    }
}

/* [] END OF FILE */
