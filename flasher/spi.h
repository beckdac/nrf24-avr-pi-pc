#ifndef __SPI_H__
#define __SPI_H__

#define GPIO_VALUE_LOW	0
#define GPIO_VALUE_HIGH	1

//define FT232R pins
#define PIN_MOSI	(1 << 0)	/* TXD, orange */
#define PIN_MISO	(1 << 1)	/* RXD, yellow */
#define PIN_SCK		(1 << 2)	/* RTS, green */
#define PIN_CSN		(1 << 3)	/* CTS, brown */
#define PINS_OUT	(PIN_CSN | PIN_SCK | PIN_MOSI)

#define BYTES_PER_BIT		3
#define FTDI_READ_FIFO_SIZE	384

void spi_init(void);
void spi_done(void);
int spi_send_packet(uint8_t *bytes, size_t size);
void spi_csn(uint8_t value);

#endif /* __SPI_H__ */
