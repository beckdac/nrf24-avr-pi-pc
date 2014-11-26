#ifndef _SPI_H_
#define _SPI_H_

#include <avr/io.h>
#include "circbuf.h"

/* port assignment for chips */
#if (defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__))
        #define SPI_SS PORTB0
        #define SPI_SCK PORTB1
        #define SPI_MOSI PORTB2
        #define SPI_MISO PORTB3
#elif (defined(__AVR_ATmega48__) || defined(_AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__))
        #define SPI_SS PORTB2
        #define SPI_SCK PORTB5
        #define SPI_MOSI PORTB3
        #define SPI_MISO PORTB4
#elif defined(__AVR_ATmega644P__)
        #define SPI_SS PORTB4
        #define SPI_MOSI PORTB5
        #define SPI_MISO PORTB6
        #define SPI_SCK PORTB7
#else
        #error avr not defined in spi.h! add spi port assignments
#endif


/* SPI data modes 					leading edge		trailing edge	*/
#define SPI_MODE_0	0x00	/* CPOL=0, CPHA=0	sample (rising)		setup (falling)	*/
#define SPI_MODE_1	0x01	/* CPOL=0, CPHA=1 	setup (rising)		sample (falling)*/
#define SPI_MODE_2	0x02	/* CPOL=1, CPHA=0 	sample (falling)	setup (rising)	*/
#define SPI_MODE_3	0x03	/* CPOL=1, CPHA=1 	setup (faling)		sample (rising)	*/

/* shift register loading order */
#define SPI_MSB 0
#define SPI_LSB 1

/* master or slave clock */
#define SPI_MASTER_CLOCK_DIV_2	0x04
#define SPI_MASTER_CLOCK_DIV_4	0x00
#define SPI_MASTER_CLOCK_DIV_8	0x05
#define SPI_MASTER_CLOCK_DIV_16	0x01
#define SPI_MASTER_CLOCK_DIV_64	0x02
#define SPI_MASTER_CLOCK_DIV_32	0x06
#define SPI_MASTER_CLOCK_DIV_128 0x03

// spi_setup can be called over and over to change (for example) the interupt or master slave
void spi_init(uint8_t mode,   // timing mode SPI_MODE[0-4]
	      uint8_t msb_lsb,
	      uint8_t clock); // clock diviser

// disable spi
void spi_disable(void);

// send and receive a byte of data (master mode)
uint8_t spi_send(uint8_t out);

void spi_send_packet(int len, void *p);

#endif /* _SPI_H_ */
