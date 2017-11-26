/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
#ifndef _SDRIVE_H
#define _SDRIVE_H

enum SDrive_Commands
{
    SDC_Put = 0x50,           // write sector without verification (AUX1 - LSB, AUX2 - MSB of sector) 
    SDC_Read = 0x52,          // read sector (AUX1 - LSB, AUX2 - MSB of sector) 
    SDC_GetStatus = 0x53,     // get drive status

    SDC_GetFileNames = 0xC0,  // $C0 xl xh - Get 20 filenames from xhxl. 8.3 + attribute (11+1)*20+1 [<241]
						      //             +1st byte of filename 21 (+1)
    SDC_SetFastIO = 0xC1,     // $C1 nn ?? - set Fast IO pokey divisor
    SDC_SetBLPos = 0xC2,      // $C2 nn ?? - set bootloader relocation = $0700+$nn00

    SDC_GetVDFlag = 0xDB,     // get vDisk flags

    SDC_GetStatus = 0xE0,     // $E0  n ?? - Get status. n=bytes of infostring "SDriveVV YYYYMMDD..."
    SDC_InitDrive = 0xE1,     // $E1  n ?? - Init drive. n == 0 - complete, else set boot drive to d0
                              //             without changing actual drive
    SDC_Deactivate = 0xE2,    // $E2  n ?? - Deactivation of device Dn:
    SDC_CDFromDrive= 0xE3,    // $E3  n ?? - Set actual directory by device Dn: 
                              //             and get filename. 8.3 + attribute + fileindex [11+1+2=14]
    SDC_GetDetails = 0xE5,    // E5 xl xh  - Get more detailed filename xhxl. 
                              //             8.3 + attribute + size/date/time [11+1+8=20]
    SDC_GetLongFName = 0xE7,  // $E6 xl xh - Get longfilename xhxl. [256]
    SDC_GetActDirLen = 0xEA,  // $EA ?? ?? - Get number of items in actual directory [<2]
    SDC_GetPath = 0xEB,       // $EB  n ?? - GetPath (n=number of updirs, if(n==0||n>20) n=20 ) 
                              //             [<n*12] for n==0 [<240]
    SDC_FindFile = 0xEC,      // $EC  n  m - Find filename 8+3.
                              //              
    SDC_FindNext = 0xED,      // $ED xl xh - Find filename (8+3) from xhxl index in actual directory//
                              //             get filename 8.3+attribute+fileindex [11+1+2=14]
    SDC_ChangeDrive = 0xEE,   // $EE  n ?? - Swap vDn: (n=0..8) with D{SDrivenum}:  
                              //            (if n>=8) Set default (all devices with relevant numbers).
    SDC_GetParams = 0xEF,     // $EF  n  m - Get SDrive parameters (n=len, m=from) [<n]
    SDC_Attach_VD0 = 0xF0,    // $F0 xl xh - Attach file no xhxl to vD0
    SDC_Attach_VD1 = 0xF1,    // $F1 xl xh - Attach file no xhxl to vD1
    SDC_Attach_VD2 = 0xF2,    // $F2 xl xh - Attach file no xhxl to vD2
    SDC_Attach_VD3 = 0xF3,    // $F3 xl xh - Attach file no xhxl to vD3
    SDC_Attach_VD4 = 0xF4,    // $F4 xl xh - Attach file no xhxl to vD4
    SDC_Attach_VD5 = 0xF5,    // $F5 xl xh - Attach file no xhxl to vD5
    SDC_Attach_VD6 = 0xF6,    // $F6 xl xh - Attach file no xhxl to vD6
    SDC_Attach_VD7 = 0xF7,    // $F7 xl xh - Attach file no xhxl to vD7
    SDC_Attach_VD8 = 0xF8,    // $F8 xl xh - Attach file no xhxl to vD8
    SDC_CDUp = 0xFD,          // $FD  n ??	 Change actual directory up (..). If n<>0 => get dirname. 
                              //             8+3+attribute+fileindex [<14]
    SDC_CDRoot = 0xFE,        // $FE ?? ?? - Change actual directory to rootdir.
    SDC_CD = 0xFF             // $FF xl xh - Change actual directory to xhxl entry
};

#endif
