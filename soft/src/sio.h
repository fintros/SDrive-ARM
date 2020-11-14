/*
 * ---------------------------------------------------------------------------
 * -- (c) 2020 Alexey Spirkov
 * -- I am happy for anyone to use this for non-commercial use.
 * -- If my verilog/vhdl/c files are used commercially or otherwise sold,
 * -- please contact me for explicit permission at me _at_ alsp.net.
 * -- This applies for source and binary form and derived works.
 * ---------------------------------------------------------------------------
 */

#ifndef _SIO_H
#define _SIO_H

#define US_POKEY_DIV_MAX (255 - 6)
#define US_POKEY_DIV_STANDARD 0x28 //#40  => 19040 bps
#define ATARI_SPEED_STANDARD (US_POKEY_DIV_STANDARD + 6)
#define US_POKEY_DIV_DEFAULT 0x06 //#6   => 68838 bps

/**
 * @brief init USART with required baud
 * @param baud baud rate
 */
void USART_Set_Baud(unsigned int baud);

/**
 * @brief init USART with required SIO divider
 * @param div SIO divider
 */
void USART_Init(unsigned char div);

void USART_Transmit_Byte(unsigned char data);

unsigned char USART_Receive_Byte(void);

void USART_Send_Buffer(unsigned char *buff, unsigned short len);

unsigned char USART_Get_Buffer_And_Check(unsigned char *buff, unsigned short len, unsigned char cmd_state);

unsigned char USART_Get_buffer_and_check_and_send_ACK_or_NACK(unsigned char *buff, unsigned short len);

void USART_Send_status_and_buffer_and_check_sum(unsigned char *buff, unsigned short len, unsigned char is_error, unsigned char do_wait);

#define USART_Send_cmpl_and_buffer_and_check_sum(X, Y) USART_Send_status_and_buffer_and_check_sum(X, Y, 0, 1)
#define USART_Send_err_and_buffer_and_check_sum(X, Y) USART_Send_status_and_buffer_and_check_sum(X, Y, 1, 1)

#define send_ACK()            \
    USART_Transmit_Byte('A'); \
    dprint("Send ACK\r\n");
#define send_NACK()           \
    USART_Transmit_Byte('N'); \
    dprint("Send NACK\r\n");
#define send_CMPL()           \
    USART_Transmit_Byte('C'); \
    dprint("Send CMPL\r\n");
#define send_ERR()            \
    USART_Transmit_Byte('E'); \
    dprint("Send ERR\r\n");

#define USART_Get_atari_sector_buffer_and_check_and_send_ACK_or_NACK(len) USART_Get_buffer_and_check_and_send_ACK_or_NACK(atari_sector_buffer, len)

int USART_Set_Fast_IO(unsigned char divider);

#endif /* _SIO_H  */