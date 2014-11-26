#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"
#include "spi.h"

void spi_init(uint8_t mode, uint8_t dord, uint8_t clock) {
	uint8_t junk;

	/* setup pin directions for SPI pins */
	DDRB |= (1<<SPI_MOSI); // output
	DDRB &= ~(1<<SPI_MISO); // input
	DDRB |= (1<<SPI_SCK);// output
	DDRB |= (1<<SPI_SS);// output

	/* clear registers by fetching them */
	junk = SPSR;
	junk = SPDR;

	SPCR = (0 << SPIE) // interrupt disabled
		| (1 << SPE) // enable SPI
		| (dord << DORD) // LSB or MSB
		| (1 << MSTR) // master mode
		| (((mode & 0x02) == 2) << CPOL) // clock timing mode CPOL
		| (((mode & 0x01)) << CPHA) // clock timing mode CPHA
		| (((clock & 0x02) == 2) << SPR1) // cpu clock divisor SPR1
		| ((clock & 0x01) << SPR0); // cpu clock divisor SPR0
}

void spi_disable(void) {
	SPCR = 0;
}

uint8_t spi_send(uint8_t out) {
	SPDR = out;
	while (!(SPSR & (1<<SPIF)));
	return SPDR;
}

void spi_send_packet(int len, void *p) {
	uint8_t i, *b = (uint8_t *)p;

	if (SPCR & (1 << MSTR)) {
		/* send the bytes */
		for (i = 0; i < len; ++i) {
			SPDR = b[i];
			while (!(SPSR & (1 << SPIF)));
			b[i] = SPDR;
		}
	}
}
