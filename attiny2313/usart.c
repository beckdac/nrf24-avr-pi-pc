#include <inttypes.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usart.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0 
#endif

#define UBBR16		((F_CPU / (BAUD * 16L)) - 1)
#define BUFFER_SIZE	16

volatile static uint8_t rx_buffer[BUFFER_SIZE];
volatile static uint8_t tx_buffer[BUFFER_SIZE];
volatile static uint8_t rx_head = 0;
volatile static uint8_t rx_tail = 0;
volatile static uint8_t tx_head = 0;
volatile static uint8_t tx_tail = 0;
volatile static uint8_t sent = TRUE;

/*
 * init_usart
 */
void init_usart(void) {
	// set baud rate
	UBRRH = (uint8_t)(UBBR16 >> 8); 
	UBRRL = (uint8_t)(UBBR16);
	// enable receive and transmit
	UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
	// set frame format
	UCSRC = (1 << USBS) | (3 << UCSZ0);	// asynchron 8n1
}

/*
 * send_usart
 * Sends a single char to USART without ISR
 */
void send_usart(uint8_t c) {
	// wait for empty data register
	while (!(UCSRA & (1<<UDRE)));
	// set data into data register
	UDR = c;
}

/*
 * receive_usart
 * Receives a single char without ISR
 */
uint8_t receive_usart() {
	while ( !(UCSRA & (1<<RXC)) ) 
		; 
	return UDR; 
}

/*
 * usart_getc
 * Gets a single char from the receive buffer.
 * return	uint16_r	the received char or USART_NO_DATA 
 */
uint16_t usart_getc(void) {
	uint8_t c = 0;
	uint8_t tmp_tail = 0;
	if (rx_head == rx_tail) {
		return USART_NO_DATA;
	}
	tmp_tail = (rx_tail + 1) % BUFFER_SIZE;
	c = rx_buffer[rx_tail];
	rx_tail = tmp_tail;
	return c;
}

/*
 * usart_getc_wait
 * Blocking call to getc. Will not return until a char is received.
 */
uint8_t usart_getc_wait(void) {
	uint16_t c;
	while ((c = usart_getc()) == USART_NO_DATA) {}
	return c;
}

/*
 * usart_putc
 * Puts a single char. Will block until there is enough space in the
 * send buffer.
 */
void usart_putc(uint8_t c) {
	uint8_t tmp_head = (tx_head + 1) % BUFFER_SIZE;
	// wait for space in buffer
	while (tmp_head == tx_tail) {
		;
	}
	tx_buffer[tx_head] = c;
	tx_head = tmp_head;
	// enable usart data interrupt (send data)
	UCSRB |= (1<<UDRIE);
}

/*
 * ISR User Data Regiser Empty
 * Send a char out of buffer via USART. If sending is complete, the 
 * interrupt gets disabled.
 */
ISR(USART_UDRE_vect) {
	uint8_t tmp_tail = 0;
	if (tx_head != tx_tail) {
		tmp_tail = (tx_tail + 1) % BUFFER_SIZE;
		UDR = tx_buffer[tx_tail];
		tx_tail = tmp_tail;
	}
	else {
		// disable this interrupt if nothing more to send
		UCSRB &= ~(1 << UDRIE);
	}
}

/*
 * ISR RX complete
 * Receives a char from USART and stores it in ring buffer.
 */
ISR(USART_RX_vect) {
	uint8_t tmp_head = 0;
	tmp_head = (rx_head + 1) % BUFFER_SIZE;
	if (tmp_head == rx_tail) {
		// buffer overflow error!
	}
	else {
		rx_buffer[rx_head] = UDR;
		rx_head = tmp_head;
	}
}
