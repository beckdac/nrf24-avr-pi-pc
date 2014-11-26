#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "error.h"
#include "serial.h"

static int serial_baud_to_constant(int baud_rate);

int fd;

int serial_set_attr(int baud, int parity) {
	struct termios tty;
	int speed = serial_baud_to_constant(baud);

	memset (&tty, 0, sizeof tty);

	if (tcgetattr (fd, &tty) != 0) {
		perror(NULL);
		warning("error in tcgetattr\n");
	      return -1;
	}

#ifdef __APPLE__
	// the B38400 is a bogus placeholder - and set the real rate below
	cfsetospeed (&tty, B38400);
	cfsetispeed (&tty, B38400);
	speed = 0;
#else
	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);
#endif

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // ignore break signal
	tty.c_lflag = 0;                // no signaling chars, no echo,
	                              // no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 1;            // read doesn't block
	tty.c_cc[VTIME] = 0;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	                              // enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	tty.c_cc[VMIN]  = 1;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		perror(NULL);
		warning("error in tcsetattr");
		return -1;
	}

#ifdef __APPLE__
#ifndef IOSSIOSPEED
#define IOSSIOSPEED    _IOW('T', 2, speed_t)
#endif
	speed_t realspeed = baud;
	if (ioctl(fd, IOSSIOSPEED, &realspeed) == -1) {
		perror(NULL);
		warning("ioctl error for OS/X specific baud rate handling\n");
	}
#endif

	return 0;
}

void serial_set_blocking(int should_block) {
	struct termios tty;

	memset (&tty, 0, sizeof tty);

	if (tcgetattr (fd, &tty) != 0) {
		perror(NULL);
		warning("error in tcgetattr\n");
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		warning("error in tcsetattr\n");
}

int serial_init(char *device, int baud) {
	fd = open (device, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		perror(NULL);
		warning("unable to open serial device: %s\n", device);
		return -1;
	}

	serial_set_attr(baud, 0);	// set speed to 500000 bps, 8n1 (no parity)
//	serial_set_blocking (1);		// set no blocking

#if 0
	while (1) {
		char c, d[100];
		for (c = 'A'; c <= 'Z'; ++c) {
			write(fd, &c, 1);
		}
		read(fd, d, 26);
		d[26] = '\0';
		printf("%s\n", d);
	}
#elif 0
{
	uint8_t in[100], out[100], i, n;
	for (i = 0; i < 26; ++i)
		out[i] = i + 'a';
	while (1) {
		write(fd, out, 26);
		n = read(fd, in, 26);
		in[n] = '\0';
		printf("%s\n", in);
	}
}
#elif 0
{
	uint8_t in[100], out[100], i, n;
	for (i = 0; i < 26; ++i)
		out[i] = i + 'a';
	while (1) {
		serial_send(out, 26);
		n = serial_recv(in, 26);
		in[n] = '\0';
		printf("%s\n", in);
	}
}
#endif
	return 1;
}

void serial_send(uint8_t *buff, uint8_t bytes) {
	/* send command */
	write(fd, buff, bytes);
}

uint8_t serial_recv(uint8_t *buff, uint8_t bytes) {
	int     i, r, bytes_read;

	/* clear buffer for return value */
	memset(buff, 0, bytes);

	i = 0;
	bytes_read = 0;
	while (bytes_read < bytes) {
		/* read response */
		r = read(fd, &buff[bytes_read], bytes - bytes_read);
		if (r < 0) {
			perror(NULL);
			fatal_error("serial_recv: read failed\n");
		}
#ifdef DEBUG
		warning("read #%d (%d bytes)", i, r);
#endif
		bytes_read += r;
		++i;
//		if (i >= 16)
//			break;
	}
#ifdef DEBUG
	if (i > 3 && i < 16)
		warning("serial_recv: required %d read() calls to read %d bytes", i + 1, bytes);
#endif
	if (i > 16)
		warning("serial_recv: bailed after 16 tries to read a packet");
	return bytes_read;
}

void serial_close(void) {
	close(fd);
}

int serial_flush(void) {
	return tcflush(fd, TCIFLUSH);
}

int serial_drain(void) {
	return tcdrain(fd);
}

static int serial_baud_to_constant(int baud_rate) {
	switch (baud_rate) {
		case 0: return B0;
		case 50: return B50;
		case 75: return B75;
		case 110: return B110;
		case 134: return B134;
		case 150: return B150;
		case 200: return B200;
		case 300: return B300;
		case 600: return B600;
		case 1200: return B1200;
		case 1800: return B1800;
		case 2400: return B2400;
		case 4800: return B4800;
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
#if defined(__linux__)
		case 230400: return B230400;
		case 460800: return B460800;
		case 500000: return B500000;
		case 576000: return B576000;
		case 921600: return B921600;
		case 1000000: return B1000000;
		case 1152000: return B1152000;
		case 1500000: return B1500000;
		case 2000000: return B2000000;
		case 2500000: return B2500000;
		case 3000000: return B3000000;
		case 3500000: return B3500000;
		case 4000000: return B4000000;
#endif
	}
	return -1;
}

