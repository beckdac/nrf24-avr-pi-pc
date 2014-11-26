#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include <stdio.h>
#include <stdlib.h>

#include "usart.h"
#include "compatability.h"
#include "timer0.h"
#include "nRF24L01+.h"
#include "rgb.h"
#include "program.h"

int main(void) {
	uint8_t i, j, k, prd[NRF24__MAX_CHANNEL + 1];
	nrf24_t *radio;

	// disable the watchdog timer, this is necessary if the timer was used to reset the device
	MCUSR &= ~(1<<WDRF);
	wdt_disable();

	usart_init(BAUD_TO_UBRR(USART_BAUD));
	printf_P(PSTR("RF channel scanner\n"));

	radio = nrf24_init();

	timer0_init();
	program_init();
	rgb_set(0, 0, 0);
	sei();

	nrf24_set_auto_ack(radio, 0);
	nrf24_start_listening(radio);
	_delay_ms(1);
	nrf24_stop_listening(radio);

	while (1) {
		nrf24_print_details(radio);

		// dump channel header
		for (i = 0; i <= NRF24__MAX_CHANNEL; ++i) {
			printf_P(PSTR("%x"), i >> 4);
			prd[i] = 0;
		}
		printf_P(PSTR("\n"));
		for (i = 0; i <= NRF24__MAX_CHANNEL; ++i)
			printf_P(PSTR("%x"), i&0xF);
		printf_P(PSTR("\n"));

		for (k = 0; k < 32; ++k) {
			for (j = 0; j <= NRF24__MAX_CHANNEL; ++j) {
				nrf24_set_channel(radio, j);
				nrf24_start_listening(radio);
				_delay_ms(2);
				nrf24_stop_listening(radio);
				if (nrf24_test_carrier(radio))
					prd[j]++;
			}
			for (j = 0; j <= NRF24__MAX_CHANNEL; ++j)
				printf_P(PSTR("%x"), (0xF < prd[j] ? 0xF : prd[j]));
			printf_P(PSTR("\n"));
		}
		_delay_ms(5000);
	}
}
