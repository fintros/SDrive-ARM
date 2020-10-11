/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#include "hwcontext.h"
#include "project.h"
#include "sdrive.h"
#include <CyLib.h>
#include "dprint.h"

void waitForAngularPosition(u16 pos) {
    dprint("%d->%d\r\n", pos, SpinTimer_COUNTER_REG & SpinTimer_16BIT_MASK);
    while(pos < (SpinTimer_COUNTER_REG & SpinTimer_16BIT_MASK));
    while(pos > (SpinTimer_COUNTER_REG & SpinTimer_16BIT_MASK));
    //dprint("Exit at current %d\r\n", SpinTimer_COUNTER_REG & SpinTimer_16BIT_MASK);
}

u16 getCurrentHeadPosition() {
    // A full rotation of the disk is represented in an ATX file by an angular positional value 
    // between 1-26042 (or 8 microseconds based on 288 rpms). So, SpinTimer always gives the 
    // current angular position of the drive head on the track any given time assuming the 
    // disk is spinning continously.
    return SpinTimer_COUNTER_REG & SpinTimer_16BIT_MASK + 1;
}

u08 is_1050() {
    return(settings.is_1050);
}