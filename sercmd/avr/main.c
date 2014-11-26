#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "usart.h"
#include "spi.h"
#include "sercmd.h"

#define LED_ON		PORTD |= (1 << PORTD5)
#define LED_OFF		PORTD &= ~(1 << PORTD5)
#define LED_BLINK	PORTD ^= (1 << PORTD5)

#define PROTECT_CORE
// protect core pins for SPI, the LED and serial port from DDR, PORT AND PIN changes
#ifndef PROTECT_CORE
#warning this code should probably protect some pins/ports from ddr, port and pin reads/writes, e.g. serial port, maybe spi and led?
#endif

// The lines that mention MCUSR_mirror are Michael Hennebry's 
// The rest, except this one are Clawson's 
unsigned char MCUSR_mirror __attribute__((section(".noinit"))); 
void early(void) __attribute__((section(".init3"), naked)); 
void early(void) {
	MCUSR_mirror=MCUSR; 
        // if the watchdog reset bit is set
        if (MCUSR & (1 << WDRF)) {
                // clear it
                MCUSR &= ~(1 << WDRF);
                // disable the watchdog so it doesn't go off during initialization
                wdt_disable();
	}
	LED_OFF;
}

void print_byte(uint8_t byte) {
	uint8_t i;
	for (i = 0; i < 8; ++i)
		if (byte & (1 << i))
			usart_putc('1');
		else
			usart_putc('0');
	usart_putc('\r');
	usart_putc('\n');
}

int main(void) {
	uint8_t cmd, byte, len;

	DDRD |= (1 << DDD5);
	LED_ON;

	init_usart();
	spi_init();

	sei();

#if 0
	while (1) {
		uint8_t c;
		c = usart_getc_wait();
		LED_BLINK;
		usart_putc(c);
	};
#elif 0
	usart_putc('h');
	usart_putc('a');
	usart_putc('i');
	usart_putc('!');
	usart_putc('\r');
	usart_putc('\n');
	while (1) {
		usart_putc('>');
		cmd = usart_getc_wait();
		usart_putc(cmd);
		usart_putc('\r');
		usart_putc('\n');
		if (cmd == 'r') {
			wdt_enable(WDTO_15MS);
			for (;;);
		} else if (cmd == 'a') {
			print_byte(DDRA);
			print_byte(PORTA);
			print_byte(PINA);
		} else if (cmd == 'b') {
			print_byte(DDRB);
			print_byte(PORTB);
			print_byte(PINB);
		} else if (cmd == 'd') {
			print_byte(DDRD);
			print_byte(PORTD);
			print_byte(PIND);
		}
	}
#endif

	while (1) {
		LED_ON;
		cmd = usart_getc_wait();
		LED_OFF;
		if (cmd == SERCMD__NOP) {
			wdt_reset();
		} else if (cmd == SERCMD__RESET) {
			wdt_enable(WDTO_15MS);
			for (;;);
		} else if (cmd == SERCMD__SPI) {
			wdt_reset();
			len = usart_getc_wait();
			do {
				byte = usart_getc_wait();
				byte = spi_transfer(byte);
				usart_putc(byte);
			} while (--len);
		} else if (cmd == SERCMD__ECHO) {
			wdt_reset();
			byte = cmd;
			usart_putc(byte);
		} else if (cmd >= SERCMD__READ_DDRA && cmd <= SERCMD__READ_PIND) {
			wdt_reset();
			// read operation
			switch (cmd) {
				case SERCMD__READ_DDRA:
					byte = DDRA;
					break;
				case SERCMD__READ_DDRB:
					byte = DDRB;
					break;
				case SERCMD__READ_DDRD:
					byte = DDRD;
					break;
				case SERCMD__READ_PORTA:
					byte = PORTA;
					break;
				case SERCMD__READ_PORTB:
					byte = PORTB;
					break;
				case SERCMD__READ_PORTD:
					byte = PORTD;
					break;
				case SERCMD__READ_PINA:
					byte = PINA;
					break;
				case SERCMD__READ_PINB:
					byte = PINB;
					break;
				case SERCMD__READ_PIND:
					byte = PIND;
					break;
			};
			// send back the data value
			usart_putc(byte);
		} else if (cmd >= SERCMD__WRITE_DDRA && cmd <= SERCMD__WRITE_PIND) {
			wdt_reset();
			// write operation
			// fetch the data value
			byte = usart_getc_wait();
			switch (cmd) {
				case SERCMD__WRITE_DDRA:
					DDRA = byte;
					break;
				case SERCMD__WRITE_DDRB:
					DDRB = byte;
					break;
				case SERCMD__WRITE_DDRD:
					DDRD = byte;
					break;
				case SERCMD__WRITE_PORTA:
					PORTA = byte;
					break;
				case SERCMD__WRITE_PORTB:
					PORTB = byte;
					break;
				case SERCMD__WRITE_PORTD:
					PORTD = byte;
					break;
				case SERCMD__WRITE_PINA:
					PINA = byte;
					break;
				case SERCMD__WRITE_PINB:
					PINB = byte;
					break;
				case SERCMD__WRITE_PIND:
					PIND = byte;
					break;
			};
		} else {
			// unhandled....
			// it would be nice to have an error LED that could turn on
			// or an out of band comm channel, maybe sending all 0xFFs until reset?
		}
	}
}
