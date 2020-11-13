/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "project.h"
#include <CyLib.h>
#include <stdlib.h>

#include "cfg.h"
#include "sio.h"
#include "diskemu.h"
#include "sdrivecmd.h"
#undef DEBUG
#include "dprint.h"
#include "fat.h"
#include "tapeemu.h"
#include "gpio.h"

static const char ConfigName[] = CONFIG_FILE_NAME;

static const char DefaultTapeName[] = "DEFAULT CAS";

int DefaultSettings(Settings* settings)
{
    memset(settings, 0, sizeof(Settings));
    settings->emulated_drive_no = 1; // Emulate D1 by defaut
    settings->default_pokey_div = US_POKEY_DIV_DEFAULT; 
    settings->is_1050 = 0;
    settings->sd_freq = 8000000;    // Slow speed by default - NB! not compatible with some ATX
    settings->tape_baud = 600;      // defaut non turbo mode
    return 0;
}

static const char DriveSettings[] =   "DRIVE";
static const char SIODivSettings[] =  "SIO_DIV";
static const char SDFreqSettings[] =  "SD_FREQ";
static const char ATX1050Settings[] = "ATX_1050";
static const char MountSettings[] =   "MOUNT_";
static const char TapeBaudSettings[]= "TAPE_BAUD";


int ParseConfigLine(HWContext* ctx, Settings* settings, char* buffer)
{
    if(!strncmp(MountSettings, buffer, 6))  // Most of setting are mount - so check it first
    {
        char* file_name = 0;
        file_t* file = 0;
        if(buffer[6] == 'D')        // drive
        {
            int disk_no = atoi(buffer+7);
            file = GetVDiskPtr(disk_no);
            file_name = strchr(buffer, '=');
            if(disk_no >= DEVICESNUM)
            {
                dprint("Skip D%d config\r\n", disk_no);
                return 0;
            }
        }
        else if( buffer[6] == 'T')  // tape
        {
            file_name = strchr(buffer, '=');
            file = 0;               
        }
        else
        {
            dprint("Skip mount [%s]\r\n", buffer);        
        }

        if(file_name)
        {
            char name_buffer[250];
            file_name++;    // skip '='
            // remove comments
            char* comment = strchr(file_name, '#');
            if(comment)
                *comment = 0;

            if(strlen(file_name) < 3)
                return 0;

            strcpy(&name_buffer[0], file_name);
            

            if(file)            
                MountFile(ctx, file, &name_buffer[0], (unsigned char*) buffer);
            else    // tape
                MountTape(ctx, GetVDiskPtr(0), &name_buffer[0], settings, (unsigned char*) buffer);
        }            
    }
    else if(!strncmp(DriveSettings, buffer, 5))
    {
        settings->emulated_drive_no = atoi(buffer+6);
        if(settings->emulated_drive_no == 0)
            settings->emulated_drive_no = 1;
    }
    else if(!strncmp(SIODivSettings, buffer, 7))
    {
        settings->default_pokey_div = atoi(buffer+8);
        if(settings->default_pokey_div == 0)
             settings->default_pokey_div = US_POKEY_DIV_DEFAULT;
    }
    else if(!strncmp(SDFreqSettings, buffer, 7))
    {
        unsigned int freq = atoi(buffer+8);
        if(freq == 0)
             freq = 8000000;
        if(freq != settings->sd_freq)
        {
            settings->sd_freq = freq;
            // restart with hight speed
            return 1;
        }                
        settings->sd_freq = atoi(buffer+8);
        if(!settings->sd_freq)
            settings->sd_freq = 8000000;
    }
    else if(!strncmp(ATX1050Settings, buffer, 8))
    {
        settings->is_1050 = atoi(buffer+9);
    }
    else if(!strncmp(TapeBaudSettings, buffer, 9))
    {
        settings->tape_baud = atoi(buffer+10);
        if(!settings->tape_baud)
            settings->tape_baud = 600;
    }
    else
    {
        dprint("Skip settings [%s]\r\n", buffer);
    }
    return 0;
}

int ReadSettings(HWContext* ctx, Settings* settings, unsigned char* buffer)
{
    file_t config_file;
    unsigned long config_offset = 0;
    
    LED_OFF;
	if ( !fatInit(ctx) ) 
	{
		LED_GREEN_OFF;
        CyDelayUs(1000u);
        return 1;
	}    

    ResetDrives();

    
    memset(&config_file, 0, sizeof(file_t));
    config_file.dir_cluster = GetVDiskPtr(0)->dir_cluster;  // set to root folder
    if(MountFile(ctx, &config_file, &ConfigName[0], buffer))
    {
        dprint("Unable to mount config file %s\r\n", &ConfigName[0]);
        return 0;
    }
    
    // read line by line from file
    while (config_offset < config_file.size)
    {
        unsigned short bytes = faccess_offset(ctx, &config_file, FILE_ACCESS_READ, config_offset, buffer, ATARI_BUFFER_SIZE - 1);
        if(!bytes)
            break;                
        buffer[bytes-1] = 0;
        
        // search for next line
        char *next_line = strchr((char*) buffer, 0x0a);
        if(next_line)
        {
            config_offset += next_line - (char*)buffer + 1;
            *next_line-- = 0;
            if(*next_line == 0x0d)
                *next_line = 0;
        }
        else
            config_offset = config_file.size;
        
        if(ParseConfigLine(ctx, settings, (char*) buffer))
            break;
    }       
   
    return 0;    
}

int WriteSettings(HWContext* ctx, Settings* settings, unsigned char* buffer)
{
    // to do
    return 0;    
}
