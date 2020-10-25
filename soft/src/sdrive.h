/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
    
#ifndef _SDRIVE_H
#define _SDRIVE_H

#define DEVICESNUM	13	//	//D0:-D12:

typedef struct _SharedParameters
{
	unsigned char actual_drive_number;  // p0
	unsigned char fastsio_active;       // p1
	unsigned char fastsio_pokeydiv;     // p2
	unsigned char bootloader_relocation;// p3
	unsigned long extraSDcommands_readwritesectornumber; // p4_5_6_7
} __attribute__((packed)) SharedParameters;


typedef struct _SDriveSettings
{
    unsigned int emulated_drive_no;
    unsigned char default_pokey_div;
    unsigned char is_1050;
    unsigned int sd_freq;
    unsigned int is_tape_turbo;
//    unsigned char* file_name_d0;
//    unsigned char* file_name_d1;
//    unsigned char* file_name_d2;
//    unsigned char* file_name_d3;
//    unsigned char* file_name_d4;
    const char* file_name_tape;
} Settings;

extern SharedParameters shared_parameters;
extern Settings settings;

#endif /* _SDRIVE_H */