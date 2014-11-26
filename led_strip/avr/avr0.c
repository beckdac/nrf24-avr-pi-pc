#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
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
#include "timer1.h"

nrf24_t *radio;

const uint64_t pipes[2] = { PIPE_0_ADDR, PIPE_1_ADDR };
uint8_t packet[NRF24__MAX_PAYLOAD_SIZE];
extern volatile program_state_e program_state;
extern volatile uint16_t icp_hz;

int main(void) {
	// disable the watchdog timer, this is necessary if the timer was used to reset the device
	MCUSR &= ~(1<<WDRF);
	wdt_disable();

	usart_init(BAUD_TO_UBRR(USART_BAUD));
	printf_P(PSTR("hello!\n"));

	radio = nrf24_init();

	nrf24_set_power_level(radio, NRF24_PA_LOW);
	nrf24_set_channel(radio, 100);
	nrf24_open_write_pipe(radio, pipes[1]);
	nrf24_open_read_pipe(radio, 1, pipes[0]);
	nrf24_enable_dynamic_payloads(radio);
	nrf24_set_auto_ack(radio, 1);
	nrf24_enable_ack_payload(radio);
	nrf24_power_up(radio);
	nrf24_start_listening(radio);

	// nrf24_init already sets the IRQ pin (PD2) as an input
	EICRA |= (1 << ISC01);		 	// trigger on falling edge
//	EICRA = 0;				// the low level of INT0 triggers interrupt
	EIMSK |= (1 << INT0);			// enable INT0 interrupt (PD2)

	// led program execution
	timer0_init();
	program_init();
	rgb_set(0, 0, 0);

	// TSL230R light to frequency chip
	timer1_init();

	// turn on interrupts
	sei();

	nrf24_print_details(radio);

	while (1) {
		program_run();
	}
}

// interrupt for nrf24 IRQ pin (PD2)
ISR(INT0_vect) {
	// incoming data
	uint8_t done = 0;
	uint8_t status = nrf24_read_status(radio);

	while (!done) {
		done = nrf24_read(radio, packet, NRF24__MAX_PAYLOAD_SIZE);

#ifdef AVR0_DEBUG
		printf_P(PSTR("packet type = %d\n"), packet[0]);
#endif

		switch (packet[0]) {
			case PACKET_RESET:
				// call the watchdog timer to reset in 15ms
				wdt_enable(WDTO_15MS);
				// wait 20ms, effectively forcing a reset
				_delay_ms(20);
				break;
			case PACKET_STOP_PROGRAM:
				program_state = PROGRAM_STOP;
				break;
			case PACKET_RUN_PROGRAM:
				program_state = PROGRAM_RUN;
				break;
			case PACKET_SET_COLOR:
				rgb_set(packet[1], packet[2], packet[3]);
				break;
			case PACKET_BEGIN_PROGRAMMING:
				program_state = PROGRAM_PROGRAMMING;
				printf_P(PSTR("entering programming state\n"));
				break;
			case PACKET_PROGRAM_LENGTH:
				if (program_state == PROGRAM_PROGRAMMING) {
					uint16_t location = PROGRAM_START, steps = ((uint16_t)packet[2] << 8) | packet[1];
					printf_P(PSTR("program steps = %" PRIu16 "\n"), steps);
					eeprom_update_word((uint16_t *)location, steps);
				}
				break;
			case PACKET_PROGRAM_STEP:
				if (program_state == PROGRAM_PROGRAMMING) {
					uint16_t location;
					uint16_t step = ((uint16_t)packet[2] << 8) | packet[1];
					uint16_t delay_in_ms = ((uint16_t)packet[7] << 8) | packet[6];
//					printf_P(PSTR("program step  = %" PRIu16 "\n"), step);
//					printf_P(PSTR("program rgb   = %u\t%u\t%u\n"), packet[3], packet[4], packet[5]);
//					printf_P(PSTR("program delay = %" PRIu16 "\n"), step);
					printf_P(PSTR("."));
					location = PROGRAM_STEP_LOCATION(step);
					eeprom_update_block(&packet[3], (void *)location, 3 * sizeof(uint8_t));
					location += 3 * sizeof(uint8_t);
					eeprom_update_word((uint16_t *)location, delay_in_ms);
				}
				break;
			case PACKET_END_PROGRAMMING:
				if (program_state == PROGRAM_PROGRAMMING) {
					program_state = PROGRAM_STOP;
					printf_P(PSTR("\ndone\n"));
				}
				break;
			case PACKET_READ_PROGRAM_LEN:
				{
					uint16_t location = PROGRAM_START;
					uint16_t steps;

					packet[0] = PACKET_PROGRAM_LENGTH;
					steps = eeprom_read_word((uint16_t *)location);
					packet[1] = (uint8_t)(steps & 0x00FF);
					packet[2] = (uint8_t)((steps & 0xFF00) >> 8);			

#ifdef AVR0_DEBUG
					printf_P(PSTR("sending stored program length as next ack packet: %" PRIu16 "\n"), steps);
#endif

					nrf24_set_payload_size(radio, PACKET_PROGRAM_LENGTH_SIZE);
					nrf24_write_ack_payload(radio, 1, packet, PACKET_PROGRAM_LENGTH_SIZE);
				}
				break;
			case PACKET_READ_PROGRAM_LEN_FLUSH:
				// nothing really to do here, we let the auto ack do the heavy lifting
				break;
			case PACKET_READ_PROGRAM_STEP:
				{
					uint16_t location;
					uint16_t step, delay_in_ms;

					packet[0] = PACKET_PROGRAM_STEP;
					step = ((uint16_t)packet[2] << 8) | packet[1];

#ifdef AVR0_DEBUG
					printf_P(PSTR("sending step %" PRIu16 "\n"), step);
#endif

					packet[1] = (uint8_t)(step & 0x00FF);
					packet[2] = (uint8_t)((step & 0xFF00) >> 8);			
					location = PROGRAM_STEP_LOCATION(step);
					eeprom_read_block(&packet[3], (void *)location, 3 * sizeof(uint8_t));
					location += 3 * sizeof(uint8_t);
					delay_in_ms = eeprom_read_word((uint16_t *)location);
					packet[6] = (uint8_t)(delay_in_ms & 0x00FF);
					packet[7] = (uint8_t)((delay_in_ms & 0xFF00) >> 8);

#ifdef AVR0_DEBUG
					printf_P(PSTR("sending stored program step %" PRIu16 ": %d %d %d - %" PRIu16 " ms\n"), step, packet[3], packet[4], packet[5], delay_in_ms);
#endif

					nrf24_set_payload_size(radio, PACKET_PROGRAM_STEP_SIZE);
					nrf24_write_ack_payload(radio, 1, packet, PACKET_PROGRAM_STEP_SIZE);
				}
				break;
			case PACKET_READ_PROGRAM_STEP_FLUSH:
				// nothing really to do here, we let the auto ack do the heavy lifting
				break;
			case PACKET_READ_LIGHT_FREQ:
				{
					uint16_t hz = icp_hz;
#ifdef AVR0_DEBUG
					printf_P(PSTR("TSL230R freq is %"  PRIu16 " Hz\n"), hz);
#endif

					packet[0] = PACKET_LIGHT_FREQ;
					packet[1] = (uint8_t)(hz & 0x00FF);
					packet[2] = (uint8_t)((hz & 0xFF00) >> 8);			

					nrf24_set_payload_size(radio, PACKET_LIGHT_FREQ_SIZE);
					nrf24_write_ack_payload(radio, 1, packet, PACKET_LIGHT_FREQ_SIZE);
				}
				break;
			case PACKET_READ_LIGHT_FREQ_FLUSH:
				// nothing really to do here, we let the auto ack do the heavy lifting
				break;
			default:
				// unknown packet type - ignore
				printf_P(PSTR("unknown packet type received (%d), ignoring\n"), packet[0]);
				break;
		};
	}

	// reset IRQ status bit
	status |= _BV(NRF24__RX_DR) | _BV(NRF24__TX_DS) | _BV(NRF24__MAX_RT);
	nrf24_write_register(radio, NRF24__STATUS, &status, 1);
}
