## sources
SRCS_SERCMD_TEST = main.c
SRCS_UNIVERSAL = error.c serial.c sercmd.c nRF24L01+.c
SRCS = $(SRCS_SERCMD_TEST)

## objects
OBJS_SERCMD_TEST = $(SRCS_SERCMD_TEST:.c=.o)
OBJS_UNIVERSAL = $(SRCS_UNIVERSAL:.c=.o)
OBJS = $(SRCS:.c=.o)

## output
BINS = sercmd_test

INCLUDE = -I ../include
DEFINES = -DUSE_SERCMD
CFLAGS = -g -Wall -std=gnu99 $(DEFINES) $(INCLUDE)
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

sercmd_test: $(OBJS_SERCMD_TEST) $(OBJS_UNIVERSAL)
	$(CC) -o $@ $(OBJS_SERCMD_TEST) $(OBJS_UNIVERSAL) -lm

test: $(BINS)
	sudo ./sercmd_test /dev/cu.usbserial-A603HRFF 38400
#	sudo ./sercmd_test /dev/cu.usbserial-A603HRFF 500000

include Makefile.depend
