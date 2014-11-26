#ifndef __SERCMD_H__
#define __SERCMD_H__

#define SERCMD__NOP		0
#define SERCMD__RESET		1
#define SERCMD__SPI		2
#define SERCMD__ECHO		3
// read byte register commands
#define SERCMD__READ_DDRA	8
#define SERCMD__READ_DDRB	9
#define SERCMD__READ_DDRD	10
#define SERCMD__READ_PORTA	11
#define SERCMD__READ_PORTB	12
#define SERCMD__READ_PORTD	13
#define SERCMD__READ_PINA	14
#define SERCMD__READ_PINB	15
#define SERCMD__READ_PIND	16
// write byte register commands
#define SERCMD__WRITE_DDRA	32
#define SERCMD__WRITE_DDRB	33
#define SERCMD__WRITE_DDRD	34
#define SERCMD__WRITE_PORTA	35
#define SERCMD__WRITE_PORTB	36
#define SERCMD__WRITE_PORTD	37
#define SERCMD__WRITE_PINA	38
#define SERCMD__WRITE_PINB	39
#define SERCMD__WRITE_PIND	40

#ifndef __AVR__

#define SERCMD__DDR_OUT	1
#define SERCMD__DDR_IN	0

void sercmd_init(char *filename, int baud);
void sercmd_done(void);

uint8_t sercmd_ddr_read(uint8_t port);
uint8_t sercmd_port_read(uint8_t port);
uint8_t sercmd_pin_read(uint8_t port);
void sercmd_ddr_pin(uint8_t port, uint8_t pin, uint8_t dir);
void sercmd_port_set(uint8_t port, uint8_t pin, uint8_t state);

uint8_t sercmd_spi_transfer(uint8_t *buf, uint8_t len);

void sercmd_sync(void);

// attiny2313 defines
#define PIND 0x010
#define PIND0 0
#define PIND1 1
#define PIND2 2
#define PIND3 3
#define PIND4 4
#define PIND5 5
#define PIND6 6

#define DDRD 0x011
#define DDD0 0
#define DDD1 1
#define DDD2 2
#define DDD3 3
#define DDD4 4
#define DDD5 5
#define DDD6 6

#define PORTD 0x012
#define PORTD0 0
#define PORTD1 1
#define PORTD2 2
#define PORTD3 3
#define PORTD4 4
#define PORTD5 5
#define PORTD6 6

#define PINB 0x016
#define PINB0 0
#define PINB1 1
#define PINB2 2
#define PINB3 3
#define PINB4 4
#define PINB5 5
#define PINB6 6
#define PINB7 7

#define DDRB 0x017
#define DDB0 0
#define DDB1 1
#define DDB2 2
#define DDB3 3
#define DDB4 4
#define DDB5 5
#define DDB6 6
#define DDB7 7

#define PORTB 0x018
#define PORTB0 0
#define PORTB1 1
#define PORTB2 2
#define PORTB3 3
#define PORTB4 4
#define PORTB5 5
#define PORTB6 6
#define PORTB7 7

#define PINA 0x019
#define PINA0 0
#define PINA1 1
#define PINA2 2

#define DDRA 0x01A
#define DDA0 0
#define DDA1 1
#define DDA2 2

#define PORTA 0x01B
#define PORTA0 0
#define PORTA1 1
#define PORTA2 2


#endif

#endif /* __SERCMD_H__ */
