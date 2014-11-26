#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "spi.h"

spi_t spi[2];

void spi_init(uint8_t device, uint8_t mode, uint8_t bits_per_word, uint32_t speed) {
	int rv;

	// force correction for invalid device
	if (device) device = 1;

	// clear memory
	memset(&spi[device], 0, sizeof(spi_t));

	spi[device].mode = mode;
	spi[device].bits_per_word = bits_per_word;
	spi[device].speed = speed;

	printf("initializing SPI device '%s' in mode %d w/ %d bit/word @ speed %d\n", (device ? SPI_DEV1 : SPI_DEV0), spi[device].mode, spi[device].bits_per_word, spi[device].speed);

	// set parameters
	// open device
	spi[device].fd = open((device ? SPI_DEV1 : SPI_DEV0), O_RDWR);
	if (spi[device].fd < 0) {
		perror(NULL);
		fatal_error("unable to open SPI device '%s'\n", (device ? SPI_DEV1 : SPI_DEV0));
	}

	// setup
	// mode (write)
	rv = ioctl(spi[device].fd, SPI_IOC_WR_MODE, &spi[device].mode);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to set SPI mode for write operations for device '%s'\n", (device ? SPI_DEV1 : SPI_DEV0));
	}
	// mode (read)
	rv = ioctl(spi[device].fd, SPI_IOC_RD_MODE, &spi[device].mode);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to set SPI mode for read operations for device '%s'\n", (device ? SPI_DEV1 : SPI_DEV0));
	}
	// bpw (write)
	rv = ioctl(spi[device].fd, SPI_IOC_WR_BITS_PER_WORD, &spi[device].bits_per_word);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to set bits per word for write operations for device '%s'\n", (device ? SPI_DEV1 : SPI_DEV0));
	}
	// bpw (read)
	rv = ioctl(spi[device].fd, SPI_IOC_RD_BITS_PER_WORD, &spi[device].bits_per_word);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to set bits per word for read operations for device '%s'\n", (device ? SPI_DEV1 : SPI_DEV0));
	}
	// speed (write)
	rv = ioctl(spi[device].fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi[device].speed);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to set the clock speed for write operations (value = %u) for device '%s'\n", spi[device].speed, (device ? SPI_DEV1 : SPI_DEV0));
	}
	// speed (read)
	rv = ioctl(spi[device].fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi[device].speed);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to set the clock speed for read operations (value = %u) for device '%s'\n", spi[device].speed, (device ? SPI_DEV1 : SPI_DEV0));
	}
}

int spi_transfer(uint8_t device, uint8_t *data, int len) {
#if 0
	struct spi_ioc_transfer spi_data[len];
	int i;
	int rv;

	// force correction for invalid device
	if (device) device = 1;

	for (i = 0 ; i < len; i++) {
		spi_data[i].tx_buf = (unsigned long)(data + i);
		spi_data[i].rx_buf = (unsigned long)(data + i);
		spi_data[i].len = sizeof(*(data + i)) ;
		spi_data[i].delay_usecs = 0 ;
		spi_data[i].speed_hz = spi[device].speed ;
		spi_data[i].bits_per_word = spi[device].bits_per_word;
		spi_data[i].cs_change = 0;
	}

	// do transfer with ioctl call
	rv = ioctl(spi[device].fd, SPI_IOC_MESSAGE(len), &spi_data);
#else
	// this method uses 1 ioctl transmission entity instead of len entities
	struct spi_ioc_transfer spi_data;
	int rv;

	spi_data.tx_buf = (unsigned long)data;
	spi_data.rx_buf = (unsigned long)data;
	spi_data.len = len;
	spi_data.delay_usecs = 0 ;
	spi_data.speed_hz = spi[device].speed ;
	spi_data.bits_per_word = spi[device].bits_per_word;
	spi_data.cs_change = 0;

	// do transfer with ioctl call
	rv = ioctl(spi[device].fd, SPI_IOC_MESSAGE(1), &spi_data);
#endif
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to transfer %d byte message on device '%s'\n", len, (device ? SPI_DEV1 : SPI_DEV0));
	}

	return rv;
}

void spi_done(uint8_t device) {
	int rv;

	// force correction for invalid device
	if (device) device = 1;

	rv = close(spi[device].fd);
	if (rv < 0) {
		perror(NULL);
		fatal_error("unable to close SPI device '%s'\n", (device ? SPI_DEV1 : SPI_DEV0));
	}
}
