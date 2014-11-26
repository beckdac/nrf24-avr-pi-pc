#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_VERSION_MAJOR 1
#define FLASH_VERSION_MINOR 0

// addresses
#define FLASH_SOURCE	0xC0FFEE1000LL
#define FLASH_TARGET	0xC0FFEE0010LL
#define FLASH_CHANNEL	113

//      2      3      5      7     11     13     17     19     23     29 
//     31     37     41     43     47     53     59     61     67     71 
//     73     79     83     89     97    101    103    107    109    113 
//    127    131    137    139    149    151    157    163    167    173 
//    179    181    191    193    197    199    211    223    227    229 
//    233    239    241    251 

// packet types
// hello, goodbye
#define FLASH_HELLO			2
#define FLASH_HELLO_FLUSH		3
#define FLASH_DONE			5
// read flash
#define FLASH_PAGE_READ			11
#define FLASH_PAGE_READ_PAYLOAD		13
#define FLASH_PAGE_READ_FLUSH		17
// write flash
#define FLASH_PAGE_PROG			23
#define FLASH_PAGE_PROG_PAYLOAD		29
// read eeprom
#define FLASH_EEPROM_READ		41
#define FLASH_EEPROM_READ_PAYLOAD	43
#define FLASH_EEPROM_READ_FLUSH		47
// write eeprom
#define FLASH_EEPROM_PROG		53
#define FLASH_EEPROM_PROG_PAYLOAD	59
// error
#define FLASH_CMD_FAILED		251

// packet sizes
// hello, goodbye
#define FLASH_HELLO_SIZE		(sizeof(uint8_t) + (2 * sizeof(uint8_t)) + (3 * sizeof(uint8_t)) + sizeof(uint8_t) + sizeof(uint16_t) + (4 * sizeof(uint8_t)) + sizeof(uint16_t))
					// packet type, major ver., minor ver., 3 byte device signature, SPM_PAGESIZE, available flash size, fuses: l, h, e, lock, EEPROM size
#define FLASH_HELLO_FLUSH_SIZE		(sizeof(uint8_t))			// packet type
#define FLASH_DONE_SIZE			(sizeof(uint8_t))			// packet type
// read flash
#define FLASH_PAGE_READ_SIZE		(sizeof(uint8_t) + sizeof(uint16_t))	// packet type, address
#define FLASH_PAGE_READ_PAYLOAD_MIN_SIZE (sizeof(uint8_t) + sizeof(uint8_t))	// packet type, sequence no, [data]
#define FLASH_PAGE_READ_FLUSH_SIZE	(sizeof(uint8_t))			// packet type
// write flash
#define FLASH_PAGE_PROG_SIZE		(sizeof(uint8_t) + sizeof(uint16_t))	// packet type, address
#define FLASH_PAGE_PROG_PAYLOAD_MIN_SIZE (sizeof(uint8_t) + sizeof(uint8_t))	// packet type, sequence no, [data]
// read eeprom
#define FLASH_EEPROM_READ_SIZE		(sizeof(uint8_t) + sizeof(uint16_t))	// packet type, address
#define FLASH_EEPROM_READ_PAYLOAD_SIZE	(sizeof(uint8_t) + sizeof(uint8_t))	// packet type, byte
#define FLASH_EEPROM_READ_FLUSH_SIZE	(sizeof(uint8_t))
// write eeprom
#define FLASH_EEPROM_PROG_SIZE		(sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t))
					// packet type, address, byte
// error
#define FLASH_CMD_FAILED_SIZE		(sizeof(uint8_t) + sizeof(uint8_t))	// packet type, reason

// failed command reasons
#define FLASH_CMD_FAILED__VERSION_MISMATCH	2
#define FLASH_CMD_FAILED__SIGNATURE_MISMATCH	3

#endif /* __FLASH_H__ */
