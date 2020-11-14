/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "mmcsd.h"
#include "sdaccess.h"

#define LED_RED_ON (ctx->LEDREG_Write(ctx->LEDREG_Read() & ~0x2))
#define LED_RED_OFF (ctx->LEDREG_Write(ctx->LEDREG_Read() | 0x2))

extern "C"
{

	void mmcInit(HWContext *ctx)
	{
		ctx->n_actual_mmc_sector = 0xFFFFFFFF;
		ctx->n_actual_mmc_sector_needswrite = 0;
	}

	int mmcReset(HWContext *ctx)
	{
		return ((SDAccess *)(ctx->sdAccess))->disk_initialize();
	}

	int mmcRead(HWContext *ctx, unsigned long sector)
	{
		return ((SDAccess *)(ctx->sdAccess))->disk_read(&ctx->mmc_sector_buffer[0], sector, 1);
	}

	int mmcWrite(HWContext *ctx, unsigned long sector)
	{
		return ((SDAccess *)(ctx->sdAccess))->disk_write(&ctx->mmc_sector_buffer[0], sector, 1);
	}

	void mmcReadCached(HWContext *ctx, unsigned long sector)
	{
		if (sector == ctx->n_actual_mmc_sector)
			return;

		unsigned char ret, retry;
		mmcWriteCachedFlush(ctx);
		retry = 0;
		do
		{
			ret = mmcRead(ctx, sector);
			retry--;
		} while (ret && retry);
		while (ret)
			;
		ctx->n_actual_mmc_sector = sector;
	}

	int mmcWriteCached(HWContext *ctx, unsigned char force)
	{
		if (ctx->ReadOnly_Read() == 0)
			return 0xff; //zakazany zapis
		LED_RED_ON;
		if (force)
		{
			unsigned char ret, retry;
			retry = 16;
			do
			{
				ret = mmcWrite(ctx, ctx->n_actual_mmc_sector);
				retry--;
			} while (ret && retry);
			while (ret)
				;
			ctx->n_actual_mmc_sector_needswrite = 0;
			LED_RED_OFF;
		}
		else
		{
			ctx->n_actual_mmc_sector_needswrite = 1;
		}
		return 0;
	}

	void mmcWriteCachedFlush(HWContext *ctx)
	{
		if (ctx->n_actual_mmc_sector_needswrite)
			while (mmcWriteCached(ctx, 1))
				;
	}
}
