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

extern const tasks_table_t tasks_nRF24[];

nrf24_t *radio;

#define PIPE_0_ADDR	0xF0F0F0F0E1LL
#define PIPE_1_ADDR	0xF0F0F0F0D2LL

#define CHANNEL	100

const uint64_t ping_pipes[2] = { PIPE_0_ADDR, PIPE_1_ADDR };

int task_nRF24(int argc, char *argv[]) {
	int (*function)(int argc, char *argv[]);

	if (argc < 3) {
		int i;
TASK_NRF24_USAGE:
		warning("usage: %s %s <task> [<task parameters>]\n", argv[0], argv[1]);
       		warning("supported tasks are:\n", argv[0]);
       		for (i = 0; tasks_nRF24[i].name != NULL; ++i)
	       		warning("\t%s\n", tasks_nRF24[i].name);
		return EXIT_FAILURE;
	}

	function = task_lookup(tasks_nRF24, 2, argc, argv);

	if (function) {
		int result;
		radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, ping_pipes[0], ping_pipes[1]);
		result = function(argc, argv);
		if (radio)
			task_radio_done(radio);
		return result;
	}

	goto TASK_NRF24_USAGE;

	return EXIT_SUCCESS;
}

int task_nRF24_print(int argc, char *argv[]) {
	if (argc != 3) {
		warning("usage: %s %s %s\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	nrf24_print_details(radio);
	
	return EXIT_SUCCESS;
}

int task_nRF24_scanner(int argc, char *argv[]) {
	int cycles, i, j;
	uint8_t prd[NRF24__MAX_CHANNEL + 1];

	if (argc != 3 && argc != 4) {
		warning("usage: %s %s %s [<scan cycles>]\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	if (argc == 4)
		cycles = atoi(argv[3]);
	else
		cycles = 32;

	printf("scanning %d channels for %d cycles\n", NRF24__MAX_CHANNEL + 1, cycles);

	nrf24_set_auto_ack(radio, 0);	// disable auto ack
	nrf24_start_listening(radio);
	nrf24_stop_listening(radio);	// go into standby mode

	nrf24_print_details(radio);

	// dump channel header
	for (i = 0; i <= NRF24__MAX_CHANNEL; ++i)
		printf("%x", i >> 4);
	printf("\r\n");
	for (i = 0; i <= NRF24__MAX_CHANNEL; ++i)
		printf("%x", i&0xF);
	printf("\r\n");

	// cycle through channels and get PRD
	memset(prd, 0, sizeof(prd));
	for (i = 0; i < cycles; ++i) {
		for (j = 0; j <= NRF24__MAX_CHANNEL; ++j) {
			nrf24_set_channel(radio, j);
			nrf24_start_listening(radio);
			usleep(5000);
			nrf24_stop_listening(radio);
			if (nrf24_test_carrier(radio))
				prd[j]++;
		}
		for (j = 0; j <= NRF24__MAX_CHANNEL; ++j)
			printf("%x", (0xF < prd[j] ? 0xF : prd[j]));
		printf("\r\n");
	}

	return EXIT_SUCCESS;
}

#define BUFLEN 32
int task_nRF24_ping(int argc, char *argv[]) {
	int pings, i, ms;
	char buffer[BUFLEN];
	struct timeval start, end;
	uint8_t ok, len;

	if (argc != 4) {
		warning("usage: %s %s %s <ping count>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}
	pings = atoi(argv[3]);

	printf("%s: pinging avr %d times\n", argv[0], pings);

	nrf24_set_retries(radio, 15, 15);
	nrf24_set_power_level(radio, NRF24_PA_LOW);
	nrf24_set_channel(radio, CHANNEL);
	nrf24_open_write_pipe(radio, ping_pipes[0]);
	nrf24_open_read_pipe(radio, 1, ping_pipes[1]);
	nrf24_enable_dynamic_payloads(radio);
	nrf24_set_auto_ack(radio, 1);
	nrf24_enable_ack_payload(radio);
	nrf24_power_up(radio);

	nrf24_start_listening(radio);
	nrf24_print_details(radio);
	nrf24_stop_listening(radio);
	for (i = 0; i < pings; ++i) {
		snprintf(buffer, BUFLEN - 1, "ping %d", i + 1);
		len = strlen(buffer) + 1;
		gettimeofday(&start, NULL);
		ok = nrf24_send(radio, (uint8_t *)buffer, len);
		gettimeofday(&end, NULL);
		ms = ((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5;
		if (ok) {
			printf("%2d bytes to 0x%llx: seq=%d, time=%d ms\n", len, ping_pipes[1], i, ms);
		} else {
			printf("%2d bytes to 0x%llx: seq=%d, time=%d ms - FAILED!\n", len, ping_pipes[1], i, ms);
		}
		usleep(50000);
	}
	return EXIT_SUCCESS;
}
