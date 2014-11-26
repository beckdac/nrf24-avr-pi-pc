#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* adapted from http://arduino.cc/playground/Code/USI-SPI?action=sourceblock&num=4 */

#include "spi.h"

void spi_init(void) {
	/* set DO and USISCK as outputs (data out and clock) */
	DDRB |= (1 << PB6) | (1 << PB7);
	/* set DI (data in) as input and enable pull-up */
	DDRB &= ~(1 << PB5);
	PORTB |= (1 << PB5);
}

uint8_t spi_transfer(uint8_t data) {
	/* place data in data register */
	USIDR = data;

	/* clear bit */
	USISR = (1 << USIOIF);

	while ((USISR & (1 << USIOIF)) == 0 ) {
		/* USIWM0 is for three wire mode (DI/DO/USISCK) */
		/* USICS1 + UCSICS0 + USITC = external clock with negative edge and software strobe source */
		/* USITC drives the clock */
		USICR = (1<<USIWM0)|(1<<USICS1)|(1<<USICLK)|(1<<USITC);
	}

	return USIDR;
}
