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

typedef struct
{
	unsigned char Attr;				//< file attr for last file accessed
	unsigned short Date;			//< last update date
	unsigned short Time;			//< last update time
	unsigned char percomstate;		//=0 default, 1=percomwrite ok (single sectors), 2=percomwrite ok (double sectors), 3=percomwrite bad
} __attribute__((packed)) fileinfo_t;
    
typedef struct						//4+4+4+2+2+4+1=21
{
    unsigned long dir_cluster;          //< containing directory claster
    unsigned long start_cluster;		//< file starting cluster
	unsigned long current_cluster;
	unsigned short ncluster;
	unsigned short file_index;			//< file index
	unsigned long size;					//< file size
	unsigned char flags;				//< file flags
    fileinfo_t fi;                     //< additional file information
} __attribute__((packed)) file_t ;

   
    
#endif