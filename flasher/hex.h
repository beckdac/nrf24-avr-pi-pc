#ifndef _HEX_H_
#define _HEX_H_

typedef struct hex_record {
	uint8_t byte_count;
	uint16_t address;
	uint8_t record_type;
	uint8_t *data;
	uint8_t checksum;
	char *line;
} hex_record_t;

#define HEX_TAG_UNALLOC	0
#define HEX_TAG_ALLOC	1

//#define HEX_DEFAULT_BYTE_COUNT	32
#define HEX_DEFAULT_BYTE_COUNT	16

typedef struct hex {
	uint16_t records;
	hex_record_t **record;
	uint16_t min_address;
	uint16_t max_address;
	uint16_t address_range;
	uint16_t total_bytes;
	uint8_t *mem;
	uint8_t	*tag;
} hex_t;

hex_t *hex_load_from_file(char *filename);
void hex_free(hex_t *p);
hex_t *hex_from_map(uint8_t *mem, uint8_t *tag, uint16_t bytes, uint16_t min_address);
void hex_save_to_file(hex_t *h, char *filename);

#endif /* _HEX_H_ */
