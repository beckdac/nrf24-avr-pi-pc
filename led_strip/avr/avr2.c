#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usart.h"
#include "compatability.h"
#include "timer0.h"
#include "nRF24L01+.h"
#include "rgb.h"
#include "program.h"
#include "packet.h"

nrf24_t *radio;

const uint64_t pipes[2] = { PIPE_0_ADDR, PIPE_1_ADDR };
#define BUFLEN 32
char buf[BUFLEN];

int main(void) {
	// disable the watchdog timer, this is necessary if the timer was used to reset the device
	MCUSR &= ~(1<<WDRF);
	wdt_disable();

	usart_init(BAUD_TO_UBRR(USART_BAUD));
	printf_P(PSTR("hello!\n"));

	radio = nrf24_init();

	timer0_init();
	program_init();
	rgb_set(0, 0, 0);

	// turn on interrupts
	sei();

	nrf24_print_details(radio);

	nrf24_set_power_level(radio, NRF24_PA_LOW);
	nrf24_set_channel(radio, 100);
	nrf24_open_write_pipe(radio, pipes[1]);
	nrf24_open_read_pipe(radio, 1, pipes[0]);
	nrf24_enable_dynamic_payloads(radio);
	nrf24_set_auto_ack(radio, 1);
	nrf24_power_up(radio);
	nrf24_start_listening(radio);
	nrf24_print_details(radio);
	while (1) {
		if (nrf24_payload_available(radio)) {
			uint8_t done = 0;
			while (!done) {
				done = nrf24_read(radio, (uint8_t *)buf, BUFLEN);
				printf_P(PSTR("%2d bytes received: %s\n"), strlen(buf), buf);
			}
		}
	}
}
