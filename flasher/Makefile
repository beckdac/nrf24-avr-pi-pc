## sources
SRCS = main.c error.c task.c task_flash.c task_nRF24.c hex.c nRF24L01+.c file.c spi.c

## objects
OBJS = $(SRCS:.c=.o)

## output
BINS = nrf24flash

DEFINES = -DUSE_FTDI
INCLUDE = -I /usr/include/libusb-1.0
CFLAGS = -g -Wall -std=gnu99 -funsigned-char -ffunction-sections -fpack-struct $(INCLUDE) $(DEFINES)
LIB = -L/usr/lib/x86_64-linux-gnu -lftdi
CC = gcc $(CFLAGS)

all: $(BINS) 

.c.o: Makefile
	$(CC) -c $< -o $@

Makefile.depend:
	touch Makefile.depend

depend: Makefile.depend
	makedepend -fMakefile.depend -- $(CFLAGS) -- $(SRCS)

clean: depend
	rm -f $(ELFS) $(BINS) $(OBJS) $(OBJS_UNIVERSAL)

nrf24flash: $(OBJS) 
	$(CC) -o $@ $(OBJS) $(LIB)

test: nrf24flash
	sudo ./nrf24flash nRF24 print
	sudo ./nrf24flash nRF24 scanner 16
	sudo ./nrf24flash nRF24 nrf24flashng 8
	sudo ./nrf24flash flash test 1e 95 f
	sudo ./nrf24flash flash download 1e 95 f flash.hex
	sudo ./nrf24flash flash upload 1e 95 f ../blink/blink.hex
	sudo ./nrf24flash flash eeprom download 1e 95 f /tmp/eeprom.hex
	sudo ./nrf24flash flash eeprom upload 1e 95 f /tmp/eeprom.hex

include Makefile.depend
