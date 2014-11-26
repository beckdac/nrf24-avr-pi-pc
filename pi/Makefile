## sources
SRCS_PI = main.c
SRCS_UNIVERSAL = error.c file.c program.c hex.c gpio.c spi.c nRF24L01+.c task.c task_program.c task_gpio.c task_nRF24.c task_flash.c
SRCS = $(SRCS_PI)

## objects
OBJS_PI = $(SRCS_PI:.c=.o)
OBJS_UNIVERSAL = $(SRCS_UNIVERSAL:.c=.o)
OBJS = $(SRCS:.c=.o)

## output
BINS = pi

DEFINES = -DUSE_GPIO
CFLAGS = -Wall -std=gnu99 -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -fshort-enums -I. $(DEFINES)
CFLAGS = -Wall -std=gnu99 -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -I. $(DEFINES)
CFLAGS = -Wall -std=gnu99 -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -I. $(DEFINES)
CFLAGS = -Wall -std=gnu99 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -I. $(DEFINES)
CFLAGS = -g -Wall -std=gnu99 -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -I. $(DEFINES)
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

pi: $(OBJS_PI) $(OBJS_UNIVERSAL)
	$(CC) -o $@ $(OBJS_PI) $(OBJS_UNIVERSAL) -lm

test: $(BINS)
	sudo ./pi gpio test 23 65535 5000
	sudo ./pi nRF24 print
	sudo ./pi nRF24 scanner 16
	sudo ./pi nRF24 ping 8
	./pi program parse test.prg
	sudo ./pi program reset
	sudo ./pi program stop
	sudo ./pi program rgb 0 0 0
	sudo ./pi program start
	./pi program rainbow rainbow.prg
	sudo ./pi program upload rainbow.prg
	sudo ./pi program download download.prg
	sudo ./pi program lightfreq
	sudo ./pi program reset; sleep 2; sudo ./pi program stop; sudo ./pi program rgb 0 0 0;
	./pi flash hex ../../blink/blink.hex /tmp/blink.hex
	diff -w ../../blink/blink.hex /tmp/blink.hex
	sudo ./pi flash test 1e 95 f
	sudo ./pi flash download 1e 95 f flash.hex
	sudo ./pi flash upload 1e 95 f ../blink/blink.hex
	sudo ./pi flash eeprom download 1e 95 f /tmp/eeprom.hex
	sudo ./pi flash eeprom upload 1e 95 f /tmp/eeprom.hex

include Makefile.depend
