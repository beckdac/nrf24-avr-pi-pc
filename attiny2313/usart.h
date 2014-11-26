#ifndef USART_H_
#define USART_H_

#include <stdio.h>

#define USART_NO_DATA 0x0100

void init_usart(void);

// Send and receive functions, that run without ISRs
uint8_t receive_usart();
void send_usart(uint8_t c);

// Receive a single char or USART_NO_DATA, if nothing received
uint16_t usart_getc(void);
// Blocking call to receive a char
uint8_t usart_getc_wait(void);

// Send a single char
void usart_putc(uint8_t c);

#endif /*USART_H_*/
