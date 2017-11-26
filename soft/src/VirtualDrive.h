/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
#ifndef _VIRTUALDRIVE_H
#define _VIRTUALDRIVE_H

#include "IDiskImage.h"
#include "HWContext.h"

#define FLAGS_ATRMEDIUMSIZE		0x80
#define FLAGS_XEXLOADER			0x40
#define FLAGS_ATRDOUBLESECTORS	0x20
#define FLAGS_XFDTYPE			0x10
#define FLAGS_READONLY			0x08
#define FLAGS_WRITEERROR		0x04
#define FLAGS_DRIVEON			0x01

class CVirtDrive
{
public:
    CVirtDrive(HWContext& context);
    ~CVirtDrive();

private:
    HWContext&     _context;
    unsigned long  _start_cluster;		//< file starting cluster
	unsigned long  _dir_cluster;
	unsigned long  _current_cluster;
	unsigned short _ncluster;
	unsigned short _file_index;			//< file index
	unsigned long  _size;				//< file size
    unsigned char  _flags;				//< file flags
    IDiskImage& ATRImage;
    IDiskImage& XFDImage;
    IDiskImage& XEXImage;
};


#endif
