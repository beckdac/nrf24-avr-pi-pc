#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "error.h"
#include "file.h"
#include "hex.h"

hex_t *hex_load_from_file(char *filename) {
	file_t *f;
	hex_t *h;
	char line[1024];
	int line_no = 0, fields, i, j;
	uint8_t checksum, eof = 0;
	hex_record_t *r;

	f = file_open(filename, "r");

	printf("reading hex from file '%s'\n", f->filename);

	h = malloc(sizeof(hex_t));
	memset(h, 0, sizeof(hex_t));
	h->min_address = UINT16_MAX;

	while (fgets(line, 1024, f->fp) != NULL) {
		++line_no;
		if (line[0] == '#')
			continue;
		if (eof) {
			warning("in file '%s' on line %d: additional lines found in file after EOF record on line %d\n", f->filename, line_no, eof);
			exit(EXIT_FAILURE);
		}
		r = malloc(sizeof(hex_record_t));
		r->line = strdup(line);
		// parse the line
		if (strlen(r->line) < (1 + 2 + 4 + 2 + 2)) {	// : + byte_count + address + record_type + checksum
			warning("in file '%s' on line %d: line too short for minimum content\n", f->filename, line_no);
			exit(EXIT_FAILURE);
		}
		fields = sscanf(line, ":%02hhX%04hx%02hhX", &r->byte_count, &r->address, &r->record_type);
		if (fields != 3) {
			warning("in file '%s' on line %d: unable to read first three fields on line: byte_count, address, and record_type\n", f->filename, line_no);
			exit(EXIT_FAILURE);
		}
		if (r->record_type == 0x0) {
			// normal
			if (r->address < h->min_address) {
				h->min_address = r->address;
			}
			if (r->address + (r->byte_count - 1) > h->max_address) {
				h->max_address = r->address + (r->byte_count - 1);
			}
		} else if (r->record_type == 0x1) {
			if (r->byte_count != 0) {
				warning("in file '%s' on line %d: for record type %04hX (EOF), byte count must = 0 (found %d)\n", f->filename, line_no, r->record_type, r->byte_count);
				exit(EXIT_FAILURE);
			} else if (r->address != 0) {
				warning("in file '%s' on line %d: for record type %04hX (EOF), address must = 0x0000 (found %0xhX)\n", f->filename, line_no, r->record_type, r->address);
				exit(EXIT_FAILURE);
			}
			eof = line_no;
		} else {
			warning("in file '%s' on line %d: unhandled record type %04hX (%d)\n", f->filename, line_no, r->record_type, r->record_type);
			exit(EXIT_FAILURE);
		}
//		printf("parsed %d fields: 0x%x 0x%x 0x%x\n", fields, r->byte_count, r->address, r->record_type);
		if (strlen(r->line) < (1 + 2 + 4 + 2 + 2 + (2 * r->byte_count))) {
			warning("in file '%s' on line %d: line too short for minimum content and specified byte count (%d)\n", f->filename, line_no, r->byte_count);
			exit(EXIT_FAILURE);
		}
		if (r->byte_count > 0) {
			r->data = (uint8_t *)malloc(r->byte_count * sizeof(uint8_t));
			for (i = 0; i < r->byte_count; ++i) {
				sscanf(&line[9 + (i * 2)], "%02hhX", &r->data[i]);
			}
		} else {
			i = 0;
		}
		// checksum
		sscanf(&line[9 + (i * 2)], "%02hhX", &r->checksum);
		checksum = r->byte_count + ((r->address & 0xFF00) >> 8) + (r->address & 0x00FF) + r->record_type;
		for (i = 0; i < r->byte_count; ++i) {
			checksum += r->data[i];
		}
//		checksum = ~checksum + 1;
//		if (checksum != r->checksum) ...
//		alternately
		if (((checksum + r->checksum) & 0xFF) != 0) {
			warning("in file '%s' on line %d: checksum failed!\n", f->filename, line_no);
			exit(EXIT_FAILURE);
		}

		h->records++;
		h->record = realloc(h->record, sizeof(hex_record_t *) * h->records);
		h->record[h->records - 1] = r;
		h->total_bytes += r->byte_count;
	}
	h->address_range = (h->max_address - h->min_address) + 1;
	h->mem = (uint8_t *)malloc((h->max_address + 1) * sizeof(uint8_t));
	h->tag = (uint8_t *)malloc((h->max_address + 1) * sizeof(uint8_t));
	memset(h->mem, 0xFF, (h->max_address + 1) * sizeof(uint8_t));
	memset(h->tag, HEX_TAG_UNALLOC, (h->max_address + 1) * sizeof(uint8_t));
	for (i = 0; i < h->records; ++i) {
		uint16_t addr;
		r = h->record[i];
		addr = r->address;
		for (j = 0; j < r->byte_count; ++j) {
			h->mem[addr] = r->data[j];
			h->tag[addr++] = HEX_TAG_ALLOC;
		}
	}

	printf("hex: %s\n", f->filename);
	printf("\t%-20s: %d\n", "records", h->records);
	printf("\t%-20s: %d\n", "total bytes", h->total_bytes);
	printf("\t%-20s: %04hX (%d)\n", "min address", h->min_address, h->min_address);
	printf("\t%-20s: %04hX (%d)\n", "max address", h->max_address, h->max_address);
	printf("\t%-20s: %04hX (%d)\n", "address range", h->address_range, h->address_range);

	file_close(f);

	return h;
}

void hex_free(hex_t *h) {
	int i;

	free(h->mem);
	free(h->tag);
	for (i = 0; i < h->records; ++i) {
		free(h->record[i]->line);
		if (h->record[i]->byte_count)
			free(h->record[i]->data);
		free(h->record[i]);
	}
	free(h->record);	
	free(h);
}

void hex_from_map_add_record(hex_t *h, uint16_t start, uint16_t end) {
	hex_record_t *r;
	uint16_t i;
	uint8_t line_len;

	r = (hex_record_t *)malloc(sizeof(hex_record_t));
	h->record = (hex_record_t **)realloc(h->record, (h->records + 1) * sizeof(hex_record_t *));
	h->record[h->records++] = r;
	r->address = start + h->min_address;
	r->byte_count = (end - start) + 1;
	r->record_type = 0;
	r->data = (uint8_t *)malloc(sizeof(uint8_t) * r->byte_count);
	for (i = start; i <= end; ++i) {
		r->data[i - start] = h->mem[i];
	}
	r->checksum = r->byte_count + ((r->address & 0xFF00) >> 8) + (r->address & 0x00FF) + r->record_type;
	for (i = 0; i < r->byte_count; ++i) {
		r->checksum += r->data[i];
	}
	r->checksum = ~r->checksum + 1;

	line_len = (1 + 2 + 4 + 2 + 2 + (2 * r->byte_count) + 1);
	r->line = (char *)malloc(sizeof(char) * line_len);
	sprintf(r->line, ":%02hhX%04hx%02hhX", r->byte_count, r->address, r->record_type);
	for (i = 0; i < r->byte_count; ++i)
		sprintf(&r->line[(i * 2) + 9], "%02hhX", r->data[i]);
	sprintf(&r->line[(i * 2) + 9], "%02hhX", r->checksum);
}

hex_t *hex_from_map(uint8_t *mem, uint8_t *tag, uint16_t bytes, uint16_t min_address) {
	hex_t *h;
	uint16_t i, start = 0;
	uint8_t last_tag = HEX_TAG_UNALLOC, consecutive_allocs = 0;
	
	h = malloc(sizeof(hex_t));
	memset(h, 0, sizeof(hex_t));
	h->min_address = min_address;
	h->max_address = min_address + (bytes - 1);
	h->address_range = (h->max_address - h->min_address) + 1;

	h->mem = (uint8_t *)malloc(sizeof(uint8_t) * bytes);
	memcpy(h->mem, mem, sizeof(uint8_t) * bytes);
	h->tag = (uint8_t *)malloc(sizeof(uint8_t) * bytes);
	memcpy(h->tag, tag, sizeof(uint8_t) * bytes);

	printf("hex_from_map: %d byte mem map\n", bytes);

	for (i = 0; i < bytes; ++i) {
		if (h->tag[i] == HEX_TAG_UNALLOC && last_tag == HEX_TAG_ALLOC) {
			// create a new record up through from start to i - 1
			hex_from_map_add_record(h, start, i - 1);
			consecutive_allocs = 0;
		} else if (consecutive_allocs >= HEX_DEFAULT_BYTE_COUNT) {
			hex_from_map_add_record(h, start, i - 1);
			start = i;
			consecutive_allocs = 0;
		} else if (h->tag[i] == HEX_TAG_ALLOC && last_tag == HEX_TAG_UNALLOC) {
			start = i;
		}
		last_tag = h->tag[i];
		if (h->tag[i] == HEX_TAG_ALLOC) {
			consecutive_allocs++;
			h->total_bytes++;
		}
	}
	if (h->tag[i - 1] == HEX_TAG_ALLOC) {
		if (last_tag == HEX_TAG_ALLOC) {
			hex_from_map_add_record(h, start, i - 1);
		} else {
			hex_from_map_add_record(h, i - 1, i - 1);
		}
	}

	printf("created %d records\n", h->records);

	return h;
}

void hex_save_to_file(hex_t *h, char *filename) {
	int i;
	file_t *f;

	f = file_open(filename, "w");

	for (i = 0; i < h->records; ++i) {
		fprintf(f->fp, "%s\n",  h->record[i]->line);
	}
	fprintf(f->fp, ":00000001FF\n");

	file_close(f);
}
