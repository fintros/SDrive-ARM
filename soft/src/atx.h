/*! \file atx.h \brief ATX file handling. */
//*****************************************************************************
//
// File Name	: 'atx.h'
// Title		: ATX file handling
// Author		: Daniel Noguerol
// Date			: 21/01/2018
// Revised		: 21/01/2018
// Version		: 0.1
// Target MCU	: ???
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef ATX_TEST_ATX_H
#define ATX_TEST_ATX_H

#include "hwcontext.h"    
    
#define ATX_VERSION		0x01
#define STS_EXTENDED	0x40
#define MAX_TRACK	42

enum eDENSITIES { eSINGLE, eMEDIUM, eDOUBLE};

struct atxFileHeader {
    u08 signature[4];
    u16 version;
    u16 minVersion;
    u16 creator;
    u16 creatorVersion;
    u32 flags;
    u16 imageType;
    u08 density;
    u08 reserved0;
    u32 imageId;
    u16 imageVersion;
    u16 reserved1;
    u32 startData;
    u32 endData;
    u08 reserved2[12];
} __attribute__((packed));

struct atxTrackHeader {
    u32 size;
    u16 type;
    u16 reserved0;
    u08 trackNumber;
    u08 reserved1;
    u16 sectorCount;
    u16 rate;
    u16 reserved3;
    u32 flags;
    u32 headerSize;
    u08 reserved4[8];
} __attribute__((packed));

struct atxSectorListHeader {
    u32 next;
    u16 type;
    u16 pad0;
} __attribute__((packed));

struct atxSectorHeader {
    u08 number;
    u08 status;
    u16 timev;
    u32 data;
} __attribute__((packed));

struct atxTrackChunk {
    u32 size;
    u08 type;
    u08 sectorIndex;
    u16 data;
} __attribute__((packed));

/***************************************************************/
/***************************************************************/

// number of angular units in a full disk rotation
#define AU_FULL_ROTATION         26042

// load an ATX file (returns sector size if ATX file is successfully loaded; 0 if not)
u16 loadAtxFile(HWContext* ctx, file_t* pDisk, unsigned char* buffer);
    
// load data for a specific disk sector (returns number of data bytes read or 0 if sector not found)
u16 loadAtxSector(HWContext* ctx, file_t* pDisk, unsigned char* buffer, u16 num, unsigned short *sectorSize);

// returns the current head position in angular units
u16 getCurrentHeadPosition();

// increments an angular unit position with a angular displacement
u16 incAngularDisplacement(u16 start, u16 delta);

// delays until head position reaches the specified position
void waitForAngularPosition(u16 pos, u08 fulldisk);

// hook to allow platform-specific implementations to change byte ordering as needed
void byteSwapAtxFileHeader(struct atxFileHeader * header);

// hook to allow platform-specific implementations to change byte ordering as needed
void byteSwapAtxTrackHeader(struct atxTrackHeader * header);

// hook to allow platform-specific implementations to change byte ordering as needed
void byteSwapAtxSectorListHeader(struct atxSectorListHeader * header);

// hook to allow platform-specific implementations to change byte ordering as needed
void byteSwapAtxSectorHeader(struct atxSectorHeader * header);

// hook to allow platform-specific implementations to change byte ordering as needed
void byteSwapAtxTrackChunk(struct atxTrackChunk *header);

//check for selected drive type, 810 or 1050
u08 is_1050();

#endif //ATX_TEST_ATX_H
