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
#include "HWContext.h"
#include "mmcsd.h"
#include "dprint.h"

unsigned long fatClustToSect(unsigned long clust)
{
    GlobalSystemValues* gsv = &((HWContext*)__hwcontext)->gsv;
	if(clust==MSDOSFSROOT)
		return gsv->FirstDataSector - gsv->RootDirSectors;

	return ((clust-2) * gsv->SectorsPerCluster) + gsv->FirstDataSector;
}

unsigned char fatInit()
{
    GlobalSystemValues* gsv = &((HWContext*)__hwcontext)->gsv;
    FatData* f = &((HWContext*)__hwcontext)->fat_data;
    unsigned char* mmc_sector_buffer = &((HWContext*)__hwcontext)->mmc_sector_buffer[0];
    FileInfoStruct* fi = &((HWContext*)__hwcontext)->file_info;
    
	partrecord PartInfo;
    partrecord *pPartInfo;
	struct bpb710 *bpb;

	// read partition table
	// TODO.... error checking
	mmcReadCached(0);
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
	mmcReadCached(PartInfo.prStartLBA);
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
			fi->vDisk.dir_cluster	= MSDOSFSROOT;
			// push data sector pointer to end of root directory area
			//FirstDataSector += (bpb->bpbRootDirEnts)/DIRENTRIES_PER_SECTOR;
			f->fat32_enabled = FALSE;
			break;
		case PART_TYPE_FAT32LBA:
		case PART_TYPE_FAT32:
			// bpbRootClust field exists in FAT32 bpb710, but not in lesser bpb's
			fi->vDisk.dir_cluster = bpb->bpbRootClust;
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

unsigned char fatGetDirEntry(unsigned short entry, unsigned char use_long_names)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    GlobalSystemValues* gsv = &ctx->gsv;
    FatData* f = &ctx->fat_data;
    unsigned char* file_name_buffer = &ctx->atari_sector_buffer[0];
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
    FileInfoStruct* fi = &ctx->file_info;
    
	unsigned long sector;
	struct direntry *de = 0;	// avoid compiler warning by initializing
	struct winentry *we;
	unsigned char haveLongNameEntry;
	unsigned char gotEntry;
	unsigned short b;
	u08 index;
	unsigned short entrycount = 0;
	unsigned long actual_cluster = fi->vDisk.dir_cluster;
	unsigned char seccount=0;

	haveLongNameEntry = 0;
	gotEntry = 0;

	if (fi->vDisk.dir_cluster!=f->last_dir_start_cluster)
	{
		//zmenil se adresar, takze musi pracovat s nim
		//a zneplatnit last_dir polozky
		f->last_dir_start_cluster=fi->vDisk.dir_cluster;
		f->last_dir_valid=0;
	}

	if (!f->last_dir_valid
		 || (entry<=f->last_dir_entry && (entry!=f->last_dir_entry || f->last_dir_valid!=1 || use_long_names!=0))
	   )
	{
		//musi zacit od zacatku
		sector = fatClustToSect(fi->vDisk.dir_cluster);
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
				actual_cluster = nextCluster(actual_cluster);
				sector=fatClustToSect(actual_cluster);
				seccount=0;
			}
fat_read_from_begin:
			index = 0;
			seccount++;		//ted bude nacitat dalsi sektor (musi ho zapocitat)
fat_read_from_last_entry:
			mmcReadCached( sector++ ); 	//prave ho nacetl
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
				u08 i;
				unsigned char *fnbPtr;

				//pokud nechceme longname, NESMI je vubec kompletovat
				//a preskoci na dalsi polozku
				//( takze ani haveLongNameEntry nikdy nebude nastaveno na 1)
				//Jinak by totiz preplacaval dalsi kusy atari_sector_bufferu 12-255 ! BACHA!!!
				if (!use_long_names) goto fat_next_dir_entry;

				we = (struct winentry *) de;
				
				b = WIN_ENTRY_CHARS*( (unsigned short)((we->weCnt-1) & 0x0f));		// index into string
				fnbPtr = &file_name_buffer[b];

				for (i=0;i<5;i++)	*fnbPtr++ = we->wePart1[i*2];	// copy first part
				for (i=0;i<6;i++)	*fnbPtr++ = we->wePart2[i*2];	// second part
				for (i=0;i<2;i++)	*fnbPtr++ = we->wePart3[i*2];	// and third part

				/*
				{
				//unsigned char * sptr;
				//sptr=we->wePart1;
				//do { *fnbPtr++ = *sptr++; sptr++; } while (sptr<(we->wePart1+10)); // copy first part
				//^-- pouziti jen tohoto prvniho a druhe dva pres normalni for(..) uspori 10 bajtu,
				//pri pouziti i dalsich dvou timto stylem uz se to ale zase prodlouzi - nechaaaaaapu!
				//sptr=we->wePart2;
				//do { *fnbPtr++ = *sptr++; sptr++; } while (sptr<(we->wePart2+12)); // second part
				//sptr=we->wePart3;
				//do { *fnbPtr++ = *sptr++; sptr++; } while (sptr<(we->wePart3+4));  // and third part
				}
				*/

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

					//Zcela vynecha adresar "." a disk label
					if( (de->deName[0]=='.' && de->deName[1]==' ' && (de->deAttributes & ATTR_DIRECTORY) ) //je to "." adresar
						|| (de->deAttributes & ATTR_VOLUME) ) goto fat_next_dir_entry;

					if(entrycount == entry)
					{
						/*
						fnbPtr = &FileNameBuffer[string_offset];
						for (i=0;i<8;i++)	*fnbPtr++ = de->deName[i];		// copy name
						for (i=0;i<3;i++)	*fnbPtr++ = de->deExtension[i];	// copy extension
						*fnbPtr = 0;										// null-terminate
						*/
						u08 i;
						unsigned char *dptr;

						dptr = &file_name_buffer[0];
						for (i=0;i<8;i++)	*dptr++ = de->deName[i];		// copy name+ext
						for (i=0;i<3;i++)	*dptr++ = de->deExtension[i];	// copy name+ext
						*dptr=0;	//ukonceni za nazvem

						// desired entry has been found, break out
						//pokud chtel longname, tak to neni a proto upravi 8+3 jmeno na nazev.ext
						//aby to melo formatovani jako longname
						if (use_long_names)
						{
							//upravi 'NAME    EXT' na 'NAME.EXT'
							//(vyhazi mezery a prida tecku)
							//krome nazvu '.          ', a '..         ', tam jen vyhaze mezery
							unsigned char *sptr;
							//EXT => .EXT   (posune vcetne 0x00 za koncem extenderu)
							dptr=(file_name_buffer+12);
							i=4; do { sptr=dptr-1; *dptr=*sptr; dptr=sptr; i--; } while(i>0);
							if (file_name_buffer[0]!='.') *dptr='.'; //jen pro jine nez '.' a '..'
							//NAME    .EXT => NAME.EXT
							sptr=dptr=&file_name_buffer[0];
							do
							{
							 if ((*sptr)!=' ') *dptr++=*sptr;
							 sptr++;
							} while (*sptr);
							*dptr=0; //ukonceni
						}
						//
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
	fi->vDisk.start_cluster = (unsigned long) ((unsigned long)de->deHighClust << 16) +  (de->deStartCluster);
	// fileindex
	fi->vDisk.file_index = entry;	//fileindex teto polozky
	// store file/dir size (note: size field for subdirectory entries is always zero)
	fi->vDisk.size = de->deFileSize;
	// store file/dir attributes
	fi->Attr = de->deAttributes;
	// store file/dir last update time
	fi->Time = (unsigned short) (de->deMTime[0] | de->deMTime[1]<<8);
	// store file/dir last update date
	fi->Date = (unsigned short) (de->deMDate[0] | de->deMDate[1]<<8);

	if(gotEntry)
	{
		f->last_dir_entry = entrycount;
		f->last_dir_index = index;
		f->last_dir_sector = sector-1; //protoze ihned po nacteni se posune
		f->last_dir_cluster = actual_cluster;
		f->last_dir_sector_count = seccount; //skace se az za inkrementaci seccount, takze tady se 1 neodecita!
		f->last_dir_valid=gotEntry;
        
        dprint("DE %s: ec:%d, ind: %d, sec: %d, ac: %d, sc: %d\r\n",file_name_buffer, entrycount, index, f->last_dir_sector, f->last_dir_cluster, f->last_dir_sector_count);
	}
    else
    {
        dprint("No entry\r\n");
    }

	return gotEntry;
}


unsigned long nextCluster(unsigned long cluster)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    GlobalSystemValues* gsv = &ctx->gsv;
    FatData* f = &ctx->fat_data;
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
   
    
    
	  unsigned long nextCluster;
      unsigned long fatMask;
	  unsigned long fatOffset;
	  unsigned long sector;
	  unsigned long offset;

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
	  sector = gsv->FirstFATSector + (fatOffset / ((u32)gsv->BytesPerSector));
	  // calculate offset of the our entry within that FAT sector
	  offset = fatOffset % ((u32)gsv->BytesPerSector);

	  // if we don't already have this FAT chunk loaded, go get it
	  // read sector of FAT table
  	  mmcReadCached( sector );

	  // read the nextCluster value
	  nextCluster = (*((unsigned long*) &((char*)mmc_sector_buffer)[offset])) & fatMask;

	  // check to see if we're at the end of the chain
	  if (nextCluster == (CLUST_EOFE & fatMask))
		nextCluster = 0;

	return nextCluster;
}


unsigned long getClusterN(unsigned short ncluster)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    FileInfoStruct* fi = &ctx->file_info;
    FatData* f = &ctx->fat_data;
    
	if(ncluster<fi->vDisk.ncluster)
	{
		fi->vDisk.current_cluster=fi->vDisk.start_cluster;
		fi->vDisk.ncluster=0;
	}

	while(fi->vDisk.ncluster!=ncluster)
	{
		fi->vDisk.current_cluster=nextCluster(fi->vDisk.current_cluster);
		fi->vDisk.ncluster++;
	}

	return (fi->vDisk.current_cluster&(f->fat32_enabled?0xFFFFFFFF:0xFFFF));
}

unsigned short faccess_offset(char mode, unsigned long offset_start, unsigned short ncount)
{
    HWContext* ctx = (HWContext*)__hwcontext;
    FileInfoStruct* fi = &ctx->file_info;
    GlobalSystemValues* gsv = &ctx->gsv;
    unsigned char* atari_sector_buffer = &ctx->atari_sector_buffer[0];
    unsigned char* mmc_sector_buffer = &ctx->mmc_sector_buffer[0];
        
	unsigned short j;
	unsigned long offset=offset_start;
	unsigned long end_of_file;
	unsigned long ncluster, nsector, current_sector;
	unsigned long bytespercluster=((u32)gsv->SectorsPerCluster)*((u32)gsv->BytesPerSector);

	if(offset_start>=fi->vDisk.size)
		return 0;

	end_of_file = (fi->vDisk.size - offset_start);

	ncluster = offset/bytespercluster;
	offset-=ncluster*bytespercluster;

	nsector = offset/((u32)gsv->BytesPerSector);
	offset-=nsector*((u32)gsv->BytesPerSector);

	getClusterN(ncluster);

	current_sector = fatClustToSect(fi->vDisk.current_cluster) + nsector;
	mmcReadCached(current_sector);

	for(j=0;j<ncount;j++,offset++)
	{
			if(offset>=((u32)gsv->BytesPerSector) )
			{
				if(mode==FILE_ACCESS_WRITE && mmcWriteCached(0))
					return 0;

				offset=0;
				nsector++;

				if(nsector>=((u32)gsv->SectorsPerCluster) )
				{
					nsector=0;
					ncluster++;
					getClusterN(ncluster);
				}

				current_sector = fatClustToSect(fi->vDisk.current_cluster) + nsector;
				mmcReadCached(current_sector);
			}

		if(!end_of_file)
			break; //return (j&0xFFFF);

		if(mode==FILE_ACCESS_WRITE)
			mmc_sector_buffer[offset]=atari_sector_buffer[j]; //SDsektor<-atarisektor
		else
			atari_sector_buffer[j]=mmc_sector_buffer[offset]; //atarisektor<-SDsektor

		end_of_file--;

	}
	

	if(mode==FILE_ACCESS_WRITE && mmcWriteCached(0))
		return 0;

	return (j&0xFFFF);
}
