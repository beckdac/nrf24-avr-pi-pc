#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ftdi.h>
#include <libusb.h>

#include "error.h"
#include "spi.h"

static struct ftdi_context *ftdi;
static uint8_t pin_state = 0;

void spi_init(void) {
	int ret;

	ftdi = ftdi_new();
	if (!ftdi) {
		fatal_error("ftdi_new failed!\n");
	}

	ret = ftdi_usb_open(ftdi, 0x0403, 0x6001);

	if (ret < 0 && ret != -5) {
		ftdi_free(ftdi);
		fatal_error("unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
	}

	ret = ftdi_set_bitmode(ftdi, PINS_OUT, BITMODE_SYNCBB);
	if (ret != 0) {
		ftdi_free(ftdi);
		fatal_error("unable to set bitmode: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
	}

	ret = ftdi_set_baudrate(ftdi, 57600);
	if (ret != 0) {
		ftdi_disable_bitbang(ftdi);
		ftdi_free(ftdi);
		fatal_error("unable to set baudrate: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
	}
}

void spi_done(void) {
	ftdi_disable_bitbang(ftdi);
	ftdi_free(ftdi);
}

void spi_csn(uint8_t value) {
        uint8_t v;

        if (value)
                pin_state |= PIN_CSN;
        else
                pin_state &= ~PIN_CSN;

        ftdi_write_data(ftdi, &pin_state, sizeof(pin_state));
        ftdi_read_data(ftdi, &v, sizeof(v));
}

static int spi_buf_w(const uint8_t *b, size_t s) {
	const size_t total_size = s * 8 * BYTES_PER_BIT;
	int j = 0, pos;
	uint8_t *buf = calloc(1, total_size);

	for (pos = 0; pos < s; pos++) {
		uint8_t bit;

		// most significant bit first
		for (bit = (1 << 7); bit > 0; bit >>= 1) {
			if (b[pos] & bit) {
				pin_state |= PIN_MOSI;
				buf[j++] = pin_state;
			} else {
				pin_state &= ~PIN_MOSI;
				buf[j++] = pin_state;
			}
			pin_state |= PIN_SCK;
			buf[j++] = pin_state;

			pin_state &= ~PIN_SCK;
			buf[j++] = pin_state;
		}
	}
	j = ftdi_write_data(ftdi, buf, j);
	free(buf);

	return j / 8 / BYTES_PER_BIT;
}

static int spi_buf_r(uint8_t *b, size_t s) {
	const size_t total_size = s * 8 * BYTES_PER_BIT;
	int j = 0, pos;
	uint8_t *buf = calloc(1, total_size);

	if (ftdi_read_data(ftdi, buf, total_size) != total_size) {
		fprintf(stderr, "problem reading device\n");
		free(buf);
		return 0;
	}

	for (pos = 0; pos < s; pos++) {
		uint8_t v = 0;
		uint8_t bit;

		// most significant bit first
		for (bit = (1 << 7); bit > 0; bit >>= 1) {
			j += (BYTES_PER_BIT + 1) / 2;
			if (buf[j++] & PIN_MISO)
				v |= bit;
		}

		b[pos] = v;
	}

	free(buf);
	return j / 8 / BYTES_PER_BIT;
}

int spi_send_packet(uint8_t *bytes, size_t size) {
	int pos;
	int sz = FTDI_READ_FIFO_SIZE / 8 / BYTES_PER_BIT;

	for (pos = 0; pos < size; pos += sz) {
		int cksize = ((size - pos) < sz) ? (size - pos) : sz;

		if (spi_buf_w(bytes + pos, cksize) != cksize) {
			warning("error writing spi\n");
			return pos;
		}

		if (spi_buf_r(bytes + pos, cksize) != cksize) {
			warning("error reading spi\n");
			return pos;
		}
	}

	return size;
}
