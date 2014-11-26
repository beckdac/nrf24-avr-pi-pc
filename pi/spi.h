#ifndef __SPI_H__
#define __SPI_H__

#include <linux/spi/spidev.h>

typedef struct spi {
	int fd;
	uint8_t mode;
	uint8_t bits_per_word;
	uint32_t speed;
} spi_t;

// device
#define SPI_CS0 0
#define SPI_CS1 1
// 0=CS0 1=CS1

// mode
// SPI_MODE_0 (0,0)  CPOL=0 (Clock Idle low level), CPHA=0 (SDO transmit/change edge active to idle)
// SPI_MODE_1 (0,1)  CPOL=0 (Clock Idle low level), CPHA=1 (SDO transmit/change edge idle to active)
// SPI_MODE_2 (1,0)  CPOL=1 (Clock Idle high level), CPHA=0 (SDO transmit/change edge active to idle)
// SPI_MODE_3 (1,1)  CPOL=1 (Clock Idle high level), CPHA=1 (SDO transmit/change edge idle to active)

// bpw
#define SPI_BPW_8 8
#define SPI_BPW_9 9

// speed
#define SPI_1MHZ 1000000
#define SPI_2MHZ 2000000
#define SPI_8MHZ 8000000

// filename
#define SPI_DEV0 "/dev/spidev0.0"
#define SPI_DEV1 "/dev/spidev0.1"

void spi_init(uint8_t device, uint8_t mode, uint8_t bits_per_word, uint32_t speed);
int spi_transfer(uint8_t device, uint8_t *data, int len);
void spi_done(uint8_t device);

#endif /* __SPI_H__ */
