#ifndef __AVR0_SPI_H__

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void spi_init(void);
uint8_t spi_transfer(uint8_t data);

#endif
