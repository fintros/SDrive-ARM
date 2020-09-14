/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */
#ifndef _HELPERS_H
#define _HELPERS_H

unsigned char get_checksum(unsigned char* buffer, unsigned short len);

void set_display(unsigned char n);

#endif /* _HELPERS_H */