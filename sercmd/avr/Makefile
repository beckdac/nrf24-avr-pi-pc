## deployment specific defines
DEVICE     = attiny2313
# for internal oscillator
CLOCK      = 8000000UL

## compile time defines
## sources
SRCS_SERCMD	= main.c
SRCS_UNIVERSAL = spi.c usart.c
SRCS = $(SRCS_CORE) $(SRCS_UNIVERSAL)

## objects
OBJS_SERCMD = $(SRCS_SERCMD:.c=.o)
OBJS_UNIVERSAL = $(SRCS_UNIVERSAL:.c=.o)
OBJS = $(OBJS_BOOTLOADER) $(OBJS_SERCMD) $(OBJS_UNIVERSAL)

## bins
ELFS = sercmd.hex
BINS = $(ELFS:.elf=.hex)

INCLUDE = -I ../include
DEFINES = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -DAVAILABLE_FLASH=$(AVAILABLE_FLASH)U -DDEBUG=1
DEFINES = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -DAVAILABLE_FLASH=$(AVAILABLE_FLASH)U -DBAUD=38400
DEFINES = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -DAVAILABLE_FLASH=$(AVAILABLE_FLASH)U -DBAUD=500000
CFLAGS = -Wall -std=gnu99 -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -fshort-enums -I. -Os $(DEFINES)
CFLAGS = -Wall -Os -fno-inline-small-functions -fno-split-wide-types -mshort-calls $(DEFINES)
CFLAGS = -Wall -Os -fno-inline-small-functions -fno-split-wide-types -mrelax $(DEFINES) $(INCLUDE)
LINK = -Wl,--relax

CC = avr-gcc $(CFLAGS)

all: $(BINS)

.c.o: Makefile
	$(CC) -c $< -o $@

clean: depend
	rm -f $(ELFS) $(BINS) $(OBJS)

sercmd.elf: $(OBJS_SERCMD) $(OBJS_UNIVERSAL)
	$(CC) -Wl,--relax -o $@ $(OBJS_SERCMD) $(OBJS_UNIVERSAL)

%.hex: %.elf
	\rm -rf $@
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-objdump -h -S $< > $<.lst
	avr-size $@
	\ls -lh $@

Makefile.depend:
	touch Makefile.depend

depend: Makefile.depend
	makedepend -fMakefile.depend -- $(CFLAGS) -- $(SRCS) 

include Makefile.depend
