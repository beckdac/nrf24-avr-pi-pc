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
#include "gpio.h"
#include "nRF24L01+.h"
#include "program.h"
#include "task.h"
#include "compatability.h"
#include "packet.h"
#include "flash.h"

const uint64_t program_pipes[2] = { PIPE_0_ADDR, PIPE_1_ADDR };

nrf24_t *radio;

extern const tasks_table_t tasks_program[];

int task_program(int argc, char *argv[]) {
	int (*function)(int argc, char *argv[]);

	if (argc < 3) {
		int i;
TASK_PROGRAM_USAGE:
		warning("usage: %s %s <task> [<task parameters>]\n", argv[0], argv[1]);
		warning("supported tasks are:\n", argv[0]);
		for (i = 0; tasks_program[i].name != NULL; ++i)
			warning("\t%s\n", tasks_program[i].name);
		return EXIT_FAILURE;
	}

	function = task_lookup(tasks_program, 2, argc, argv);

	if (function) {
		int result;
		radio = NULL;
		result = function(argc, argv);
		if (radio)
			task_radio_done(radio);
		return result;
	}

	goto TASK_PROGRAM_USAGE;

	return EXIT_SUCCESS;
}

int task_program_parse(int argc, char *argv[]) {
	program_t *p;

	if (argc != 4) {
		warning("usage: %s %s %s <filename>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	p = program_load_from_file(argv[3]);

	program_free(p);
	
	return EXIT_SUCCESS;
}

int task_program_rainbow(int argc, char *argv[]) {
	float frequency = .3;
	uint8_t i, rgb[3];
	uint16_t steps = 43, delay_in_ms = 100;
	char *filename;
	file_t *f;

	if (argc < 4 && argc > 7) {
		warning("usage: %s %s %s <filename> [<frequency> <steps> <delay>]\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}
	filename = argv[3];
	if (argc == 7) {
		frequency = atof(argv[4]);
		steps = atoi(argv[5]);
		delay_in_ms = atoi(argv[6]);
	}

	f = file_open(filename, "w");
	
	for (i = 0; i < steps; ++i) {
   		rgb[0] = (uint8_t)(sin(frequency*(float)i + 0.) * 127.) + 128;
   		rgb[1] = (uint8_t)(sin(frequency*(float)i + 2.) * 127.) + 128;
   		rgb[2] = (uint8_t)(sin(frequency*(float)i + 4.) * 127.) + 128;
		fprintf(f->fp, "%d\t%d\t%d\t%d\n", rgb[0], rgb[1], rgb[2], delay_in_ms);
	}

	file_close(f);

	return EXIT_SUCCESS;
}

int task_program_upload(int argc, char *argv[]) {
	program_t *p;
	uint8_t packet[NRF24__MAX_PAYLOAD_SIZE];
	uint16_t i;

	if (argc != 4) {
		warning("usage: %s %s %s <filename>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	printf("%s: uploading program from '%s' to avr\n", argv[0], argv[3]);

	p = program_load_from_file(argv[3]);

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	// send stop program
	packet[0] = PACKET_STOP_PROGRAM;
	if (!task_send_packet(radio, "STOP_PROGRAM", packet, PACKET_STOP_PROGRAM_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	
	// send black color
	packet[0] = PACKET_SET_COLOR;
	packet[1] = 0;
	packet[2] = 0;
	packet[3] = 0;
	if (!task_send_packet(radio, "STOP_PROGRAM", packet, PACKET_SET_COLOR_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	// send programming begin packet
	packet[0] = PACKET_BEGIN_PROGRAMMING;
	if (!task_send_packet(radio, "BEGIN_PROGRAMMING", packet, PACKET_BEGIN_PROGRAMMING_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	
	// send program length
	packet[0] = PACKET_PROGRAM_LENGTH;
	packet[1] = (uint8_t)(p->steps & 0x00FF);
	packet[2] = (uint8_t)((p->steps & 0xFF00) >> 8);
	if (!task_send_packet(radio, "PROGRAM_LENGTH", packet, PACKET_PROGRAM_LENGTH_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	// send program steps
	for (i = 0; i < p->steps; ++i) {
		packet[0] = PACKET_PROGRAM_STEP;
		packet[1] = (uint8_t)(i & 0x00FF);
		packet[2] = (uint8_t)((i & 0xFF00) >> 8);
		packet[3] = p->step[i]->rgb[0];
		packet[4] = p->step[i]->rgb[1];
		packet[5] = p->step[i]->rgb[2];
		packet[6] = (uint8_t)(p->step[i]->delay_in_ms & 0x00FF);
		packet[7] = (uint8_t)((p->step[i]->delay_in_ms & 0xFF00) >> 8);
		if (!task_send_packet(radio, "PROGRAM_STEP", packet, PACKET_PROGRAM_STEP_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
			return EXIT_FAILURE;
	}

	// send programming done packet
	packet[0] = PACKET_END_PROGRAMMING;
	if (!task_send_packet(radio, "END_PROGRAMMING", packet, PACKET_END_PROGRAMMING_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	// send start program
	packet[0] = PACKET_RUN_PROGRAM;
	if (!task_send_packet(radio, "RUN_PROGRAM", packet, PACKET_RUN_PROGRAM_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	program_free(p);

	return EXIT_SUCCESS;
}

int task_program_download(int argc, char *argv[]) {
	file_t *f;
	uint8_t packet[NRF24__MAX_PAYLOAD_SIZE], rgb[3];
	uint16_t steps, step, i, delay_in_ms;

	if (argc != 4) {
		warning("usage: %s %s %s <filename>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	printf("%s: downloading program from %llx to '%s'\n", argv[0], program_pipes[1], argv[3]);

	f = file_open(argv[3], "w");
	
	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	// send stop program
	packet[0] = PACKET_STOP_PROGRAM;
	if (!task_send_packet(radio, "STOP_PROGRAM", packet, PACKET_STOP_PROGRAM_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	
	// get program length
	// first packet primes client to send data on the followup flush packet
	packet[0] = PACKET_READ_PROGRAM_LEN;
	if (!task_send_packet(radio, "READ_PROGRAM_LEN", packet, PACKET_READ_PROGRAM_LEN_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	packet[0] = PACKET_READ_PROGRAM_LEN_FLUSH;
	if (!task_send_packet(radio, "READ_PROGRAM_LEN_FLUSH", packet, PACKET_READ_PROGRAM_LEN_FLUSH_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	// read payload
	if (!task_read_ack_payload(radio, packet, PACKET_PROGRAM_LENGTH, PACKET_PROGRAM_LENGTH_SIZE)) {
		task_program_print_ack_payload(radio, packet);
		return EXIT_FAILURE;
	}
	steps = ((uint16_t)packet[2] << 8) | packet[1];

	printf("reading %d steps...\n", steps);

	// get program steps
	for (i = 0; i < steps; ++i) {
		packet[0] = PACKET_READ_PROGRAM_STEP;
		packet[1] = (uint8_t)(i & 0x00FF);
		packet[2] = (uint8_t)((i & 0xFF00) >> 8);
		printf("fetching data for step %d (packet type = %d)\n", i, PACKET_READ_PROGRAM_STEP);
		if (!task_send_packet(radio, "READ_PROGRAM_STEP", packet, PACKET_READ_PROGRAM_STEP_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
			return EXIT_FAILURE;
		packet[0] = PACKET_READ_PROGRAM_STEP_FLUSH;
		if (!task_send_packet(radio, "READ_PROGRAM_STEP_FLUSH", packet, PACKET_READ_PROGRAM_STEP_FLUSH_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
			return EXIT_FAILURE;
		// read payload
		if (!task_read_ack_payload(radio, packet, PACKET_PROGRAM_STEP, PACKET_PROGRAM_STEP_SIZE)) {
			task_program_print_ack_payload(radio, packet);
			return EXIT_FAILURE;
		}
		step = ((uint16_t)packet[2] << 8) | packet[1];
		rgb[0] = packet[3];
		rgb[1] = packet[4];
		rgb[2] = packet[5];
		delay_in_ms = ((uint16_t)packet[7] << 8) | packet[6];

		if (step != i) {
			warning("%s: program read back is out of order, expected step=%d, got=%d\n", argv[0], i, step);
		}
		fprintf(f->fp, "# step %d\n%d\t%d\t%d\t%d\n", step, rgb[0], rgb[1], rgb[2], delay_in_ms);
	}

	// send start program
	packet[0] = PACKET_RUN_PROGRAM;
	if (!task_send_packet(radio, "RUN_PROGRAM", packet, PACKET_RUN_PROGRAM_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	file_close(f);

	return EXIT_SUCCESS;
}

int task_program_stop(int argc, char *argv[]) {
	uint8_t packet[1];

	if (argc != 3) {
		warning("usage: %s %s %s\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
	nrf24_print_details(radio);
#ifdef PI_DEBUG
#endif

	packet[0] = PACKET_STOP_PROGRAM;
	if (!task_send_packet(radio, "STOP_PROGRAM", packet, PACKET_STOP_PROGRAM_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int task_program_start(int argc, char *argv[]) {
	uint8_t packet[1];

	if (argc != 3) {
		warning("usage: %s %s %s\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	packet[0] = PACKET_RUN_PROGRAM;
	if (!task_send_packet(radio, "RUN_PROGRAM", packet, PACKET_RUN_PROGRAM_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int task_program_rgb(int argc, char *argv[]) {
	uint8_t packet[4];

	if (argc != 6) {
		warning("usage: %s %s %s <red (0 to 255)> <green (0 to 255)> <blue (0 to 255)>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	packet[0] = PACKET_SET_COLOR;
	packet[1] = atoi(argv[3]);
	packet[2] = atoi(argv[4]);
	packet[3] = atoi(argv[5]);
	if (!task_send_packet(radio, "SET_COLOR", packet, PACKET_SET_COLOR_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int task_program_reset(int argc, char *argv[]) {
	uint8_t packet[1];

	if (argc != 3) {
		warning("usage: %s %s %s\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	packet[0] = PACKET_RESET;
	if (!task_send_packet(radio, "RESET", packet, PACKET_RESET_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int task_program_lightfreq(int argc, char *argv[]) {
	uint8_t packet[NRF24__MAX_PAYLOAD_SIZE];
	uint16_t hz;

	if (argc != 3) {
		warning("usage: %s %s %s\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	printf("%s: fetching TSL230R frequency data from %llx\n", argv[0], program_pipes[1]);

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, CHANNEL, program_pipes[0], program_pipes[1]);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	// get frequency report in Hz from input capture pin hooked to TSL230R
	// first packet primes client to send data on the followup flush packet
	packet[0] = PACKET_READ_LIGHT_FREQ;
	if (!task_send_packet(radio, "READ_LIGHT_FREQ", packet, PACKET_READ_LIGHT_FREQ_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	packet[0] = PACKET_READ_LIGHT_FREQ_FLUSH;
	if (!task_send_packet(radio, "READ_LIGHT_FREQ_FLUSH", packet, PACKET_READ_LIGHT_FREQ_FLUSH_SIZE, PROGRAM_SEND_POST_DELAY_US, program_pipes[1]))
		return EXIT_FAILURE;
	// read payload
	if (!task_read_ack_payload(radio, packet, PACKET_LIGHT_FREQ, PACKET_LIGHT_FREQ_SIZE)) {
		task_program_print_ack_payload(radio, packet);
		return EXIT_FAILURE;
	}
	hz = ((uint16_t)packet[2] << 8) | packet[1];
	printf("TSL230R on input capture reports %d Hz\n", hz);

	return EXIT_SUCCESS;
}

void task_program_print_ack_payload(nrf24_t *radio, uint8_t *packet) {
        int i;

        printf("ACK packet:\n");
        switch (packet[0]) {
                case PACKET_PROGRAM_LENGTH: {
                                uint16_t steps = ((uint16_t)packet[2] << 8) | packet[1];
                                printf("\tPACKET_PROGRAM_LENGTH (expected length = %d, got = %d)\n", PACKET_PROGRAM_LENGTH_SIZE, radio->ack_payload_length);
                                for (i = 0; i < radio->ack_payload_length; ++i) {
                                        printf("\t\tpacket[%d] = %d\n", i, packet[i]);
                                }
                                printf("\tdecoded content: steps = %d\n", steps);
                                break;
                        }
                case PACKET_PROGRAM_STEP: {
                                uint16_t step = ((uint16_t)packet[2] << 8) | packet[1];
                                uint8_t rgb[3] = { packet[3], packet[4], packet[5] };
                                uint16_t delay_in_ms = ((uint16_t)packet[7] << 8) | packet[6];
                                printf("PACKET_PROGRAM_STEP (expected length = %d, got = %d)\n", PACKET_PROGRAM_STEP_SIZE, radio->ack_payload_length);
                                for (i = 0; i < radio->ack_payload_length; ++i) {
                                        printf("\t\tpacket[%d] = %d\n", i, packet[i]);
                                }
                                printf("\tdecoded content: step = %d, rgb = %d  %d  %d, delay_in_ms = %d\n", step, rgb[0], rgb[1], rgb[2], delay_in_ms);
                                break;
                        }
                case PACKET_LIGHT_FREQ: {
                                uint16_t hz = ((uint16_t)packet[2] << 8) | packet[1];
                                printf("PACKET_LIGHT_FREQ (expected length = %d, got = %d)\n", PACKET_LIGHT_FREQ_SIZE, radio->ack_payload_length);
                                for (i = 0; i < radio->ack_payload_length; ++i) {
                                        printf("\t\tpacket[%d] = %d\n", i, packet[i]);
                                }
                                printf("\tdecoded content: hz = %d\n", hz);
                                break;
                        }
                default:
                        printf("\tunhandled ack packet type in print function: %d\n", packet[0]);
                        break;
        };
}
