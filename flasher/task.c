#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>

#include "error.h"
#include "file.h"
#include "spi.h"
#include "nRF24L01+.h"
#include "task.h"
#include "compatability.h"
#include "flash.h"

const tasks_table_t tasks[] = { \
	{ "flash", 	&task_flash }, \
	{ "nRF24",	&task_nRF24 }, \
	{ NULL,		NULL } /* end */
};

const tasks_table_t tasks_flash[] = { \
	{ "hex", 	&task_flash_hex }, \
	{ "test",	&task_flash_test }, \
	{ "upload", 	&task_flash_upload }, \
	{ "download", 	&task_flash_download }, \
	{ "eeprom",	&task_flash_eeprom }, \
	{ NULL, 	NULL } /* end */
};

const tasks_table_t tasks_nRF24[] = { \
	{ "print", 	&task_nRF24_print }, \
	{ "scanner", 	&task_nRF24_scanner }, \
	{ "ping",	&task_nRF24_ping }, \
	{ NULL, 	NULL } /* end */
};

int task_parse(int level, int argc, char *argv[]) {
	int i;
	int (*function)(int argc, char *argv[]);

	if (argc < level + 1) {
		goto TASK_PARSE_USAGE;
	}

	function = task_lookup(tasks, level, argc, argv);

	if (function) {
		return function(argc, argv);
	}

TASK_PARSE_USAGE:
	warning("usage: %s <task> [<task parameters>]\n", argv[0]);
       	warning("supported tasks are:\n", argv[0]);
       	for (i = 0; tasks[i].name != NULL; ++i)
	       	warning("\t%s\n", tasks[i].name);

	return EXIT_FAILURE;
}

int (*task_lookup(const tasks_table_t *lookup_table, int argv_index, int argc, char *argv[]))(int argc, char *argv[]) {
	int i;

	if (argv_index >= argc) {
		warning("%s: argv index %d is out of range (0 to %d)\n", __PRETTY_FUNCTION__, argv_index, argc - 1);
		return NULL;
	}

	for (i = 0; lookup_table[i].name != NULL; ++i) {
		if (strcmp(lookup_table[i].name, argv[argv_index]) == 0) {
			return lookup_table[i].function;
			break;
		}
	}
	warning("%s: unhandled token '%s'\n", __PRETTY_FUNCTION__, argv[argv_index]);
	return NULL;
}


nrf24_t *task_radio_setup(nrf24_power_level_e power_level, nrf24_data_rate_e data_rate, uint8_t channel, uint64_t write_pipe_addr, uint64_t read_pipe_addr) {
	nrf24_t *radio;

	if (getuid() != 0) {
		fatal_error("must be run with root priveleges to access GPIO sysfs files; try with sudo\n");
	}

	radio = nrf24_init();

	nrf24_set_retries(radio, 15, 15);
	nrf24_set_power_level(radio, power_level);
	nrf24_set_data_rate(radio, data_rate);
	nrf24_set_channel(radio, channel);
	nrf24_open_write_pipe(radio, write_pipe_addr);
	nrf24_open_read_pipe(radio, 1, read_pipe_addr);
	nrf24_enable_dynamic_payloads(radio);
	nrf24_set_auto_ack(radio, 1);
	nrf24_enable_ack_payload(radio);
	nrf24_power_up(radio);

	return radio;
}

void task_radio_done(nrf24_t *radio) {
	nrf24_done(radio);
}

// verbose should be the uint64_t address of the recipient if anything should be printed, other this function is stdout silent
uint8_t task_send_packet(nrf24_t *radio, char *packet_type_name, uint8_t *packet, uint8_t len, uint32_t delay_in_us, uint64_t verbose) {
	uint8_t ok;
	struct timeval start, end;
	int ms;

	if (verbose)
		gettimeofday(&start, NULL);
	ok = nrf24_send(radio, packet, len);
	if (verbose) {
		gettimeofday(&end, NULL);
		ms = ((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5;

		printf("sent %s to 0x%llx: time=%d ms%s\n", packet_type_name, verbose, ms, (ok ? "" : " - FAILED!"));
	}

	usleep(delay_in_us);

	return ok;
}

uint8_t task_read_ack_payload(nrf24_t *radio, uint8_t *packet, uint8_t packet_type, uint8_t packet_len) {
	// read payload
	
	if (nrf24_is_ack_payload_available(radio)) {
		nrf24_read_payload(radio, packet, radio->ack_payload_length);
		if (radio->ack_payload_length != packet_len) {
			warning("ack payload for last packet is not the correct size, got=%d - expected=%d\n", radio->ack_payload_length, packet_len);
			return 0;
		}
		if (radio->ack_payload_length < 1) {
			warning("expected ack payload < 1\n");
			return 0;
		}
		if (packet[0] != packet_type) {
			warning("ack payload for last packet is not the correct type, got=%d - expected=%d", packet[0], packet_type);
			return 0;
		}
	} else {
		warning("no ack payload for last packet available!\n");
		return 0;
	}
	return 1;
}
