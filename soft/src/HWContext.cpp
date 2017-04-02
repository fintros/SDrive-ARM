extern "C"
{   
#include "project.h"
}

#include "HWContext.h"

#include "SDAccess.h"

static struct
{
    HWContext _context;
    SDAccess _sdAccess;
#ifdef DEBUG
    char print_buffer[255];
#endif
} _staticVars;

extern "C" 
{
#ifdef LOADER
void InitLoaderCTX(void)
#else
void InitLoadableCTX(void)
#endif
{
    memset(&_staticVars._context, 0, sizeof(HWContext));
    
    __hwcontext = &_staticVars._context;    

 #ifdef LOADER   
    _staticVars._context.SPI_CLK_SetFractionalDividerRegister = SPI_CLK_BL_SetFractionalDividerRegister;
    _staticVars._context.SPIM_Start = SPIM_BL_Start;
    _staticVars._context.SPIM_WriteTxData = SPIM_BL_SpiUartWriteTxData;
    _staticVars._context.SPIM_GetRxBufferSize = SPIM_BL_SpiUartGetRxBufferSize;
    _staticVars._context.SPIM_ReadRxData = SPIM_BL_SpiUartReadRxData;
    
#ifdef DEBUG
    Debug_BL_Start();
    _staticVars._context.Debug_PutArray = Debug_BL_SpiUartPutArray;
    _staticVars._context.print_buffer = &_staticVars.print_buffer[0];
    _staticVars._context.print_buffer_len = sizeof(_staticVars.print_buffer);
#endif
    
    _staticVars._context.LEDREG_Read = LEDREG_BL_Read;
    _staticVars._context.LEDREG_Write = LEDREG_BL_Write;
#else
    _staticVars._context.SPI_CLK_SetFractionalDividerRegister = SPI_CLK_B_SetFractionalDividerRegister;
    _staticVars._context.SPIM_Start = SPIM_B_Start;
    _staticVars._context.SPIM_WriteTxData = SPIM_B_SpiUartWriteTxData;
    _staticVars._context.SPIM_GetRxBufferSize = SPIM_B_SpiUartGetRxBufferSize;
    _staticVars._context.SPIM_ReadRxData = SPIM_B_SpiUartReadRxData;
    
#ifdef DEBUG
    Debug_B_Start();
    _staticVars._context.Debug_PutArray = Debug_B_SpiUartPutArray;
    _staticVars._context.print_buffer = &_staticVars.print_buffer[0];
    _staticVars._context.print_buffer_len = sizeof(_staticVars.print_buffer);
    
#endif
    
    _staticVars._context.LEDREG_Read = LEDREG_B_Read;
    _staticVars._context.LEDREG_Write = LEDREG_B_Write;
#endif

    _staticVars._context.Delay = CyDelay;

    _staticVars._context.sdAccess = &_staticVars._sdAccess;

    _staticVars._context._StaticVarsSize = sizeof (_staticVars);

}
}