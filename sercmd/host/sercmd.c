#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "sercmd.h"
#include "serial.h"
#include "error.h"

#define DEBUG
#undef DEBUG

#define B2BP "%1d%1d%1d%1d%1d%1d%1d%1d"
#define B2B(byte)  \
  (byte & 0x01 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x80 ? 1 : 0) 

static uint8_t _DDRA, _DDRB, _DDRD;
static uint8_t _PORTA, _PORTB, _PORTD;
static uint8_t _PINA, _PINB, _PIND;

void sercmd_init(char *serial_device, int baud) {
	uint8_t cmd, byte;

	serial_init(serial_device, baud);

#ifdef DEBUG
	printf("sending reset command\n");
#endif
	// reset and wait 50 ms
	cmd = SERCMD__RESET;
	serial_send(&cmd, 1);
	usleep(100000);

#ifdef DEBUG
	printf("test for echo\n");
#endif
	cmd = SERCMD__ECHO;
	serial_send(&cmd, 1);
	serial_recv(&byte, 1);
	if (byte != cmd)
		fatal_error("sercmd_init: test for echo failed!\n");

#ifdef DEBUG
	printf("syncing internal state to chip state\n");
#endif
	// get the state of the chip
	sercmd_sync();
}

void sercmd_sync(void) {
	// read all of the DDR
	_DDRA = sercmd_ddr_read(DDRA);
	_DDRB = sercmd_ddr_read(DDRB);
	_DDRD = sercmd_ddr_read(DDRD);

	// read all of the PORT values
	_PORTA = sercmd_port_read(PORTA);
	_PORTB = sercmd_port_read(PORTB);
	_PORTD = sercmd_port_read(PORTD);

	// read all of the PIN values
	_PINA = sercmd_pin_read(PINA);
	_PINB = sercmd_pin_read(PINB);
	_PIND = sercmd_pin_read(PIND);

#ifdef DEBUG
	printf("port A: DDRA 0b"B2BP"  PORTA 0b"B2BP"  PINA 0b"B2BP"\n", B2B(_DDRA), B2B(_PORTA), B2B(_PINA));
	printf("port B: DDRB 0b"B2BP"  PORTB 0b"B2BP"  PINB 0b"B2BP"\n", B2B(_DDRB), B2B(_PORTB), B2B(_PINB));
	printf("port D: DDRD 0b"B2BP"  PORTD 0b"B2BP"  PIND 0b"B2BP"\n", B2B(_DDRD), B2B(_PORTD), B2B(_PIND));
#endif
}

uint8_t sercmd_ddr_read(uint8_t port) {
	uint8_t cmd;
	switch (port) {
		case DDRA:
			cmd = SERCMD__READ_DDRA;
			break;
		case DDRB:
			cmd = SERCMD__READ_DDRB;
			break;
		case DDRD:
			cmd = SERCMD__READ_DDRD;
			break;
		default:
			warning("sercmd_ddr_read: unknown port specified (%d)\n", port);
			return 0;
			break;
	}
	serial_send(&cmd, 1);
	serial_recv(&cmd, 1);
	return cmd;
}

uint8_t sercmd_port_read(uint8_t port) {
	uint8_t cmd;
	switch (port) {
		case PORTA:
			cmd = SERCMD__READ_PORTA;
			break;
		case PORTB:
			cmd = SERCMD__READ_PORTB;
			break;
		case PORTD:
			cmd = SERCMD__READ_PORTD;
			break;
		default:
			warning("sercmd_port_read: unknown port specified (%d)\n", port);
			return 0;
			break;
	}
	serial_send(&cmd, 1);
	serial_recv(&cmd, 1);
	return cmd;
}

uint8_t sercmd_pin_read(uint8_t port) {
	uint8_t cmd;
	switch (port) {
		case PINA:
			cmd = SERCMD__READ_PINA;
			break;
		case PINB:
			cmd = SERCMD__READ_PINB;
			break;
		case PIND:
			cmd = SERCMD__READ_PIND;
			break;
		default:
			warning("sercmd_pin_read: unknown port specified (%d)\n", port);
			return 0;
			break;
	}
	serial_send(&cmd, 1);
	serial_recv(&cmd, 1);
	return cmd;
}

void sercmd_ddr_pin(uint8_t port, uint8_t pin, uint8_t dir) {
	uint8_t *ddr;
	uint8_t cmd2[2];

	switch (port) {
		case DDRA:
			ddr = &_DDRA;
			cmd2[0] = SERCMD__WRITE_DDRA;
			break;
		case DDRB:
			ddr = &_DDRB;
			cmd2[0] = SERCMD__WRITE_DDRB;
			break;
		case DDRD:
			ddr = &_DDRD;
			cmd2[0] = SERCMD__WRITE_DDRD;
			break;
		default:
			warning("sercmd_ddr: unknown port specified (%d)\n", port);
			return;
			break;
	}
	*ddr = (*ddr & ~(1 << pin)) | (dir << pin);
	cmd2[1] = *ddr;
	serial_send(cmd2, 2);
#ifdef DEBUG
	printf("sercmd_ddr_pin: cmd[0] = %d = 0b"B2BP"  |  cmd[1] = %d = 0b"B2BP"\n", cmd2[0], B2B(cmd2[0]), cmd2[1], B2B(cmd2[1]));
#endif
}

void sercmd_port_set(uint8_t port, uint8_t pin, uint8_t state) {
	uint8_t *_port;
	uint8_t cmd2[2];

	switch (port) {
		case PORTA:
			_port = &_PORTA;
			cmd2[0] = SERCMD__WRITE_PORTA;
			break;
		case PORTB:
			_port = &_PORTB;
			cmd2[0] = SERCMD__WRITE_PORTB;
			break;
		case PORTD:
			_port = &_PORTD;
			cmd2[0] = SERCMD__WRITE_PORTD;
			break;
		default:
			warning("sercmd_port_set: unknown port specified (%d)\n", port);
			return;
			break;
	}
	*_port = (*_port & ~(1 << pin)) | (state << pin);
	cmd2[1] = *_port;
	serial_send(cmd2, 2);
#ifdef DEBUG
	printf("sercmd_port_set: cmd[0] = %d = 0b"B2BP"  |  cmd[1] = %d = 0b"B2BP"\n", cmd2[0], B2B(cmd2[0]), cmd2[1], B2B(cmd2[1]));
#endif
}

uint8_t sercmd_spi_transfer(uint8_t *buf, uint8_t len) {
	uint8_t cmdbuf[len + 2];
	cmdbuf[0] = SERCMD__SPI;
	cmdbuf[1] = len;
	memcpy(&cmdbuf[2], buf, len);
#ifdef DEBUG
	printf("sercmd_spi_transfer: cmdbuf[0] = %d = 0b"B2BP"  |  cmdbuf[1] = %d = 0b"B2BP" AKA len\n", cmdbuf[0], B2B(cmdbuf[0]), cmdbuf[1], B2B(cmdbuf[1]));
#endif
	serial_send(cmdbuf, len + 2);
	return serial_recv(buf, len);
}

void sercmd_done(void) {
	serial_close();
}
