/*! \file fat.c \brief FAT16/32 file system driver. */
//*****************************************************************************
//
// File Name	: 'fat.c'
// Title		: FAT16/32 file system driver
// Author		: Pascal Stang
// Date			: 11/07/2000
// Revised		: 12/12/2000
// Version		: 0.3
// Target MCU	: ATmega103 (should work for Atmel AVR Series)
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
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "fat.h"
#include "mmc.h"

void USART_SendString(char *buff);
u08 mmcReadCached(u32 sector);


#define FileNameBuffer atari_sector_buffer
extern unsigned char atari_sector_buffer[256];
extern unsigned char mmc_sector_buffer[512];	// one sector
extern struct GlobalSystemValues GS;
extern struct FileInfoStruct FileInfo;			//< file information for last file accessed


unsigned long fatClustToSect(unsigned short clust)
{
	u32 ret;
	if(clust==MSDOSFSROOT)
		return (u32)((u32)(FirstDataSector) - (u32)(RootDirSectors));
	
	ret = (u32)(clust-2);
	ret *= (u32)SectorsPerCluster;
	ret += (u32)FirstDataSector;
	return ((unsigned long)ret);
}


//POZOR - DEBUG !!!
//prvne definovana promenna
u32 debug_endofvariables;		//promenna co je v pameti jako posledni (za ni je uz jen stack)
//POZOR - DEBUG !!!

unsigned short last_dir_start_cluster;
unsigned char last_dir_valid;
unsigned short last_dir_entry=0x0;
unsigned long last_dir_sector=0;
unsigned char last_dir_sector_count=0;
unsigned short last_dir_cluster=0;
unsigned char last_dir_index=0;


unsigned char fatInit()
{
	struct partrecord PartInfo;
	struct bpb710 *bpb;

	// read partition table
	// TODO.... error checking
	mmcReadCached(0);
	// map first partition record	
	// save partition information to global PartInfo
	PartInfo = *((struct partrecord *) ((struct partsector *) (char*)mmc_sector_buffer)->psPart);

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
	FirstDataSector	= PartInfo.prStartLBA;

	RootDirSectors = bpb->bpbRootDirEnts>>4; // /16 ; 512/16 = 32  (sector size / max entries in one sector = number of sectors)

	// bpbFATsecs is non-zero and is therefore valid
	FirstDataSector	+= bpb->bpbResSectors + bpb->bpbFATs * bpb->bpbFATsecs + RootDirSectors;

	SectorsPerCluster	= bpb->bpbSecPerClust;
	BytesPerSector		= bpb->bpbBytesPerSec;
	FirstFATSector		= bpb->bpbResSectors + PartInfo.prStartLBA;

	// set current directory to root (\)
	FileInfo.vDisk.dir_cluster = MSDOSFSROOT; //RootDirStartCluster;

    last_dir_start_cluster=0xffff;
	//last_dir_valid=0;		//<-- neni potreba, protoze na zacatku fatGetDirEntry se pri zjisteni
							//ze je (FileInfo.vDisk.dir_cluster!=last_dir_start_cluster)
							//vynuluje last_dir_valid=0;

	if (   PartInfo.prPartType==PART_TYPE_DOSFAT16
		|| PartInfo.prPartType==PART_TYPE_FAT16
		|| PartInfo.prPartType==PART_TYPE_FAT16LBA
	   )
		return 1; //je to ok
	else
		return 0; //neni to zadny filesystem co umime
}

//////////////////////////////////////////////////////////////

unsigned char fatGetDirEntry(unsigned short entry, unsigned char use_long_names)
{
	unsigned long sector;
	struct direntry *de = 0;	// avoid compiler warning by initializing
	struct winentry *we;
	unsigned char haveLongNameEntry;
	unsigned char gotEntry;
	unsigned short b;
	u08 index;
	unsigned short entrycount = 0;
	unsigned short actual_cluster = FileInfo.vDisk.dir_cluster;
	unsigned char seccount=0;

	haveLongNameEntry = 0;
	gotEntry = 0;

	if (FileInfo.vDisk.dir_cluster!=last_dir_start_cluster)
	{
		//zmenil se adresar, takze musi pracovat s nim
		//a zneplatnit last_dir polozky
		last_dir_start_cluster=FileInfo.vDisk.dir_cluster;
		last_dir_valid=0;
	}

	if ( !last_dir_valid
		 || (entry<=last_dir_entry && (entry!=last_dir_entry || last_dir_valid!=1 || use_long_names!=0))
	   )
	{
		//musi zacit od zacatku
		sector = fatClustToSect(FileInfo.vDisk.dir_cluster);
		//index = 0;
		goto fat_read_from_begin;
	}
	else
	{
		//muze zacit od posledne pouzite
		entrycount=last_dir_entry;
		index = last_dir_index;
		sector=last_dir_sector;
		actual_cluster=last_dir_cluster;
		seccount = last_dir_sector_count;
		goto fat_read_from_last_entry;
	}

	while(1)
	{
		if(index==16)	// time for next sector ?
		{
			if ( actual_cluster==MSDOSFSROOT )
			{
				//v MSDOSFSROOT se nerozlisuji clustery
				//protoze MSDOSFSROOT ma sektory stale dal za sebou bez clusterovani
				if (seccount>=RootDirSectors) return 0; //prekrocil maximalni pocet polozek v MSDOSFSROOT
			}
			else //MUSI BYT!!! (aby neporovnaval seccount je-li actual_cluster==MSDOSFSROOT)
			if( seccount>=SectorsPerCluster )
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
				fnbPtr = &FileNameBuffer[b];

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

						dptr = &FileNameBuffer;
						for (i=0;i<11;i++)	*dptr++ = de->deName[i];		// copy name+ext
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
							dptr=(FileNameBuffer+12);
							i=4; do { sptr=dptr-1; *dptr=*sptr; dptr=sptr; i--; } while(i>0);
							if (FileNameBuffer[0]!='.') *dptr='.'; //jen pro jine nez '.' a '..'
							//NAME    .EXT => NAME.EXT
							sptr=dptr=&FileNameBuffer;
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
	FileInfo.vDisk.start_cluster = (unsigned short) (de->deStartCluster);
	// fileindex
	FileInfo.vDisk.file_index = entry;	//fileindex teto polozky
	// store file/dir size (note: size field for subdirectory entries is always zero)
	FileInfo.vDisk.size = de->deFileSize;
	// store file/dir attributes
	FileInfo.Attr = de->deAttributes;
	// store file/dir last update time
	FileInfo.Time = (unsigned short) (de->deMTime[0] | de->deMTime[1]<<8);
	// store file/dir last update date
	FileInfo.Date = (unsigned short) (de->deMDate[0] | de->deMDate[1]<<8);

	if(gotEntry)
	{
		last_dir_entry = entrycount;
		last_dir_index = index;
		last_dir_sector = sector-1; //protoze ihned po nacteni se posune
		last_dir_cluster = actual_cluster;
		last_dir_sector_count = seccount; //skace se az za inkrementaci seccount, takze tady se 1 neodecita!
		last_dir_valid=gotEntry;
	}

	return gotEntry;
}


unsigned short nextCluster(unsigned short cluster)
{
	  unsigned short nextCluster;
	  unsigned long fatOffset;
	  unsigned long sector;
	  unsigned long offset;

	  // two FAT bytes (16 bits) for every cluster
	  fatOffset = ((u32)cluster) << 1;

	  // calculate the FAT sector that we're interested in
	  sector = FirstFATSector + (fatOffset / ((u32)BytesPerSector));
	  // calculate offset of the our entry within that FAT sector
	  offset = fatOffset % ((u32)BytesPerSector);

	  // if we don't already have this FAT chunk loaded, go get it
	  // read sector of FAT table
  	  mmcReadCached( sector );

	  // read the nextCluster value
	  nextCluster = (*((unsigned short*) &((char*)mmc_sector_buffer)[offset])) & FAT16_MASK;

	  // check to see if we're at the end of the chain
	  if (nextCluster == (CLUST_EOFE & FAT16_MASK))
		nextCluster = 0;

	return (nextCluster&0xFFFF);
}
