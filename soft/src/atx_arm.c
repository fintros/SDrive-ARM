/*! \file atx_avr.c \brief AVR-specific functions for ATX file handling. */
//*****************************************************************************
//
// File Name    : 'atx_avr.c'
// Title        : AVR-specific functions for ATX file handling
// Author       : Daniel Noguerol
// Date         : 21/01/2018
// Revised      : 21/01/2018
// Version      : 0.1
// Target MCU   : ???
// Editor Tabs  : 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//      which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include "project.h"
#include <CyLib.h>
#include "HWContext.h"
#include <stdlib.h>
#include "atx.h"
#include "fat.h"


void waitForAngularPosition(unsigned short pos) {
    // if the position is less than the current timer, we need to wait for a rollover 
    // to occur
    while (pos < SpinTimer_ReadCounter());
    // wait for the timer to reach the target position
    while (SpinTimer_ReadCounter() < pos);
}

unsigned short getCurrentHeadPosition() {
    // SpinTimer works on 125KHz. A full rotation of the disk is represented in an ATX file by 
    // an angular positional value between 1-26042 (or 8 microseconds based on 288 rpms). 
    // So, getCurrentHeadPosition always gives the current angular position of the drive head 
    // on the track any given time assuming the disk is spinning continously.
    return SpinTimer_ReadCounter();
}

void byteSwapAtxFileHeader(struct atxFileHeader * header) {
   // ARM implementation is a NO-OP
}

void byteSwapAtxTrackHeader(struct atxTrackHeader * header) {
   // ARM implementation is a NO-OP
}

void byteSwapAtxSectorListHeader(struct atxSectorListHeader * header) {
    // ARM implementation is a NO-OP
}

void byteSwapAtxSectorHeader(struct atxSectorHeader * header) {
    // ARM implementation is a NO-OP
}

void byteSwapAtxTrackChunk(struct atxTrackChunk *header) {
    // ARM implementation is a NO-OP
}

unsigned char is_1050() {
    return 0;
}
