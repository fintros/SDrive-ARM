/*! \file mmc.c \brief MultiMedia and SD Flash Card Interface. */
//*****************************************************************************
//
// File Name	: 'mmc.c'
// Title		: MultiMedia and SD Flash Card Interface
// Author		: Pascal Stang - Copyright (C) 2004
// Created		: 2004.09.22
// Revised		: 2004.09.22
// Version		: 0.1
// Target MCU	: Atmel AVR Series
// Editor Tabs	: 4
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

//----- Include Files ---------------------------------------------------------
#include <avr/io.h>			// include I/O definitions (port names, pin names, etc)
#include <avr/signal.h>		// include "signal" names (interrupt names)
#include <avr/interrupt.h>	// include interrupt support

#include "avrlibdefs.h"			// global AVRLIB defines
#include "avrlibtypes.h"		// global AVRLIB types definitions
#include "spi.h"		// include spi bus support


//#define spiTransferTwoFF()	{ spiTransferFF(); spiTransferFF(); }


#include "mmc.h"

// include project-specific hardware configuration
#include "mmcconf.h"


void mmcInit(void)
{
	// initialize SPI interface
	spiInit();
	// release chip select
	sbi(MMC_CS_DDR, MMC_CS_PIN);
	sbi(MMC_CS_PORT,MMC_CS_PIN);
}

extern u32 n_actual_mmc_sector;
extern u08 n_actual_mmc_sector_needswrite;

u08 mmcReset(void)
{
	u16 retry;
	u08 r1;
	u08 i;
	u08 sdcard;

	n_actual_mmc_sector=0xFFFFFFFF;	//!!! pridano dovnitr
	n_actual_mmc_sector_needswrite=0;

	//
	i=0xff; //255x
	do { spiTransferFF(); i--; } while(i);	//255x

	//
	r1 = mmcSendCommand(MMC_GO_IDLE_STATE, 0);
	if (r1!=1) return -1;

	retry=0xffff; //65535x
	do
	{
		r1 = mmcSendCommand(55, 0);
		if (r1!=1) break; //MMC
		//if (r1==5) break; //MMC  <--- kontrolovat takhle rekl Bob 27.6.2008
								//Strana 120 ve specifikaci.
								//bit 0 - in idle state, bit 2 - illegal command

		r1 = mmcSendCommand(41, 0);
		if (r1==0) break; //SD

		retry--;
	}
	while(retry);

	// initializing card for operation
	r1 = mmcSendCommand(MMC_SEND_OP_COND, 0);
	// SDkarty vraceji 0
	// MMCkarta vraci 1
	//if (r1>=2) return -1; //problem (MMC taky nefunguje)
	if (r1!=0) return -1;

	//kdyz jsem to tady nechal jit s MMC kartou dal (s r1=1)
	//tak to na nekterem z dalsich povelu zustalo zasekle
	//a po vytazeni MMC a vlozeni jine SD se muset mackat Reset :-(

	/*
	if( r1!=0  &&  r1!=5)
	{
		u08 v;
		v=inb(PINC)&0xf0;
		v|=(0x0f ^ (r1 & 0x0f) );
		PORTC=v;
	 	while(1); //nekonecna smycka
	 return -1;
	}
	*/

	// turn off CRC checking to simplify communication
	//r1 = 
	mmcSendCommand(MMC_CRC_ON_OFF, 0);

	// set block length to 512 bytes
	//r1 = 
	mmcSendCommand(MMC_SET_BLOCKLEN, 512);

	// return success
	return 0;
}

u08 mmcSendCommand(u08 cmd, u32 arg)
{
	u08 r1;

	// assert chip select
	cbi(MMC_CS_PORT,MMC_CS_PIN);
	//
	//spiTransferFF();			//pribude 34 bajtu kodu (!)
	// issue the command
	r1 = mmcCommand(cmd, arg);
	//
	//spiTransferFF();
	// release chip select
	sbi(MMC_CS_PORT,MMC_CS_PIN);

	return r1;
}

extern unsigned char mmc_sector_buffer[512];

u08 mmcRead(u32 sector)
{
	u08 r1;
	u16 i;
	u08 *buffer=mmc_sector_buffer;	//natvrdo!

	// assert chip select
	cbi(MMC_CS_PORT,MMC_CS_PIN);
	// issue command
	r1 = mmcCommand(MMC_READ_SINGLE_BLOCK, sector<<9);

	// check for valid response
	if(r1 != 0x00) return r1;

	// wait for block start
	while(spiTransferFF() != MMC_STARTBLOCK_READ);

	//zacatek bloku

	//nacti 512 bytu
	i=0x200;	//512
	do { *buffer++ = spiTransferFF(); i--; } while(i);

	// read 16-bit CRC
	//2x FF:
	spiTransferFF();
	spiTransferFF();
	//// + 1x FF navic pred releasnutim chip selectu
	//spiTransferFF();
	//
	// release chip select
	sbi(MMC_CS_PORT,MMC_CS_PIN);
	//
	return 0;	//success
}

u08 mmcWrite(u32 sector)
{
	u08 r1;
	u16 i;
	u08 *buffer=mmc_sector_buffer;	//natvrdo!

	// assert chip select
	cbi(MMC_CS_PORT,MMC_CS_PIN);
	// issue command
	r1 = mmcCommand(MMC_WRITE_BLOCK, sector<<9);
	// check for valid response
	if(r1 != 0x00)
		return r1;
	// send dummy
	spiTransferFF();
	// send data start token
	spiTransferByte(MMC_STARTBLOCK_WRITE);

	// write data (512 bytes)
	i=0x200;
	do { spiTransferByte(*buffer++); i--; }	while(i);

	// write 16-bit CRC (dummy values)
	//2x FF:
	spiTransferFF();
	spiTransferFF();
	// read data response token
	r1 = spiTransferFF();
	if( (r1&MMC_DR_MASK) != MMC_DR_ACCEPT)
		return r1;
	// wait until card not busy
	while(!spiTransferFF());
	// release chip select
	sbi(MMC_CS_PORT,MMC_CS_PIN);
	// return success
	return 0;
}

u08 mmcCommand(u08 cmd, u32 arg)
{
	u08 r1;
	u08 retry=0xff;	//255x
	//

	spiTransferFF();

	spiTransferFF();	//pridano navic! 27.6.2008 doporucil Bob!k
	spiTransferFF();	//pridano navic! 27.6.2008 doporucil Bob!k
	spiTransferFF();	//pridano navic! 27.6.2008 doporucil Bob!k

	// send command
	spiTransferByte(cmd | 0x40);
	//
	spiTransferByte(arg>>24);
	spiTransferByte(arg>>16);
	spiTransferByte(arg>>8);
	spiTransferByte(arg);
	//Alternativni zpusob pres ((u08*)&arg)[] je delsi.

	spiTransferByte(0x95);	// crc valid only for MMC_GO_IDLE_STATE
	// end command
	// wait for response
	// if more than 255 retries, card has timed-out
	// return the received 0xFF
	do
	{
		r1 = spiTransferFF();
		retry--;
	}
	while(retry && (r1 == 0xFF) );

	// return response
	return r1;
}
