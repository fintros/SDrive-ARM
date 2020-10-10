/*! \file fat.c \brief FAT16/32 file system driver. */
//*****************************************************************************
//
// File Name	: 'fat.c'
// Title		: FAT16/32 file system driver
// Author		: Pascal Stang / Alexey Spirkov
// Date			: 11/07/2000
// Revised		: 23/03/2017
// Version		: 1.0
// Target MCU	: ARM-Cortex-M 
// Editor Tabs	: 4
//
// This code is based in part on work done by Jesper Hansen for his
//		YAMPP MP3 player project.
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// ----------------------------------------------------------------------------
// 17.8.2008
// Bob!k & Raster, C.P.U.
// Original code was modified especially for the SDrive device. 
// Some parts of code have been added, removed, rewrited or optimized due to
// lack of MCU AVR Atmega8 memory.
// ----------------------------------------------------------------------------
// 23.03.2017
// Alexey Spirkov (AlSp).
// Original code ported to ARM, added work with global context for bootloader
// compatibility
// ----------------------------------------------------------------------------
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <stdio.h>
#include <string.h>

#include "fat.h"
#include "atari.h"
#include "mmcsd.h"
#include "dprint.h"

unsigned long fatClustToSect(HWContext* ctx, unsigned long clust)
{
    GlobalSystemValues* gsv = &ctx->gsv;
	if(clust==MSDOSFSROOT)
		return gsv->FirstDataSector - gsv->RootDirSectors;

	return ((clust-2) * gsv->SectorsPerCluster) + gsv->FirstDataSector;
}

unsigned char fatInit(HWContext* ctx)
{
    GlobalSystemValues* gsv = &ctx->gsv;
    FatData* f = &ctx->fat_data;
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
    
	partrecord PartInfo;
    partrecord *pPartInfo;
	struct bpb710 *bpb;

	// read partition table
	// TODO.... error checking
	mmcReadCached(ctx, 0);
	// map first partition record	
	// save partition information to global PartInfo
    pPartInfo = (partrecord *) (((partsector *) mmc_sector_buffer)->psPart);
	PartInfo = *pPartInfo;

	if(mmc_sector_buffer[0x36]=='F' && mmc_sector_buffer[0x37]=='A' && mmc_sector_buffer[0x38]=='T' && mmc_sector_buffer[0x39]=='1' && mmc_sector_buffer[0x3a]=='6')
	{
		PartInfo.prPartType = PART_TYPE_FAT16LBA; //0x04
		PartInfo.prStartLBA = 0x00;
	}
	
	// Read the Partition BootSector
	// **first sector of partition in PartInfo.prStartLBA
	mmcReadCached(ctx, PartInfo.prStartLBA);
	bpb = (struct bpb710 *) ((struct bootsector710 *) (char*)mmc_sector_buffer)->bsBPB;

	// setup global disk constants
	gsv->FirstDataSector	= PartInfo.prStartLBA;

	gsv->RootDirSectors = bpb->bpbRootDirEnts>>4; // /16 ; 512/16 = 32  (sector size / max entries in one sector = number of sectors)

	if(bpb->bpbFATsecs)
	{
		// bpbFATsecs is non-zero and is therefore valid
		gsv->FirstDataSector	+= bpb->bpbResSectors + bpb->bpbFATs * bpb->bpbFATsecs + gsv->RootDirSectors;
	}
	else
	{
		// bpbFATsecs is zero, real value is in bpbBigFATsecs
		gsv->FirstDataSector	+= bpb->bpbResSectors + bpb->bpbFATs * bpb->bpbBigFATsecs + gsv->RootDirSectors;
	}

	gsv->SectorsPerCluster	= bpb->bpbSecPerClust;
	gsv->BytesPerSector		= bpb->bpbBytesPerSec;
	gsv->FirstFATSector		= bpb->bpbResSectors + PartInfo.prStartLBA;

	// set current directory to root (\)
	// FileInfo.vDisk.dir_cluster = MSDOSFSROOT; //RootDirStartCluster;

    f->last_dir_start_cluster=0xffffffff;
	//last_dir_valid=0;		//<-- neni potreba, protoze na zacatku fatGetDirEntry se pri zjisteni
							//ze je (FileInfo.vDisk.dir_cluster!=last_dir_start_cluster)
							//vynuluje last_dir_valid=0;

    
	switch (PartInfo.prPartType)
	{
		case PART_TYPE_DOSFAT16:
		case PART_TYPE_FAT16:
		case PART_TYPE_FAT16LBA:
			// first directory cluster is 2 by default (clusters range 2->big)
			f->dir_cluster	= MSDOSFSROOT;
			// push data sector pointer to end of root directory area
			//FirstDataSector += (bpb->bpbRootDirEnts)/DIRENTRIES_PER_SECTOR;
			f->fat32_enabled = FALSE;
			break;
		case PART_TYPE_FAT32LBA:
		case PART_TYPE_FAT32:
			// bpbRootClust field exists in FAT32 bpb710, but not in lesser bpb's
			f ->dir_cluster = bpb->bpbRootClust;
			// push data sector pointer to end of root directory area
			// need this? FirstDataSector += (bpb->bpbRootDirEnts)/DIRENTRIES_PER_SECTOR;
			f->fat32_enabled = TRUE;
			break;
		default:
			return 0;
			break;
	}
    
    return 1;
}

//////////////////////////////////////////////////////////////

unsigned char fatGetDirEntry(HWContext* ctx, file_t* pDisk, unsigned char* file_name, unsigned short entry, unsigned char use_long_names)
{
    GlobalSystemValues* gsv = &ctx->gsv;
    FatData* f = &ctx->fat_data;
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
    
	unsigned long sector;
	struct direntry *de = 0;	// avoid compiler warning by initializing
	struct winentry *we;
	unsigned char haveLongNameEntry;
	unsigned char gotEntry;
	unsigned short b;
	unsigned char index;
	unsigned short entrycount = 0;
	unsigned long actual_cluster = pDisk->dir_cluster;
	unsigned char seccount=0;

	haveLongNameEntry = 0;
	gotEntry = 0;

	if (pDisk->dir_cluster!=f->last_dir_start_cluster)
	{
		//zmenil se adresar, takze musi pracovat s nim
		//a zneplatnit last_dir polozky
		f->last_dir_start_cluster=pDisk->dir_cluster;
		f->last_dir_valid=0;
	}

	if (!f->last_dir_valid
		 || (entry<=f->last_dir_entry && (entry!=f->last_dir_entry || f->last_dir_valid!=1 || use_long_names!=0))
	   )
	{
		//musi zacit od zacatku
		sector = fatClustToSect(ctx, pDisk->dir_cluster);
		//index = 0;
        dprint("Read from begin\r\n");
		goto fat_read_from_begin;
	}
	else
	{
		//muze zacit od posledne pouzite
		entrycount=f->last_dir_entry;
		index = f->last_dir_index;
		sector=f->last_dir_sector;
		actual_cluster=f->last_dir_cluster;
		seccount = f->last_dir_sector_count;
		goto fat_read_from_last_entry;
	}

	while(1)
	{
		if(index==16)	// time for next sector ?
		{
			//if ( actual_cluster==MSDOSFSROOT )
			//{
				//v MSDOSFSROOT se nerozlisuji clustery
				//protoze MSDOSFSROOT ma sektory stale dal za sebou bez clusterovani
			//	if (seccount>=RootDirSectors) return 0; //prekrocil maximalni pocet polozek v MSDOSFSROOT
			//}
			//else //MUSI BYT!!! (aby neporovnaval seccount je-li actual_cluster==MSDOSFSROOT)
			if( seccount>=gsv->SectorsPerCluster )
			{
				//next cluster
				//pri prechodu pres pocet sektoru v clusteru
				actual_cluster = nextCluster(ctx, actual_cluster);
				sector=fatClustToSect(ctx, actual_cluster);
				seccount=0;
			}
fat_read_from_begin:
			index = 0;
			seccount++;		//ted bude nacitat dalsi sektor (musi ho zapocitat)
fat_read_from_last_entry:
			mmcReadCached( ctx, sector++ ); 	//prave ho nacetl
			de = (struct direntry *) (char*)mmc_sector_buffer;
			de+=index;
		}
		
		// check the status of this directory entry slot
		if(de->deName[0] == 0x00) //SLOT_EMPTY
		{
			// slot is empty and this is the end of directory
			gotEntry = 0;
			break;
		}
		else if(de->deName[0] == 0xE5)	//SLOT_DELETED
		{
			// this is an empty slot
			// do nothing and move to the next one
		}
		else
		{
			// this is a valid and occupied entry
			// is it a part of a long file/dir name?
			if( de->deAttributes==ATTR_LONG_FILENAME )
			{
				// we have a long name entry
				// cast this directory entry as a "windows" (LFN: LongFileName) entry
				unsigned char i;
				unsigned char *fnbPtr;

				if (!use_long_names) goto fat_next_dir_entry;

				we = (struct winentry *) de;
				
				b = WIN_ENTRY_CHARS*( (unsigned short)((we->weCnt-1) & 0x0f));		// index into string
				fnbPtr = &file_name[b];

				for (i=0;i<5;i++)	*fnbPtr++ = we->wePart1[i*2];	// copy first part
				for (i=0;i<6;i++)	*fnbPtr++ = we->wePart2[i*2];	// second part
				for (i=0;i<2;i++)	*fnbPtr++ = we->wePart3[i*2];	// and third part


				if (we->weCnt & WIN_LAST) *fnbPtr = 0;				// in case dirnamelength is multiple of 13, add termination
				if ((we->weCnt & 0x0f) == 1) haveLongNameEntry = 1;	// flag that we have a complete long name entry set
			}
			else
			{
				// we have a short name entry
				
				// check if this is the short name entry corresponding
				// to the end of a multi-part long name entry
				if(haveLongNameEntry)
				{
					// a long entry name has been collected
					if(entrycount == entry)		
					{
						// desired entry has been found, break out
						gotEntry = 2;
						break;
					}
					// otherwise
					haveLongNameEntry = 0;	// clear long name flag
					entrycount++;			// increment entry counter		
				}
				else
				{
					// entry is a short name (8.3 format) without a
					// corresponding multi-part long name entry

					if( (de->deName[0]=='.' && de->deName[1]==' ' && (de->deAttributes & ATTR_DIRECTORY) ) //je to "." adresar
						|| (de->deAttributes & ATTR_VOLUME) ) goto fat_next_dir_entry;


                    if(entrycount == entry)
					{                      
                    	// use 'NAME    EXT' or 'NAME.EXT' depend on long names flag
                        memcpy(&file_name[0], &de->deName[0], 8);
                        int ext_pos = 8;
                        if (use_long_names)
                        {
                            int i =0;
                            for(;i<8;i++)
                                if(file_name[i] == ' ')
                                    break;
                            if(file_name[0] != '.') // . and .. are exceptions
                                file_name[i] = '.';
                            else
                                file_name[i] = 0;
                            ext_pos = i+1;
                        }
                        memcpy(&file_name[ext_pos], &de->deExtension[0], 3);
                        file_name[ext_pos+3] = 0;

						gotEntry = 1;
						break;
					}
					// otherwise
					entrycount++;			// increment entry counter		
				}
			}
		}
fat_next_dir_entry:
		// next directory entry
		de++;
		// next index
		index++;
	}
	
	// we have a file/dir to return
	// store file/dir starting cluster (start of data)
	pDisk->start_cluster = (unsigned long) ((unsigned long)de->deHighClust << 16) +  (de->deStartCluster);
	// fileindex
	pDisk->file_index = entry;	//fileindex teto polozky
	// store file/dir size (note: size field for subdirectory entries is always zero)
	pDisk->size = de->deFileSize;
	// store file/dir attributes
	pDisk->fi.Attr = de->deAttributes;
	// store file/dir last update time
	pDisk->fi.Time = (unsigned short) (de->deMTime[0] | de->deMTime[1]<<8);
	// store file/dir last update date
	pDisk->fi.Date = (unsigned short) (de->deMDate[0] | de->deMDate[1]<<8);

	if(gotEntry)
	{
		f->last_dir_entry = entrycount;
		f->last_dir_index = index;
		f->last_dir_sector = sector-1; //protoze ihned po nacteni se posune
		f->last_dir_cluster = actual_cluster;
		f->last_dir_sector_count = seccount; //skace se az za inkrementaci seccount, takze tady se 1 neodecita!
		f->last_dir_valid=gotEntry;
        
        //dprint("DE %s: ec:%d, ind: %d, sec: %d, ac: %d, sc: %d\r\n",file_name_buffer, entrycount, index, f->last_dir_sector, f->last_dir_cluster, f->last_dir_sector_count);
	}
    else
    {
        dprint("No entry\r\n");
    }

	return gotEntry;
}


unsigned long nextCluster(HWContext* ctx, unsigned long cluster)
{
    GlobalSystemValues* gsv = &ctx->gsv;
    FatData* f = &ctx->fat_data;
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
   
    
    
	  unsigned long nextCluster;
      unsigned long fatMask;
	  unsigned long fatOffset;
	  unsigned long sector;
	  unsigned long offset;
      unsigned long tmp;

    	// get fat offset in bytes
    	if(f->fat32_enabled)
    	{
    		// four FAT bytes (32 bits) for every cluster
    		fatOffset = cluster << 2;
    		// set the FAT bit mask
    		fatMask = FAT32_MASK;
    	}
    	else
    	{
    		// two FAT bytes (16 bits) for every cluster
    		fatOffset = cluster << 1;
    		// set the FAT bit mask
    		fatMask = FAT16_MASK;
    	}

	  // calculate the FAT sector that we're interested in
	  sector = gsv->FirstFATSector + (fatOffset / ((unsigned long)gsv->BytesPerSector));
	  // calculate offset of the our entry within that FAT sector
	  offset = fatOffset % ((unsigned long)gsv->BytesPerSector);

	  // if we don't already have this FAT chunk loaded, go get it
	  // read sector of FAT table
  	  mmcReadCached( ctx, sector );

      memcpy(&tmp, &((char*)mmc_sector_buffer)[offset], sizeof(tmp));
	  // read the nextCluster value
	  nextCluster = tmp & fatMask;

	  // check to see if we're at the end of the chain
	  if (nextCluster == (CLUST_EOFE & fatMask))
		nextCluster = 0;

	return nextCluster;
}


unsigned long getClusterN(HWContext* ctx, file_t *pDisk, unsigned short ncluster)
{
    FatData* f = &ctx->fat_data;
    
	if(ncluster<pDisk->ncluster)
	{
		pDisk->current_cluster=pDisk->start_cluster;
		pDisk->ncluster=0;
	}

	while(pDisk->ncluster!=ncluster)
	{
		pDisk->current_cluster=nextCluster(ctx, pDisk->current_cluster);
		pDisk->ncluster++;
	}

	return (pDisk->current_cluster&(f->fat32_enabled?0xFFFFFFFF:0xFFFF));
}

unsigned short faccess_offset(HWContext* ctx, file_t* pDisk, char mode, unsigned long offset_start, unsigned char* buffer, unsigned short ncount)
{
    GlobalSystemValues* gsv = &ctx->gsv;
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
        
	unsigned short j;
	unsigned long offset=offset_start;
	unsigned long end_of_file;
	unsigned long ncluster, nsector, current_sector;
	unsigned long bytespercluster=((unsigned long)gsv->SectorsPerCluster)*((unsigned long)gsv->BytesPerSector);

	if(offset_start>=pDisk->size)
		return 0;

	end_of_file = (pDisk->size - offset_start);

	ncluster = offset/bytespercluster;
	offset-=ncluster*bytespercluster;

	nsector = offset/((unsigned long)gsv->BytesPerSector);
	offset-=nsector*((unsigned long)gsv->BytesPerSector);

	getClusterN(ctx, pDisk, ncluster);

	current_sector = fatClustToSect(ctx, pDisk->current_cluster) + nsector;
	mmcReadCached(ctx, current_sector);

	for(j=0;j<ncount;j++,offset++)
	{
			if(offset>=((unsigned long)gsv->BytesPerSector) )
			{
				if(mode==FILE_ACCESS_WRITE && mmcWriteCached(ctx, 0))
					return 0;

				offset=0;
				nsector++;

				if(nsector>=((unsigned long)gsv->SectorsPerCluster) )
				{
					nsector=0;
					ncluster++;
					getClusterN(ctx, pDisk, ncluster);
				}

				current_sector = fatClustToSect(ctx, pDisk->current_cluster) + nsector;
				mmcReadCached(ctx, current_sector);
			}

		if(!end_of_file)
			break; //return (j&0xFFFF);

		if(mode==FILE_ACCESS_WRITE)
			mmc_sector_buffer[offset]=buffer[j]; //SDsektor<-atarisektor
		else
			buffer[j]=mmc_sector_buffer[offset]; //atarisektor<-SDsektor

		end_of_file--;

	}
	

	if(mode==FILE_ACCESS_WRITE && mmcWriteCached(ctx, 0))
		return 0;

	return (j&0xFFFF);
}
