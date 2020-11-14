/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _SLOWUART_H
#define _SLOWUART_H

/**
  * @brief Reset SlowUART sequence 
  */
void ResetSlowUART();

/**
  * @brief Decode captured Slow Uart packet
  * @param command target buffer
  * @max_len length of buffer
  * @return 0 if success and 1 otherwise
  */
int DecodeSlowUART(unsigned char *command, int max_len);

/**
  * @brief Stop SlowUART serving
  */
void StopSlowUART();

#endif /* _SLOWUART_H */
