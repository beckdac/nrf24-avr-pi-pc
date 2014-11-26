#include <avr/io.h>

#include "spi.h"

void spi_init(uint8_t mode, uint8_t dord, uint8_t clock) {
	uint8_t junk;

	/* setup pin directions for SPI pins */
	SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS);
	SPI_DDR &= ~(1 << SPI_MISO); // input

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

uint8_t spi_transfer(const uint8_t out) {
	SPDR = out;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}
