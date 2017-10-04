#ifndef _ATARI_H_
#define _ATARI_H_
   
#define FLAGS_ATRMEDIUMSIZE		0x80
#define FLAGS_XEXLOADER			0x40
#define FLAGS_ATRDOUBLESECTORS	0x20
#define FLAGS_XFDTYPE			0x10
#define FLAGS_READONLY			0x08
#define FLAGS_WRITEERROR		0x04
#define FLAGS_DRIVEON			0x01

// Stuctures
typedef struct						//4+4+4+2+2+4+1=21
{
	unsigned long start_cluster;		//< file starting cluster
	unsigned long dir_cluster;
	unsigned long current_cluster;
	unsigned short ncluster;
	unsigned short file_index;			//< file index
	unsigned long size;					//< file size
	unsigned char flags;				//< file flags
} __attribute__((packed)) virtual_disk_t ;

typedef struct _FileInfoStruct
{
	unsigned char Attr;				//< file attr for last file accessed
	unsigned short Date;			//< last update date
	unsigned short Time;			//< last update time
	unsigned char percomstate;		//=0 default, 1=percomwrite ok (single sectors), 2=percomwrite ok (double sectors), 3=percomwrite bad
	//
	virtual_disk_t vDisk;
} __attribute__((packed)) FileInfoStruct ;
    
    
#endif