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
#include "task.h"
#include "compatability.h"
#include "flash.h"
#include "hex.h"

// data structure for holding the various pieces of flash related data
typedef struct flash_data {
	struct flash_data_addr {
		uint64_t source;
		uint64_t target;
	} addr;
	uint8_t hello_retries;
	uint8_t sig[3];
	struct flash_data_version {
		uint8_t major;
		uint8_t minor;
	} version;
	uint8_t spm_pagesize;
	uint16_t available_flash;
	struct flash_data_fuses {
		uint8_t low;
		uint8_t high;
		uint8_t extended;
		uint8_t lock;
	} fuses;
	uint16_t eeprom_size;
	hex_t *hex;
	uint8_t packet[NRF24__MAX_PAYLOAD_SIZE];
} flash_t;

// table of available tasks in the flash unit
extern const tasks_table_t tasks_flash[];

// addresses for us and bootloader
const uint64_t flash_pipes[2] = { FLASH_SOURCE, FLASH_TARGET };

// prototypes for local functions
static uint8_t task_flash_hello_exchange(flash_t *f, uint8_t expected_sig[3]);
static void task_flash_print_details(flash_t *f);
static int task_flash_upload_core(flash_t *f, hex_t *hex);
static int task_flash_download_core(flash_t *f, uint16_t start_address, uint16_t end_address);

// pointer to radio data structure
nrf24_t *radio;

int task_flash(int argc, char *argv[]) {
	int (*function)(int argc, char *argv[]);

	if (argc < 3) {
		int i;
TASK_FLASH_USAGE:
		warning("usage: %s %s <task> [<task parameters>]\n", argv[0], argv[1]);
       		warning("supported tasks are:\n", argv[0]);
       		for (i = 0; tasks_flash[i].name != NULL; ++i)
	       		warning("\t%s\n", tasks_flash[i].name);
		return EXIT_FAILURE;
	}

	function = task_lookup(tasks_flash, 2, argc, argv);

	if (function) {
		int result;
		radio = NULL;
		result = function(argc, argv);
		if (radio)
			task_radio_done(radio);
		return result;
	}

	goto TASK_FLASH_USAGE;

	return EXIT_SUCCESS;
}

int task_flash_hex(int argc, char *argv[]) {
	hex_t *h;

	if (argc < 4 || argc > 5) {
		warning("usage: %s %s %s <input hex filename> [<output hex filename>]\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	h = hex_load_from_file(argv[3]);

	if (argc == 5) {
#if 0
		hex_save_to_file(h, argv[4]);
#else
		hex_t *hout;
		hout = hex_from_map(h->mem, h->tag, h->total_bytes, h->min_address);
		hex_save_to_file(hout, argv[4]);
		hex_free(hout);
#endif
	}

	hex_free(h);
	
	return EXIT_SUCCESS;
}

int task_flash_test(int argc, char *argv[]) {
	flash_t f;
	uint8_t i, expected_sig[3];

	if (argc != 6 && argc != 7) {
		warning("usage: %s %s %s <sig byte 0> <sig byte 1> <sig byte 2> [<retries>]\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	memset(&f, 0, sizeof(flash_t));
	f.hello_retries = 10;

	// setup the data structure from constants and arguments
	if (argc == 7) {
		f.hello_retries = atoi(argv[6]);
	}
	for (i = 0; i < 3; ++i)
		expected_sig[i] = (uint8_t)strtoul(argv[i + 3], NULL, 16);
	f.addr.source = flash_pipes[0];
	f.addr.target = flash_pipes[1];

	printf("%s: testing bootloader at address: %llx\n", argv[0], f.addr.target);

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, FLASH_CHANNEL, f.addr.source, f.addr.target);
	nrf24_print_details(radio);
#ifdef PI_DEBUG
#endif

	if (!task_flash_hello_exchange(&f, expected_sig)) {
		warning("%s: HELLO exchange failed!\n", argv[0]);
		task_flash_print_details(&f);
		return EXIT_FAILURE;
	}

	task_flash_print_details(&f);

#if 0
	// send application start
	f.packet[0] = FLASH_DONE;
	if (!task_send_packet(radio, "FLASH_DONE", f.packet, FLASH_DONE_SIZE, FLASH_SEND_POST_DELAY_US, f.addr.target))
		return EXIT_FAILURE;
#endif

	return EXIT_SUCCESS;
}

int task_flash_upload(int argc, char *argv[]) {
	hex_t *h;
	flash_t f;
	uint8_t expected_sig[3];
	uint16_t i;

	if (argc != 7) {
		warning("usage: %s %s %s <sig byte 0> <sig byte 1> <sig byte 2> <hex filename>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	h = hex_load_from_file(argv[6]);

	memset(&f, 0, sizeof(flash_t));

	// setup the data structure from constants and arguments
	f.hello_retries = 10;
	for (i = 0; i < 3; ++i)
		expected_sig[i] = (uint8_t)strtoul(argv[i + 3], NULL, 16);
	f.addr.source = flash_pipes[0];
	f.addr.target = flash_pipes[1];

	printf("%s: updating flash via bootloader at address: %llx\n", argv[0], f.addr.target);

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, FLASH_CHANNEL, f.addr.source, f.addr.target);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	if (!task_flash_hello_exchange(&f, expected_sig)) {
		warning("%s: HELLO exchange failed!\n", argv[0]);
		task_flash_print_details(&f);
		return EXIT_FAILURE;
	}

	task_flash_print_details(&f);

	if (!task_flash_upload_core(&f, h)) {
		warning("%s: downloading application space failed!\n", argv[0]);
	}

	if (!task_flash_download_core(&f, h->min_address, h->max_address)) {
		warning("%s: downloading application space failed!\n", argv[0]);
	}

	// compare hex
	for (i = 0; i < h->total_bytes; ++i) {
		if (h->mem[i] != f.hex->mem[i]) {
			warning("%s: verification of uploaded program failed at address %d (found %hhX, expected %hhX)", argv[0], i + h->min_address, f.hex->mem[i], h->mem[i]);
			return EXIT_FAILURE;
		}
	}

	hex_free(f.hex);

	// send application start
	f.packet[0] = FLASH_DONE;
	if (!task_send_packet(radio, "FLASH_DONE", f.packet, FLASH_DONE_SIZE, FLASH_SEND_POST_DELAY_US, f.addr.target))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int task_flash_upload_core(flash_t *f, hex_t *h) {
	uint16_t i, pages, j, k;
	uint8_t packets_per_page, packet_mod;
	struct timeval start, end;

	if (h->total_bytes > f->available_flash) {
		warning("required upload size (%d) exceeds available flash (%d)\n", h->total_bytes, f->available_flash);
		return 0;
	}
	if (h->min_address >= f->available_flash) {
		warning("start address (%d) exceeds available flash (%d)\n", h->min_address, f->available_flash);
		return 0;
	}
	if (h->max_address >= f->available_flash) {
		warning("end address (%d) exceeds available flash (%d)\n", h->max_address, f->available_flash);
		return 0;
	}
	pages = h->total_bytes / f->spm_pagesize;
	if (h->total_bytes % f->spm_pagesize > 0)
		pages++;
	packets_per_page = f->spm_pagesize / (NRF24__MAX_PAYLOAD_SIZE - FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE);
	packet_mod = f->spm_pagesize % (NRF24__MAX_PAYLOAD_SIZE - FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE);
	if (packet_mod > 0)
		packets_per_page++;
	printf("uploading %d pages of %d bytes in %d packets for each page\n", pages, f->spm_pagesize, packets_per_page);
	gettimeofday(&start, NULL);
	for (i = 0; i < pages; ++i) {
		uint16_t address = (i * f->spm_pagesize) + h->min_address;
		uint8_t length, data_len;
		//printf("downloading page %3d\n", i + 1);
		printf("o");
		f->packet[0] = FLASH_PAGE_PROG;
		f->packet[1] = (uint8_t)(address & 0x00FF);
		f->packet[2] = (uint8_t)((address & 0xFF00) >> 8);
		//if (!task_send_packet(radio, "FLASH_PAGE_PROG", f->packet, FLASH_PAGE_PROG_SIZE, FLASH_SEND_POST_DELAY_US, f->addr.target))
		if (!task_send_packet(radio, "FLASH_PAGE_PROG", f->packet, FLASH_PAGE_PROG_SIZE, FLASH_SEND_POST_DELAY_US, 0))
			return 0;
		for (j = 0; j < packets_per_page; ++j) {
			//printf("\t%d/%d", j + 1, packets_per_page);
			printf(".");
			f->packet[0] = FLASH_PAGE_PROG_PAYLOAD;
			f->packet[1] = j;
			if ((j == packets_per_page - 1) && (packet_mod > 0)) {
				length = FLASH_PAGE_READ_PAYLOAD_MIN_SIZE + packet_mod;
			} else {
				length = NRF24__MAX_PAYLOAD_SIZE;
			}
			data_len = length - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE;
#if 0
			if (!(address + data_len > h->max_address))
				memcpy(&f->packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE], &h->mem[address - h->min_address], data_len);
			else {
				for (k = address; k <= h->max_address && k - address < data_len; ++k)
					f->packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE + (k - address)] = h->mem[k - h->min_address];
				for (; k - address < data_len; ++k)
					f->packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE + (k - address)] = 0xFF;
			}
#else
			for (k = address; k - address < data_len; ++k) {
				if (k <= h->max_address) {	
					f->packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE + (k - address)] = h->mem[k - h->min_address];
				} else {
					f->packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE + (k - address)] = 0xFF;
				}
			}
#endif
			address += data_len;
			if (!task_send_packet(radio, "FLASH_PAGE_PROG_PAYLOAD", f->packet, length, FLASH_SEND_POST_DELAY_US, 0))
				return 0;
			fflush(stdout);
		}
	}
	printf("\n");
	gettimeofday(&end, NULL);
	printf("uploaded %d bytes in %.2f seconds (%.2f bytes / second)\n", h->total_bytes, (((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5) / 1000., (float)h->total_bytes / ((((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5)/1000.));

	return 1;
}

int task_flash_download(int argc, char *argv[]) {
	flash_t f;
	uint8_t i, expected_sig[3];

	if (argc != 7) {
		warning("usage: %s %s %s <sig byte 0> <sig byte 1> <sig byte 2> <hex filename>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	memset(&f, 0, sizeof(flash_t));

	// setup the data structure from constants and arguments
	f.hello_retries = 10;
	for (i = 0; i < 3; ++i)
		expected_sig[i] = (uint8_t)strtoul(argv[i + 3], NULL, 16);
	f.addr.source = flash_pipes[0];
	f.addr.target = flash_pipes[1];

	printf("%s: downloading flash contents from  bootloader at address: %llx\n", argv[0], f.addr.target);

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, FLASH_CHANNEL, f.addr.source, f.addr.target);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	if (!task_flash_hello_exchange(&f, expected_sig)) {
		warning("%s: HELLO exchange failed!\n", argv[0]);
		task_flash_print_details(&f);
		return EXIT_FAILURE;
	}

	task_flash_print_details(&f);

	if (!task_flash_download_core(&f, 0, f.available_flash - 1)) {
		warning("%s: downloading application space failed!\n", argv[0]);
	}

	hex_save_to_file(f.hex, argv[6]);

	hex_free(f.hex);

	// send application start
	f.packet[0] = FLASH_DONE;
	if (!task_send_packet(radio, "FLASH_DONE", f.packet, FLASH_DONE_SIZE, FLASH_SEND_POST_DELAY_US, f.addr.target))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int task_flash_download_core(flash_t *f, uint16_t start_address, uint16_t end_address) {
	uint8_t *mem, *tag;
	uint16_t bytes, i, pages, j, k;
	uint8_t packets_per_page, packet_mod;
	struct timeval start, end;

	bytes = (end_address - start_address) + 1;
	if (bytes > f->available_flash) {
		warning("requested download size (%d) exceeds available flash (%d)\n", bytes, f->available_flash);
		return 0;
	}
	if (start_address >= f->available_flash) {
		warning("start address (%d) exceeds available flash (%d)\n", start_address, f->available_flash);
		return 0;
	}
	if (end_address >= f->available_flash) {
		warning("end address (%d) exceeds available flash (%d)\n", end_address, f->available_flash);
		return 0;
	}
	pages = bytes / f->spm_pagesize;
	if (bytes % f->spm_pagesize > 0)
		++pages;
	mem = (uint8_t *)malloc(bytes);
	tag = (uint8_t *)malloc(bytes);
	memset(mem, 0xFF, bytes);
	memset(tag, HEX_TAG_ALLOC, bytes);
	// initiate download
	packets_per_page = f->spm_pagesize / (NRF24__MAX_PAYLOAD_SIZE - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE);
	packet_mod = f->spm_pagesize % (NRF24__MAX_PAYLOAD_SIZE - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE);
	if (packet_mod > 0)
		packets_per_page++;
	printf("downloading %d pages of %d bytes in %d packets for each page\n", pages, f->spm_pagesize, packets_per_page);
	gettimeofday(&start, NULL);
	for (i = 0; i < pages; ++i) {
		uint16_t address = (i * f->spm_pagesize) + start_address;
		uint8_t length, data_len;
		//printf("downloading page %3d\n", i + 1);
		printf("o");
		f->packet[0] = FLASH_PAGE_READ;
		f->packet[1] = (uint8_t)(address & 0x00FF);
		f->packet[2] = (uint8_t)((address & 0xFF00) >> 8);
		//if (!task_send_packet(radio, "FLASH_PAGE_READ", f->packet, FLASH_PAGE_READ_SIZE, FLASH_SEND_POST_DELAY_US, f->addr.target))
		if (!task_send_packet(radio, "FLASH_PAGE_READ", f->packet, FLASH_PAGE_READ_SIZE, FLASH_SEND_POST_DELAY_US, 0))
			return 0;
		for (j = 0; j < packets_per_page; ++j) {
			//printf("\t%d/%d", j + 1, packets_per_page);
			printf(".");
			f->packet[0] = FLASH_PAGE_READ_FLUSH;
			//if (!task_send_packet(radio, "FLASH_PAGE_READ_FLUSH", f->packet, FLASH_PAGE_READ_FLUSH_SIZE, FLASH_SEND_POST_DELAY_US, f->addr.target))
			if (!task_send_packet(radio, "FLASH_PAGE_READ_FLUSH", f->packet, FLASH_PAGE_READ_FLUSH_SIZE, FLASH_SEND_POST_DELAY_US, 0))
				return 0;
			if ((j == packets_per_page - 1) && (packet_mod > 0)) {
				length = FLASH_PAGE_READ_PAYLOAD_MIN_SIZE + packet_mod;
			} else {
				length = NRF24__MAX_PAYLOAD_SIZE;
			}
			data_len = length - FLASH_PAGE_READ_PAYLOAD_MIN_SIZE;
			if (!task_read_ack_payload(radio, f->packet, FLASH_PAGE_READ_PAYLOAD, length)) {
				warning("while reading page %d, did not received payload packet %d of %d\n", i + 1, j + 1, packets_per_page);
				return 0;
			}
			if (f->packet[1] != j) {
				warning("while reading page %d, for payload packet %d of %d, the sequence number was incorrect (%d)\n", i + 1, j + 1, packets_per_page, f->packet[1]);
				return 0;
			}
#if 0
			memcpy(&mem[address], &f->packet[FLASH_PAGE_READ_PAYLOAD_MIN_SIZE], data_len);
#else
			for (k = address; k - address < data_len; ++k) {
				if (k - start_address < bytes)
					mem[k - start_address] = f->packet[FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE + (k - address)];
			}
#endif
			address += data_len;
			fflush(stdout);
		}
	}
	printf("\n");
	gettimeofday(&end, NULL);
	printf("downloaded %d bytes in %.2f seconds (%.2f bytes / second)\n", bytes, (((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5) / 1000., (float)bytes / ((((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5)/1000.));

	f->hex = hex_from_map(mem, tag, bytes, start_address);

	free(mem);
	free(tag);
	
	return 1;
}

int task_flash_eeprom(int argc, char *argv[]) {
	flash_t f;
	uint8_t mode, i, expected_sig[3];
	uint16_t address;
	struct timeval start, end;
	char *hex_filename;

	if (argc != 8) {
TASK_FLASH_EEPROM_USAGE:
		warning("usage: %s %s %s <command: upload or download> <sig byte 0> <sig byte 1> <sig byte 2> <hex filename>\n", argv[0], argv[1], argv[2]);
		return EXIT_FAILURE;
	}

	if (strcmp(argv[3], "upload") == 0)
		mode = 0;
	else if (strcmp(argv[3], "download") == 0)
		mode = 1;
	else {
		goto TASK_FLASH_EEPROM_USAGE;
	}
	hex_filename = argv[7];

	memset(&f, 0, sizeof(flash_t));

	// setup the data structure from constants and arguments
	f.hello_retries = 10;
	for (i = 0; i < 3; ++i)
		expected_sig[i] = (uint8_t)strtoul(argv[i + 4], NULL, 16);
	f.addr.source = flash_pipes[0];
	f.addr.target = flash_pipes[1];

	printf("%s: %s EEPROM via bootloader at address: %llx %s file '%s'\n", argv[0], (mode == 0 ? "uploading" : "downloading"), f.addr.target, (mode == 0 ? "loading from" : "saving to"), hex_filename);

	radio = task_radio_setup(NRF24_PA_MAX, NRF24_1MBPS, FLASH_CHANNEL, f.addr.source, f.addr.target);
#ifdef PI_DEBUG
	nrf24_print_details(radio);
#endif

	if (!task_flash_hello_exchange(&f, expected_sig)) {
		warning("%s: HELLO exchange failed!\n", argv[0]);
		task_flash_print_details(&f);
		return EXIT_FAILURE;
	}

	task_flash_print_details(&f);	

	gettimeofday(&start, NULL);
	if (mode == 0) {
		f.hex = hex_load_from_file(hex_filename);
		for (address = f.hex->min_address; address <= f.hex->max_address; ++address) {
			if (f.hex->tag[address - f.hex->min_address] == HEX_TAG_ALLOC) {
				f.packet[0] = FLASH_EEPROM_PROG;
				f.packet[1] = (uint8_t)(address & 0x00FF);
				f.packet[2] = (uint8_t)((address & 0xFF00) >> 8);
				f.packet[3] = f.hex->mem[address - f.hex->min_address];
				printf(".");
				fflush(stdout);
				if (!task_send_packet(radio, "FLASH_EEPROM_PROG", f.packet, FLASH_EEPROM_PROG_SIZE, FLASH_SEND_POST_DELAY_US, 0))
					return EXIT_FAILURE;
			}
		}
		printf("\n");
	} else {
		uint8_t *map, *tag;
		map = (uint8_t *)malloc(f.eeprom_size);
		tag = (uint8_t *)malloc(f.eeprom_size);
		memset(map, 0xFF, f.eeprom_size);
		memset(tag, HEX_TAG_ALLOC, f.eeprom_size);
		for (address = 0; address < f.eeprom_size; ++address) {
			f.packet[0] = FLASH_EEPROM_READ;
			f.packet[1] = (uint8_t)(address & 0x00FF);
			f.packet[2] = (uint8_t)((address & 0xFF00) >> 8);
			printf(".");
			fflush(stdout);
			if (!task_send_packet(radio, "FLASH_EEPROM_READ", f.packet, FLASH_EEPROM_READ_SIZE, FLASH_SEND_POST_DELAY_US, 0))
				return EXIT_FAILURE;
			if (!task_read_ack_payload(radio, f.packet, FLASH_EEPROM_READ_PAYLOAD, FLASH_EEPROM_READ_PAYLOAD_SIZE)) {
				warning("%s: while reading EEPROM address %d, did not received payload packet\n", argv[0], address);
				return 0;
			}
			map[address] = f.packet[1];
		}
		printf("\n");
		f.hex = hex_from_map(map, tag, f.eeprom_size, 0);
		hex_save_to_file(f.hex, hex_filename);
	}
	gettimeofday(&end, NULL);
	printf("%sloaded %d bytes in %.2f seconds (%.2f bytes / second)\n", (mode == 0 ? "up" : "down"), f.hex->total_bytes, (((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5) / 1000., (float)f.hex->total_bytes / ((((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5)/1000.));

	hex_free(f.hex);

	return EXIT_SUCCESS;
}

static void task_flash_print_details(flash_t *f) {
	printf("flash/bootloader details:\n");
	printf("\t%-20s: 0x%llx\n", "source address", f->addr.source);
	printf("\t%-20s: 0x%llx\n", "target address", f->addr.target);
	printf("\t%-20s: %d\n", "hello retries", f->hello_retries);
	printf("\t%-20s: 0x%x 0x%x 0x%x\n", "signature", f->sig[0], f->sig[1], f->sig[2]);
	printf("\t%-20s: %d.%d\n", "bootloader version", f->version.major, f->version.minor);
	printf("\t%-20s: %d bytes\n", "SPM_PAGESIZE", f->spm_pagesize);
	printf("\t%-20s: %d bytes\n",  "available flash", f->available_flash);
	printf("\t%-20s:\n", "fuses");
	printf("\t\t%-12s: 0x%x\n", "low", f->fuses.low);
	printf("\t\t%-12s: 0x%x\n", "high", f->fuses.high);
	printf("\t\t%-12s: 0x%x\n", "extended", f->fuses.extended);
	printf("\t\t%-12s: 0x%x\n", "lock", f->fuses.lock);
	printf("\t%-20s: %d bytes\n", "EEPROM size", f->eeprom_size);
}

static uint8_t task_flash_hello_exchange(flash_t *f, uint8_t expected_sig[3]) {
	uint8_t i, result = 1;

	// send hello and print results
	f->packet[0] = FLASH_HELLO;
	f->packet[1] = FLASH_VERSION_MAJOR;
	f->packet[2] = FLASH_VERSION_MAJOR;
	f->packet[3] = expected_sig[0];
	f->packet[4] = expected_sig[1];
	f->packet[5] = expected_sig[2];
	for (i = 6; i < FLASH_HELLO_SIZE; ++i)
		f->packet[i] = 0;
	// try to send the hello packet until it succeeds or retries exceeded
	for (i = 0; i < f->hello_retries; ++i)
		if (task_send_packet(radio, "FLASH_HELLO", f->packet, FLASH_HELLO_SIZE, FLASH_SEND_POST_DELAY_US, f->addr.target))
			break;
	if (i == f->hello_retries) {
		warning("maximum number of retries (%d) reached for HELLO packet\n", f->hello_retries);
		return 0;
	}
	f->packet[0] = FLASH_HELLO_FLUSH;
	if (!task_send_packet(radio, "FLASH_HELLO_FLUSH", f->packet, FLASH_HELLO_FLUSH_SIZE, FLASH_SEND_POST_DELAY_US, f->addr.target))
		return 0;
	// read response
	if (!task_read_ack_payload(radio, f->packet, FLASH_HELLO, FLASH_HELLO_SIZE)) {
		// report on a failed command
		if (f->packet[0] == FLASH_CMD_FAILED) {
			warning("FLASH_CMD_FAILED packet received after sending FLASH_HELLO\n");
			switch(f->packet[1]) {
				case FLASH_CMD_FAILED__VERSION_MISMATCH:
					warning("bootloader target reported a version mismatch (sent %d.%d)\n", FLASH_VERSION_MAJOR, FLASH_VERSION_MINOR); 
					break;
				case FLASH_CMD_FAILED__SIGNATURE_MISMATCH:
					warning("bootloader target reported a signature mismatch (sent 0x%x 0x%x 0x%x)\n", expected_sig[0], expected_sig[1], expected_sig[2]);
					break;
				default:
					warning("unknown failure for last command reported by bootloader\n");
					break;
			};
		}
		return 0;
	}
	f->version.major = f->packet[1];
	f->version.minor = f->packet[2];
	// repeat the checks to make sure bidirectional comms are OK
	if (f->packet[1] != FLASH_VERSION_MAJOR) {
		warning("major version mismatch; received %d.%d - expected %d.%d\n", f->packet[1], f->packet[2], FLASH_VERSION_MAJOR, FLASH_VERSION_MINOR);
		result = 0;
	}
	f->sig[0] = f->packet[3];
	f->sig[1] = f->packet[4];
	f->sig[2] = f->packet[5];
	if (f->sig[0] != expected_sig[0] || f->sig[1] != expected_sig[1] || f->sig[2] != expected_sig[2]) {
		warning("signature mismatch; received 0x%x 0x%x 0x%x - expected 0x%x 0x%x 0x%x\n", f->sig[0], f->sig[1], f->sig[2], expected_sig[0], expected_sig[1], expected_sig[2]);
		result = 0;
	}
	f->spm_pagesize = f->packet[6];
	f->available_flash = ((uint16_t)f->packet[8] << 8) | f->packet[7];
	f->fuses.low = f->packet[9];
	f->fuses.high = f->packet[10];
	f->fuses.extended = f->packet[11];
	f->fuses.lock = f->packet[12];
	f->eeprom_size = ((uint16_t)f->packet[14] << 8) | f->packet[13];

	return result;
}
