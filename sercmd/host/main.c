#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "serial.h"
#include "nRF24L01+.h"

void scanner(nrf24_t *radio);

int main(int argc, char *argv[]) {
	nrf24_t *radio;

	if (argc != 3) {
		fatal_error("usage: %s <serial device> <serial baud>\n", argv[0]);
	}

	radio = nrf24_init(argv[1], strtoul(argv[2], NULL, 10));

	scanner(radio);

	exit(EXIT_SUCCESS);
}

void scanner(nrf24_t *radio) {
        int cycles, i, j;
        uint8_t prd[NRF24__MAX_CHANNEL + 1];

	cycles = 32;

        printf("scanning %d channels for %d cycles\n", NRF24__MAX_CHANNEL + 1, cycles);

        nrf24_set_auto_ack(radio, 0);   // disable auto ack
        nrf24_start_listening(radio);
        nrf24_stop_listening(radio);    // go into standby mode

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
                        //usleep(5000);
                        nrf24_stop_listening(radio);
                        if (nrf24_test_carrier(radio))
                                prd[j]++;
                }
                for (j = 0; j <= NRF24__MAX_CHANNEL; ++j)
                        printf("%x", (0xF < prd[j] ? 0xF : prd[j]));
                printf("\r\n");
        }
}
