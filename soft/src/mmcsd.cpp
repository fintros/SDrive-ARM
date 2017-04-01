
#include "mmcsd.h"
#include "SDAccess.h"
#include "HWContext.h"


extern "C" {    

void mmcInit(void)
{
    HWContext* ctx = (HWContext*)__hwcontext;
  	ctx->n_actual_mmc_sector=0xFFFFFFFF;
	ctx->n_actual_mmc_sector_needswrite=0;

    // initialize SPI interface
}

u08 mmcReset(void)
{
    HWContext* ctx = (HWContext*)__hwcontext;
  
    return ((SDAccess*)(ctx->sdAccess))->disk_initialize();
}

u08 mmcRead(u32 sector)
{
    HWContext* ctx = (HWContext*)__hwcontext;
	return ((SDAccess*)(ctx->sdAccess))->disk_read(&ctx->mmc_sector_buffer[0], sector, 1);
}

u08 mmcWrite(u32 sector)
{
    HWContext* ctx = (HWContext*)__hwcontext;
	return ((SDAccess*)(ctx->sdAccess))->disk_write(&ctx->mmc_sector_buffer[0], sector, 1);
}

void mmcReadCached(u32 sector)
{
    HWContext* ctx = (HWContext*)__hwcontext;
	if(sector==ctx->n_actual_mmc_sector) return;

	u08 ret,retry;
	//predtim nez nacte jiny, musi ulozit soucasny
	mmcWriteCachedFlush();
	//az ted nacte novy
	retry=0; //zkusi to maximalne 256x
	do
	{
		ret = mmcRead(sector);	//vraci 0 kdyz ok
		retry--;
	} while (ret && retry);
	while(ret); //a pokud se vubec nepovedlo, tady zustane zablokovany cely SDrive!
	ctx->n_actual_mmc_sector=sector;
}

u08 mmcWriteCached(unsigned char force)
{
    HWContext* ctx = (HWContext*)__hwcontext;
	//if ( get_readonly() ) return 0xff; //zakazany zapis
	//LED_RED_ON;
	if (force)
	{
		u08 ret,retry;
		retry=16; //zkusi to maximalne 16x
		do
		{
			ret = mmcWrite(ctx->n_actual_mmc_sector); //vraci 0 kdyz ok
			retry--;
		} while (ret && retry);
		while(ret); //a pokud se vubec nepovedlo, tady zustane zablokovany cely SDrive!
		ctx->n_actual_mmc_sector_needswrite = 0;
		//LED_RED_OFF;
	}
	else
	{
		ctx->n_actual_mmc_sector_needswrite=1;
	}
	return 0; //vraci 0 kdyz ok
}

void mmcWriteCachedFlush()
{
    HWContext* ctx = (HWContext*)__hwcontext;
	if (ctx->n_actual_mmc_sector_needswrite)
	{
	 while (mmcWriteCached(1)); //pokud je zakazany zapis, tady zustane zablokovany cely SDrive!
	 							//do okamziku, nez ten zapis nebude povolen (visi mu neco v cache)
	}
}

}    
