
#include "mmcsd.h"
#include "SDAccess.h"

static SDAccess _sdAccess;

extern "C" {    

extern u32 n_actual_mmc_sector;
extern u08 n_actual_mmc_sector_needswrite;

void mmcInit(void)
{
	// initialize SPI interface
}

u08 mmcReset(void)
{
	n_actual_mmc_sector=0xFFFFFFFF;
	n_actual_mmc_sector_needswrite=0;
    
    return _sdAccess.disk_initialize();
}

extern unsigned char mmc_sector_buffer[512];

u08 mmcRead(u32 sector)
{
	return _sdAccess.disk_read(&mmc_sector_buffer[0], sector, 1);
}

u08 mmcWrite(u32 sector)
{
	return _sdAccess.disk_write(&mmc_sector_buffer[0], sector, 1);
}

}    
