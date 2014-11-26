#ifndef _PACKET_H_
#define _PACKET_H_

//      2      3      5      7     11     13     17     19     23     29 
//     31     37     41     43     47     53     59     61     67     71 
//     73     79     83     89     97    101    103    107    109    113 
//    127    131    137    139    149    151    157    163    167    173 
//    179    181    191    193    197    199    211    223    227    229 
//    233    239    241    251 

// PACKET TYPES
#define PACKET_RESET			2		// 1 byte
#define PACKET_SET_COLOR		3		// 1 byte + 3*uint8_t (reg,green,blue)
#define PACKET_STOP_PROGRAM		5		// 1 byte
#define PACKET_RUN_PROGRAM		7		// 1 byte
// programming options
#define PACKET_BEGIN_PROGRAMMING	11		// 1 byte
#define PACKET_PROGRAM_LENGTH		13		// 1 byte + uint16_t (length)
#define PACKET_PROGRAM_STEP		17		// 1 byte + uint16_t (current step) + 3*uint8_t (reg,green,blue) + uint16_t (delay in ms)
#define PACKET_END_PROGRAMMING		19		// 1 byte
// read back options
// for each command is a followup flush command
// the first command instructs the client to prime the ack payload fifo
// for the followup flush that actually does the read
#define PACKET_READ_PROGRAM_LEN		23		// 1 byte ||| returns 1 byte + uint16_t (length)
#define PACKET_READ_PROGRAM_LEN_FLUSH	29		// 1 byte
#define PACKET_READ_PROGRAM_STEP	31		// 1 byte + uint16_t (step to read) ||| returns 1 byte + uint16_t (step) + 3*uint8_t (reg,green,blue) + uint16_t (delay in ms)
#define PACKET_READ_PROGRAM_STEP_FLUSH	37		// 1 byte
// light to frequency reading
#define PACKET_READ_LIGHT_FREQ		41		// 1 byte ||| returns 1 byte + uint16_t (freq in hz)
#define PACKET_READ_LIGHT_FREQ_FLUSH	43		// 1 byte
#define	PACKET_LIGHT_FREQ		47		// 1 byte + uint16_t (freq in hz)

// PACKET TYPE SIZES
#define PACKET_RESET_SIZE			(sizeof(uint8_t))
#define PACKET_SET_COLOR_SIZE			(sizeof(uint8_t) + (3 * sizeof(uint8_t)))
#define PACKET_STOP_PROGRAM_SIZE		(sizeof(uint8_t))
#define PACKET_RUN_PROGRAM_SIZE			(sizeof(uint8_t))
#define PACKET_BEGIN_PROGRAMMING_SIZE		(sizeof(uint8_t))
#define PACKET_PROGRAM_LENGTH_SIZE		(sizeof(uint8_t) + sizeof(uint16_t))
#define PACKET_PROGRAM_STEP_SIZE		(sizeof(uint8_t) + sizeof(uint16_t) + (3 * sizeof(uint8_t)) + sizeof(uint16_t))
#define PACKET_END_PROGRAMMING_SIZE		(sizeof(uint8_t))
#define PACKET_READ_PROGRAM_LEN_SIZE		(sizeof(uint8_t))
#define PACKET_READ_PROGRAM_LEN_FLUSH_SIZE	(sizeof(uint8_t))
#define PACKET_READ_PROGRAM_STEP_SIZE		(sizeof(uint8_t) + sizeof(uint16_t))
#define PACKET_READ_PROGRAM_STEP_FLUSH_SIZE	(sizeof(uint8_t))
#define PACKET_READ_LIGHT_FREQ_SIZE		(sizeof(uint8_t))
#define PACKET_READ_LIGHT_FREQ_FLUSH_SIZE	(sizeof(uint8_t))
#define PACKET_LIGHT_FREQ_SIZE			(sizeof(uint8_t) + sizeof(uint16_t))

#define PIPE_0_ADDR	0xF0F0F0F0E1LL
#define PIPE_1_ADDR	0xF0F0F0F0D2LL

#define CHANNEL	100

#endif /* _PACKET_H_ */
