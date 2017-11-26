/*
 * ---------------------------------------------------------------------------
 * -- (c) 2017 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
#ifndef _ATARI_SIO_H
#define _ATARI_SIO_H


 enum ESIO_Devices
 {
    SIOD_Drive_01 = 0x31,      // D1:
    SIOD_Drive_02 = 0x32,      // D2:
    SIOD_Drive_03 = 0x33,      // D3:
    SIOD_Drive_04 = 0x34,      // D4:
    SIOD_Drive_05 = 0x35,      // D5:
    SIOD_Drive_06 = 0x36,      // D5:
    SIOD_Drive_07 = 0x37,      // D7:
    SIOD_Drive_08 = 0x38,      // D8:
    SIOD_Drive_09 = 0x39,      // D9:
    SIOD_Drive_10 = 0x3A,      // D10:
    SIOD_Drive_11 = 0x3B,      // D11:
    SIOD_Drive_12 = 0x3C,      // D12:
    SIOD_Drive_13 = 0x3D,      // D13:
    SIOD_Drive_14 = 0x3E,      // D14:
    SIOD_Drive_15 = 0x3F,      // D15: / D0: (SDrive)
    SIOD_Printer_0 = 0x40,     // P0:
    SIOD_Printer_1 = 0x41,     // P1:
    SIOD_Printer_2 = 0x42,     // P2:
    SIOD_Printer_3 = 0x43,     // P3:
    SIOD_APE = 0x45,           // Atari Peripheral Emulator
    SIOD_AspeQt = 0x46,        // Aspeqt SIO2PC software
    SIOD_34Poll = 0x4F,        // Type 3/4 poll
    SIOD_SerialDevice_0 = 0x50,// 850 serial device
    SIOD_SerialDevice_1 = 0x51,// 850 serial device
    SIOD_SerialDevice_2 = 0x52,// 850 serial device
    SIOD_SerialDevice_3 = 0x53,// 850 serial device
    SIOD_Tape = 0x5F,          // Cassete tape
    SIOD_DOS2DOS = 0x6F,       // PC link
    SIOD_VDrive_01 = 0x71,     // SDrive virtual drive 1
    SIOD_VDrive_02 = 0x72,     // SDrive virtual drive 2
    SIOD_VDrive_03 = 0x73,     // SDrive virtual drive 3
    SIOD_VDrive_04 = 0x74,     // SDrive virtual drive 4
    SIOD_VDrive_05 = 0x75,     // SDrive virtual drive 5
    SIOD_VDrive_06 = 0x76,     // SDrive virtual drive 6
    SIOD_VDrive_07 = 0x77,     // SDrive virtual drive 7
    SIOD_VDrive_08 = 0x78,     // SDrive virtual drive 8
    SIOD_VDrive_09 = 0x79,     // SDrive virtual drive 9
    SIOD_VDrive_10 = 0x7A,     // SDrive virtual drive 10
    SIOD_VDrive_11 = 0x7B,     // SDrive virtual drive 11
    SIOD_VDrive_12 = 0x7C,     // SDrive virtual drive 12
    SIOD_VDrive_13 = 0x7D,     // SDrive virtual drive 13
    SIOD_VDrive_14 = 0x7E,     // SDrive virtual drive 14
    SIOD_VDrive_15 = 0x7F      // SDrive virtual drive 15
 };

 enum ESIO_Drive_Commands
 {
    SIODC_Format = 0x21,        // '!' format disk
    SIODC_FormatMedium = 0x22,  // '""' format medium
    SIODC_GetHSIndex = 0x3F,    // '?' Get high speed index
    SIODC_ReadPercom = 0x4e,    // 'N' Read Percom block
    SIODC_WritePercom = 0x4f,   // 'O' Write Percom block    
    SIODC_Put = 0x50,           // 'P' write sector without verification (AUX1 - LSB, AUX2 - MSB of sector) 
    SIODC_Read = 0x52,          // 'R' read sector (AUX1 - LSB, AUX2 - MSB of sector) 
    SIODC_GetStatus = 0x53,     // 'S' get drive status
    SIODC_Write = 0x57,         // 'W' write sector with verification (AUX1 - LSB, AUX2 - MSB of sector) 

     
 };


 #endif