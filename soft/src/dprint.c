#include "dprint.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "HWContext.h"

//#ifdef DEBUG
#undef dprint
__attribute__((section(".keep")))
void dprint(const char* msg, ...)
{
    // debug variant
    HWContext* ctx = (HWContext*)__hwcontext;
    if(ctx->print_buffer_len)
    {
    	va_list argptr;
    	va_start(argptr, msg);
    	vsnprintf(ctx->print_buffer, ctx->print_buffer_len, msg, argptr);    
        ctx->Debug_PutArray((const unsigned char*)&ctx->print_buffer[0], strlen((const char*)(&ctx->print_buffer[0])));

    }
} 
//#endif

