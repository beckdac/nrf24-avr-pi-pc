#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "spi.h"
#include "nrf24.h"
#include "usart.h"

#define LED_ON	PORTD |= (1 << PORTD5)
#define LED_OFF	PORTD &= ~(1 << PORTD5)

int main(void) {
	DDRD |= (1 << DDD5);
	LED_ON;

	init_usart();
//	nrf24_init();

	sei();

	while (1) {
//		usart_putc(usart_getc_wait());
		usart_putc('.');
		usart_putc('\r');
		usart_putc('\n');
	}
}
