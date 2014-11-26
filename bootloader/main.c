#include <inttypes.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "nrf24.h"
#include "flash.h"
#include "nRF24L01.h"

// this is a way of knowing if the bootloader was started by:
// 1. a watchdog reset from the application, or
// 2. a watchdog reset from within the bootloader
#define BOOTLOADER_KEY	0x8576
uint16_t reset_key __attribute__ ((section (".noinit")));

// check to see if the bootloader was started by a watchdog reset from within the bootloader
// put this function in init3 so that it runs before main and after zero init but it will go early
// enough to circumvent an existing watchdog on a short timer
void skip_bootloader(void) __attribute__ ((used, naked, section (".init3")));
void skip_bootloader(void) {
	// if the watchdog reset bit is set
	if (MCUSR & (1 << WDRF)) {
		// clear it
		MCUSR &= ~(1 << WDRF);
		// disable the watchdog so it doesn't go off during initialization
		wdt_disable();

		// if the watchdog originated from the bootloader, then the timeout occurred,
		// skip to the application
		if (reset_key == BOOTLOADER_KEY) {
			// enable a long watchdog (4s) to enable reprogramming of isolated devices
			// in the case of a botched flash or broken application
			wdt_enable(WDTO_4S);

			// clear the reset key so that we don't end up here again
			reset_key = 0;

			// jump to the application code
			(( void (*)(void))0x0000)();
		}
	}
}

int main(void) {
	uint8_t bootloader_continue = 1;

	// for data transfer
	uint8_t page[SPM_PAGESIZE];
	uint8_t packet[NRF24_MAX_PAYLOAD_SIZE];
	uint8_t seq_no = 0, *page_ptr = NULL;
	uint16_t address = 0;

	// skip_bootloader has already run in init.3 so the watchdog MCUSR bit will have been cleared and the watchdog is disabled
	// set the reset key so that if our internal watchdog reboots, we will know we have been here already
	reset_key = BOOTLOADER_KEY;

	// initialize the radio
	nrf24_init();

	// setup the watchdog timer to reset the device after 8S
	wdt_enable(WDTO_8S);

	// main loop that we will either be in during the watchdog reset
	// or that we will exit from as a result of programming done
	while (bootloader_continue) {
		// if the IRQ pin is low
		if ((PIND & (1 << PD2)) == 0) {
			// incoming data
			uint8_t done = 0;
			uint8_t status = nrf24_read_status();
			uint8_t len, length, *pkt_ptr;

			// pet the dog to keep it from resetting out of the bootloader
			wdt_reset();

			while (!done) {
				len = NRF24_MAX_PAYLOAD_SIZE;
				done = nrf24_rx_read(packet, &len);

				// do something with packet, e.g.
				switch (packet[0]) {
					case FLASH_HELLO:
						// check for version or signature incompatability
						if (packet[1] != FLASH_VERSION_MAJOR) {
							packet[0] = FLASH_CMD_FAILED;
							packet[1] = FLASH_CMD_FAILED__VERSION_MISMATCH;
						} else if (packet[3] != SIGNATURE_0 || packet[4] != SIGNATURE_1 || packet[5] != SIGNATURE_2) {
							packet[0] = FLASH_CMD_FAILED;
							packet[1] = FLASH_CMD_FAILED__SIGNATURE_MISMATCH;
						}
						// for incoming HELLO, we ignore the other bytes
						if (packet[0] == FLASH_CMD_FAILED) {
							nrf24_tx_ack_payload(packet, FLASH_CMD_FAILED_SIZE);
							break;
						}
						// send response
						packet[0] = FLASH_HELLO;
						packet[1] = FLASH_VERSION_MAJOR;
						packet[2] = FLASH_VERSION_MINOR;
						packet[3] = SIGNATURE_0;
						packet[4] = SIGNATURE_1;
						packet[5] = SIGNATURE_2;
						packet[6] = SPM_PAGESIZE;
						packet[7] = (uint8_t)(AVAILABLE_FLASH & 0x00FF);
						packet[8] = (uint8_t)((AVAILABLE_FLASH & 0xFF00) >> 8);
						packet[9] = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
						packet[10] = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
						packet[11] = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
						packet[12] = boot_lock_fuse_bits_get(GET_LOCK_BITS);
						packet[13] = (uint8_t)((E2END + 1) & 0x00FF);
						packet[14] = (uint8_t)(((E2END + 1) & 0xFF00) >> 8);
						nrf24_tx_ack_payload(packet, FLASH_HELLO_SIZE);
						break;
					case FLASH_HELLO_FLUSH:
						// ack payload already loaded
						break;
					case FLASH_DONE:
						// signal to the outer loop that we are done here
						bootloader_continue = 0;
						// disable this handler to prevent any future interupts
						break;
					case FLASH_EEPROM_READ:
						// address checking happens on the host
						packet[0] = FLASH_EEPROM_READ_PAYLOAD;
#if 0
						// wait for any SPM operation to finish or for any eeprom write operation to complete
						// the eeprom write operation isn't possible as the only way to write is below and we wait afterwards
						while (boot_spm_busy() || EECR & (1 << EEPE));
#endif
						// load address
						EEAR = ((uint16_t)packet[2] << 8) | packet[1];
						// start eeprom read
						EECR |= 1 << EERE;
						// read data
						packet[1] = EEDR;
						nrf24_tx_ack_payload(packet, FLASH_EEPROM_READ_PAYLOAD_SIZE);
						break;
					case FLASH_EEPROM_READ_FLUSH:
						// ack payload already loaded
						break;
					case FLASH_EEPROM_PROG:
						// address checking happens on the host
#if 0
						// wait for any SPM operation to finish or for any eeprom write operation to complete
						// the eeprom write operation isn't possible as the only way to write is below and we wait afterwards
						while (boot_spm_busy() || EECR & (1 << EEPE));
#endif
						// load address and data
						EEAR = ((uint16_t)packet[2] << 8) | packet[1];
						EEDR = packet[3];
						// set EEMPE
						EECR |= (1 << EEMPE);
						// begin write
						EECR |= (1 << EEPE);
						while (EECR & (1 << EEPE));	// wait for available
						break;
					case FLASH_PAGE_READ:
						address = ((uint16_t)packet[2] << 8) | packet[1];
						// fill the page
						length = SPM_PAGESIZE;
						page_ptr = page;
						do {
							*page_ptr++ = pgm_read_byte(address++);
						} while (--length);
						// prepare first ack packet
						seq_no = 0;
						packet[0] = FLASH_PAGE_READ_PAYLOAD;
						packet[1] = seq_no;
						length = NRF24_MAX_PAYLOAD_SIZE - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE;
						pkt_ptr = &packet[2];
						page_ptr = page;
						do {
							*pkt_ptr++ = *page_ptr++;
						} while (--length);
						nrf24_tx_ack_payload(packet, NRF24_MAX_PAYLOAD_SIZE);
						break;
					case FLASH_PAGE_READ_FLUSH:
						// prepare next ack_payload, if any
						// check if more - set length
						length = SPM_PAGESIZE - (page_ptr - page);
						if (length > 0) {
							uint8_t payload_length;
							if (length > NRF24_MAX_PAYLOAD_SIZE - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE)
								length = NRF24_MAX_PAYLOAD_SIZE - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE;
							payload_length = length + FLASH_PAGE_READ_PAYLOAD_MIN_SIZE;
							packet[0] = FLASH_PAGE_READ_PAYLOAD;
							packet[1] = ++seq_no;
							pkt_ptr = &packet[FLASH_PAGE_READ_PAYLOAD_MIN_SIZE];
							do {
								*pkt_ptr++ = *page_ptr++;
							} while (--length);
							nrf24_tx_ack_payload(packet, payload_length);
						} // else, we are done loading the acks and this is the final flush, nothing more to send
						break;
					case FLASH_PAGE_PROG:
						address = ((uint16_t)packet[2] << 8) | packet[1];
						// begin page erase
						boot_page_erase(address);
						// setup for next packets
						seq_no = 0;
						page_ptr = page;
						break;
					case FLASH_PAGE_PROG_PAYLOAD:
						// ignore any out of sequence data
						if (packet[1] != seq_no)
							break;
						// but it is too late, already erased
						length = len - FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE;
						pkt_ptr = &packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE];
						do {
							*page_ptr++ = *pkt_ptr++;
						} while (--length);
						++seq_no;
						if (page_ptr - page == SPM_PAGESIZE) {
							uint8_t i;
							page_ptr = page;
							// wait until the page erase is complete
							boot_spm_busy_wait();
							// fill the boot page
							for (i = 0; i < SPM_PAGESIZE; i += 2) {
								uint16_t w = *page_ptr++;
								w += (*page_ptr++) << 8;
								boot_page_fill(address + i, w);
							}
							boot_page_write(address);
							// wait until the page fill is complete - do this now vs. above as the page might be overwritten in the ISR
							boot_spm_busy_wait();
							// reenable rww so that application jump works
							boot_rww_enable();
							seq_no = 0;
						}
						break;
					default:
						// unknown packet type - ignore
						break;
				};
			}

			// reset IRQ status bit
			status |= _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT);
			nrf24_write_reg(STATUS, status);
		}
	}

	// cleanup the SPI and radio pins, etc.
	nrf24_done();

	// disable interrupts
	cli();

	// use the watchdog timer to force a timeout to reset
	// use a long enough time that the next startup completes without the watchdog firing
	wdt_enable(WDTO_120MS);
	for (;;);
}
