/*! \file fat.h \brief FAT16/32 file system driver. */
//*****************************************************************************
//
// Title		: FAT16/32 file system driver
// Author		: Pascal Stang / Alexey Spirkov
// Date			: 11/07/2000
// Revised		: 23/03/2017
// Version		: 1.0
// Target MCU	: ARM-Cortex-M 
// Editor Tabs	: 4
//
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
///	\ingroup general
/// \defgroup fat FAT16/32 File System Interface (fat.c)
/// \code #include "fat.h" \endcode
/// \par Overview
///		This FAT16/32 interface allows you to detect and mount FAT16/32
///		partitions, browse directories and files, and read file data.
///		The interface is designed to operate with the avrlib IDE/ATA driver.
///		Reading FAT efficiently requires at least 512+ bytes of RAM so this
///		interface may not be suitable for processors with less than 1K of RAM.
///		This interface will properly follow a file's cluster chain so files
///		need not be defragmented.
///
/// \note This code is based in part on work done by Jesper Hansen for his
///		excellent YAMPP MP3 player project.
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

#ifndef FAT_H
#define FAT_H

#include "atari.h"
    
#define FALSE 0
#define TRUE -1

#define DRIVE0 1

// Some useful cluster numbers
#define MSDOSFSROOT     0		// cluster 0 means the root dir
#define CLUST_FREE      0               // cluster 0 also means a free cluster
#define MSDOSFSFREE     CLUST_FREE
#define CLUST_FIRST     2               // first legal cluster number
#define CLUST_RSRVD     0xfffffff6      // reserved cluster range
#define CLUST_BAD       0xfffffff7      // a cluster with a defect
#define CLUST_EOFS      0xfffffff8      // start of eof cluster range
#define CLUST_EOFE      0xffffffff      // end of eof cluster range

#define FAT12_MASK      0x00000fff      // mask for 12 bit cluster numbers
#define FAT16_MASK      0x0000ffff      // mask for 16 bit cluster numbers
#define FAT32_MASK      0x0fffffff      // mask for FAT32 cluster numbers


// Partition Type used in the partition record
#define PART_TYPE_UNKNOWN		0x00
#define PART_TYPE_FAT12			0x01
#define PART_TYPE_XENIX			0x02
#define PART_TYPE_DOSFAT16		0x04
#define PART_TYPE_EXTDOS		0x05
#define PART_TYPE_FAT16			0x06
#define PART_TYPE_NTFS			0x07
#define PART_TYPE_FAT32			0x0B
#define PART_TYPE_FAT32LBA		0x0C
#define PART_TYPE_FAT16LBA		0x0E
#define PART_TYPE_EXTDOSLBA		0x0F
#define PART_TYPE_ONTRACK		0x33
#define PART_TYPE_NOVELL		0x40
#define PART_TYPE_PCIX			0x4B
#define PART_TYPE_PHOENIXSAVE	0xA0
#define PART_TYPE_CPM			0xDB
#define PART_TYPE_DBFS			0xE0
#define PART_TYPE_BBT			0xFF

typedef struct _partrecord // length 16 bytes
{			
	unsigned char	prIsActive;					// 0x80 indicates active partition
	unsigned char	prStartHead;				// starting head for partition
	unsigned short	prStartCylSect;				// starting cylinder and sector
	unsigned char	prPartType;					// partition type (see above)
	unsigned char	prEndHead;					// ending head for this partition
	unsigned short	prEndCylSect;				// ending cylinder and sector
	unsigned long	prStartLBA;					// first LBA sector for this partition
	unsigned long	prSize;						// size of this partition (bytes or sectors ?)
} __attribute__((packed)) partrecord; 

        
typedef struct _partsector
{
	unsigned char	psPartCode[512-64-2];		// pad so struct is 512b
	unsigned char	psPart[64];					// four partition records (64 bytes)
	unsigned char	psBootSectSig0;				// two signature bytes (2 bytes)
	unsigned char	psBootSectSig1;
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
} __attribute__((packed)) partsector;


// Format of a boot sector.  This is the first sector on a DOS floppy disk
// or the first sector of a partition on a hard disk.  But, it is not the
// first sector of a partitioned hard disk.
struct bootsector33 {
	unsigned char 	bsJump[3];					// jump inst E9xxxx or EBxx90
	unsigned char	bsOemName[8];				// OEM name and version
	unsigned char	bsBPB[19];					// BIOS parameter block
	unsigned char	bsDriveNumber;				// drive number (0x80)
	unsigned char	bsBootCode[479];			// pad so struct is 512b
	unsigned char	bsBootSectSig0;				// boot sector signature byte 0x55
	unsigned char	bsBootSectSig1;				// boot sector signature byte 0xAA
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
} __attribute__((packed));

struct extboot {
	unsigned char	exDriveNumber;				// drive number (0x80)
	unsigned char	exReserved1;				// reserved
	unsigned char	exBootSignature;			// ext. boot signature (0x29)
#define EXBOOTSIG       0x29
	unsigned char	exVolumeID[4];				// volume ID number
	unsigned char	exVolumeLabel[11];			// volume label
	unsigned char	exFileSysType[8];			// fs type (FAT12 or FAT16)
} __attribute__((packed));

struct bootsector50 {
	unsigned char	bsJump[3];					// jump inst E9xxxx or EBxx90
	unsigned char	bsOemName[8];				// OEM name and version
	unsigned char	bsBPB[25];					// BIOS parameter block
	unsigned char	bsExt[26];					// Bootsector Extension
	unsigned char	bsBootCode[448];			// pad so structure is 512b
	unsigned char	bsBootSectSig0;				// boot sector signature byte 0x55 
	unsigned char	bsBootSectSig1;				// boot sector signature byte 0xAA
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
} __attribute__((packed));

struct bootsector710 {
	unsigned char	bsJump[3];					// jump inst E9xxxx or EBxx90
	unsigned char	bsOEMName[8];				// OEM name and version
	unsigned char	bsBPB[53];					// BIOS parameter block
	unsigned char	bsExt[26];					// Bootsector Extension
	unsigned char	bsBootCode[418];			// pad so structure is 512b
	unsigned char	bsBootSectSig2;				// 2 & 3 are only defined for FAT32?
	unsigned char	bsBootSectSig3;
	unsigned char	bsBootSectSig0;				// boot sector signature byte 0x55
	unsigned char	bsBootSectSig1;				// boot sector signature byte 0xAA
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
#define BOOTSIG2        0
#define BOOTSIG3        0
} __attribute__((packed));


/***************************************************************/
/***************************************************************/

// BIOS Parameter Block (BPB) for DOS 3.3
struct bpb33 {
        unsigned short	bpbBytesPerSec; // bytes per sector
        unsigned char    bpbSecPerClust;	// sectors per cluster
        unsigned short	bpbResSectors;	// number of reserved sectors
        unsigned char	bpbFATs;		// number of FATs
        unsigned short	bpbRootDirEnts;	// number of root directory entries
        unsigned short	bpbSectors;		// total number of sectors
        unsigned char	bpbMedia;		// media descriptor
        unsigned short	bpbFATsecs;     // number of sectors per FAT
        unsigned short	bpbSecPerTrack; // sectors per track
        unsigned short	bpbHeads;       // number of heads
        unsigned short	bpbHiddenSecs;  // number of hidden sectors
} __attribute__((packed));

// BPB for DOS 5.0
// The difference is bpbHiddenSecs is a short for DOS 3.3,
// and bpbHugeSectors is not present in the DOS 3.3 bpb.
struct bpb50 {
        unsigned short	bpbBytesPerSec; // bytes per sector
        unsigned char	bpbSecPerClust; // sectors per cluster
        unsigned short	bpbResSectors;  // number of reserved sectors
        unsigned char	bpbFATs;        // number of FATs
        unsigned short	bpbRootDirEnts; // number of root directory entries
        unsigned short	bpbSectors;     // total number of sectors
        unsigned char	bpbMedia;       // media descriptor
        unsigned short	bpbFATsecs;     // number of sectors per FAT
        unsigned short	bpbSecPerTrack; // sectors per track
        unsigned short	bpbHeads;       // number of heads
        unsigned long	bpbHiddenSecs;  // # of hidden sectors
// 3.3 compat ends here
        unsigned long	bpbHugeSectors; // # of sectors if bpbSectors == 0
} __attribute__((packed));

// BPB for DOS 7.10 (FAT32)
// This one has a few extensions to bpb50.
struct bpb710 {
		unsigned short	bpbBytesPerSec;	// bytes per sector
		unsigned char	bpbSecPerClust;	// sectors per cluster
		unsigned short	bpbResSectors;	// number of reserved sectors
		unsigned char	bpbFATs;		// number of FATs
		unsigned short	bpbRootDirEnts;	// number of root directory entries
		unsigned short	bpbSectors;		// total number of sectors
		unsigned char	bpbMedia;		// media descriptor
		unsigned short	bpbFATsecs;		// number of sectors per FAT
		unsigned short	bpbSecPerTrack;	// sectors per track
		unsigned short	bpbHeads;		// number of heads
		unsigned long	bpbHiddenSecs;	// # of hidden sectors
// 3.3 compat ends here
		unsigned long	bpbHugeSectors;	// # of sectors if bpbSectors == 0
// 5.0 compat ends here
		unsigned long     bpbBigFATsecs;// like bpbFATsecs for FAT32
		unsigned short      bpbExtFlags;	// extended flags:
#define FATNUM    0xf			// mask for numbering active FAT
#define FATMIRROR 0x80			// FAT is mirrored (like it always was)
		unsigned short      bpbFSVers;	// filesystem version
#define FSVERS    0				// currently only 0 is understood
		unsigned long     bpbRootClust;	// start cluster for root directory
		unsigned short      bpbFSInfo;	// filesystem info structure sector
		unsigned short      bpbBackup;	// backup boot sector
		// There is a 12 byte filler here, but we ignore it
} __attribute__((packed));


// ***************************************************************
// * byte versions of the above structs                          *
// ***************************************************************

// BIOS Parameter Block (BPB) for DOS 3.3
struct byte_bpb33 {
        unsigned char bpbBytesPerSec[2];		// bytes per sector
        unsigned char bpbSecPerClust;        // sectors per cluster
        unsigned char bpbResSectors[2];      // number of reserved sectors
        unsigned char bpbFATs;               // number of FATs
        unsigned char bpbRootDirEnts[2];     // number of root directory entries
        unsigned char bpbSectors[2];         // total number of sectors
        unsigned char bpbMedia;              // media descriptor
        unsigned char bpbFATsecs[2];         // number of sectors per FAT
        unsigned char bpbSecPerTrack[2];     // sectors per track
        unsigned char bpbHeads[2];           // number of heads
        unsigned char bpbHiddenSecs[2];      // number of hidden sectors
} __attribute__((packed));

// BPB for DOS 5.0
// The difference is bpbHiddenSecs is a short for DOS 3.3,
// and bpbHugeSectors is not in the 3.3 bpb.
struct byte_bpb50 {
        unsigned char bpbBytesPerSec[2];     // bytes per sector
        unsigned char bpbSecPerClust;        // sectors per cluster
        unsigned char bpbResSectors[2];      // number of reserved sectors
        unsigned char bpbFATs;               // number of FATs
        unsigned char bpbRootDirEnts[2];     // number of root directory entries
        unsigned char bpbSectors[2];         // total number of sectors
        unsigned char bpbMedia;              // media descriptor
        unsigned char bpbFATsecs[2];         // number of sectors per FAT
        unsigned char bpbSecPerTrack[2];     // sectors per track
        unsigned char bpbHeads[2];           // number of heads
        unsigned char bpbHiddenSecs[4];      // number of hidden sectors
        unsigned char bpbHugeSectors[4];		// # of sectors if bpbSectors == 0
} __attribute__((packed));

// BPB for DOS 7.10 (FAT32).
// This one has a few extensions to bpb50.
struct byte_bpb710 {
        unsigned char bpbBytesPerSec[2];     // bytes per sector
        unsigned char bpbSecPerClust;        // sectors per cluster
        unsigned char bpbResSectors[2];      // number of reserved sectors
        unsigned char bpbFATs;               // number of FATs
        unsigned char bpbRootDirEnts[2];     // number of root directory entries
        unsigned char bpbSectors[2];         // total number of sectors
        unsigned char bpbMedia;              // media descriptor
        unsigned char bpbFATsecs[2];         // number of sectors per FAT
        unsigned char bpbSecPerTrack[2];     // sectors per track
        unsigned char bpbHeads[2];           // number of heads
        unsigned char bpbHiddenSecs[4];      // # of hidden sectors
        unsigned char bpbHugeSectors[4];     // # of sectors if bpbSectors == 0
        unsigned char bpbBigFATsecs[4];      // like bpbFATsecs for FAT32
        unsigned char bpbExtFlags[2];        // extended flags:
        unsigned char bpbFSVers[2];          // filesystem version
        unsigned char bpbRootClust[4];       // start cluster for root directory
        unsigned char bpbFSInfo[2];          // filesystem info structure sector
        unsigned char bpbBackup[2];          // backup boot sector
        // There is a 12 byte filler here, but we ignore it
} __attribute__((packed));

// FAT32 FSInfo block.
struct fsinfo {
        unsigned char fsisig1[4];
        unsigned char fsifill1[480];
        unsigned char fsisig2[4];
        unsigned char fsinfree[4];
        unsigned char fsinxtfree[4];
        unsigned char fsifill2[12];
        unsigned char fsisig3[4];
        unsigned char fsifill3[508];
        unsigned char fsisig4[4];
} __attribute__((packed));


// Structure of a dos directory entry.
struct direntry {
		unsigned char		deName[8];      // filename, blank filled
#define SLOT_EMPTY      0x00            // slot has never been used
#define SLOT_E5         0x05            // the real value is 0xE5
#define SLOT_DELETED    0xE5            // file in this slot deleted
		unsigned char		deExtension[3]; // extension, blank filled
		unsigned char		deAttributes;   // file attributes
#define ATTR_NORMAL     0x00            // normal file
#define ATTR_READONLY   0x01            // file is readonly
#define ATTR_HIDDEN     0x02            // file is hidden
#define ATTR_SYSTEM     0x04            // file is a system file
#define ATTR_VOLUME     0x08            // entry is a volume label
#define ATTR_LONG_FILENAME	0x0F		// this is a long filename entry			    
#define ATTR_DIRECTORY  0x10            // entry is a directory name
#define ATTR_ARCHIVE    0x20            // file is new or modified
		unsigned char        deLowerCase;    // NT VFAT lower case flags  (set to zero)
#define LCASE_BASE      0x08            // filename base in lower case
#define LCASE_EXT       0x10            // filename extension in lower case
		unsigned char        deCHundredth;   // hundredth of seconds in CTime
		unsigned char        deCTime[2];     // create time
		unsigned char        deCDate[2];     // create date
		unsigned char        deADate[2];     // access date
		unsigned short        deHighClust; 	// high bytes of cluster number
		unsigned char        deMTime[2];     // last update time
		unsigned char        deMDate[2];     // last update date
		unsigned short        deStartCluster; // starting cluster of file
		unsigned long       deFileSize;  	// size of file in bytes
} __attribute__((packed));

// number of directory entries in one sector
#define DIRENTRIES_PER_SECTOR	0x10

// Structure of a Win95 long name directory entry
struct winentry {
		unsigned char		weCnt;			// 
#define WIN_LAST        0x40
#define WIN_CNT         0x3f
		unsigned char		wePart1[10];
		unsigned char		weAttributes;
#define ATTR_WIN95      0x0f
		unsigned char		weReserved1;
		unsigned char		weChksum;
		unsigned char		wePart2[12];
		unsigned short       	weReserved2;
		unsigned char		wePart3[4];
} __attribute__((packed));

#define WIN_ENTRY_CHARS	13      // Number of chars per winentry

// Maximum filename length in Win95
// Note: Must be < sizeof(dirent.d_name)
#define WIN_MAXLEN      255

// This is the format of the contents of the deTime field in the direntry
// structure.
// We don't use bitfields because we don't know how compilers for
// arbitrary machines will lay them out.
#define DT_2SECONDS_MASK        0x1F    // seconds divided by 2
#define DT_2SECONDS_SHIFT       0
#define DT_MINUTES_MASK         0x7E0   // minutes
#define DT_MINUTES_SHIFT        5
#define DT_HOURS_MASK           0xF800  // hours
#define DT_HOURS_SHIFT          11

// This is the format of the contents of the deDate field in the direntry
// structure.
#define DD_DAY_MASK				0x1F	// day of month
#define DD_DAY_SHIFT			0
#define DD_MONTH_MASK			0x1E0	// month
#define DD_MONTH_SHIFT			5
#define DD_YEAR_MASK			0xFE00	// year - 1980
#define DD_YEAR_SHIFT			9


unsigned char fatInit();
unsigned long fatClustToSect(unsigned long clust);
unsigned char fatChangeDirectory(unsigned short entry);
unsigned char fatGetDirEntry(file_t* file, unsigned char* file_name, unsigned short entry, unsigned char use_long_names);
unsigned long nextCluster(unsigned long clust);

unsigned short faccess_offset(file_t* file, char mode, unsigned long offset_start, unsigned char* buffer, unsigned short ncount);

typedef struct _FatData
{
    unsigned long last_dir_start_cluster;
    unsigned char last_dir_valid;
    unsigned short last_dir_entry;
    unsigned long last_dir_sector;
    unsigned char last_dir_sector_count;
    unsigned long last_dir_cluster;
    unsigned char last_dir_index;
    unsigned char fat32_enabled;
	unsigned long dir_cluster;    
} FatData;

#define FILE_ACCESS_READ	0
#define FILE_ACCESS_WRITE	1


#endif
