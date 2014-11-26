## deployment specific defines
DEVICE     = atmega328
#CLOCK      = 16000000L
#CLOCK      = 20000000L
# for internal oscillator
CLOCK      = 8000000UL

## compile time defines
FLASH_SIZE_KB         = 32
BOOT_SECTION_SIZE_KB  = 2
CALC_ADDRESS_IN_HEX   = $(shell printf "0x%X" $$(( $(1) )) )
BOOT_START            = $(call CALC_ADDRESS_IN_HEX, ($(FLASH_SIZE_KB) - $(BOOT_SECTION_SIZE_KB)) * 1024 )
CALC_AVAILABLE_FLASH  = $(shell printf "%d" $$(( $(1) )) )
AVAILABLE_FLASH       = $(call CALC_AVAILABLE_FLASH, ($(FLASH_SIZE_KB) - $(BOOT_SECTION_SIZE_KB)) * 1024 )

## sources
SRCS_BOOTLOADER = main.c
SRCS_APP	= 
SRCS_UNIVERSAL = spi.c nrf24.c
SRCS = $(SRCS_CORE) $(SRCS_UNIVERSAL)

## objects
OBJS_BOOTLOADER = $(SRCS_BOOTLOADER:.c=.o)
OBJS_APP = $(SRCS_APP:.c=.o)
OBJS_UNIVERSAL = $(SRCS_UNIVERSAL:.c=.o)
OBJS = $(OBJS_BOOTLOADER) $(OBJS_APP) $(OBJS_UNIVERSAL)

## bins
ELFS = bootloader.elf 
BINS = $(ELFS:.elf=.hex) combined.hex

DEFINES = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -DAVAILABLE_FLASH=$(AVAILABLE_FLASH)U -DDEBUG=1
DEFINES = -mmcu=$(DEVICE) -DF_CPU=$(CLOCK) -DAVAILABLE_FLASH=$(AVAILABLE_FLASH)U
CFLAGS = -Wall -std=gnu99 -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -fshort-enums -I. -Os $(DEFINES)
CFLAGS = -Wall -Os -fno-inline-small-functions -fno-split-wide-types -mshort-calls $(DEFINES)
CFLAGS = -Wall -Os -fno-inline-small-functions -fno-split-wide-types -mrelax $(DEFINES)
LINK = -Wl,--section-start=.text=$(BOOT_START) -Wl,--relax -Wl,--gc-sections -nostartfiles -nostdlib
LINK = -Wl,--section-start=.text=$(BOOT_START) -Wl,--relax -Wl,--gc-sections -Wl,--print-gc-sections -nostdlib
LINK = -Wl,--section-start=.text=$(BOOT_START) -Wl,--relax -nostdlib
LINK = -Wl,--section-start=.text=$(BOOT_START) -Wl,--relax

CC = avr-gcc $(CFLAGS)

all: $(BINS)

.c.o: Makefile
	$(CC) -c $< -o $@

clean: depend
	rm -f $(ELFS) $(BINS) $(OBJS)

bootloader.elf: $(OBJS_BOOTLOADER) $(OBJS_UNIVERSAL)
	$(CC) $(LINK) -o $@ $(OBJS_BOOTLOADER) $(OBJS_UNIVERSAL) 

app.elf: $(OBJS_APP) $(OBJS_UNIVERSAL)
	$(CC) -Wl,--relax -o $@ $(OBJS_APP) $(OBJS_UNIVERSAL)

combined.hex: ../blink/blink.hex bootloader.hex
	srec_cat ../blink/blink.hex -I bootloader.hex -I -o combined.hex -I

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
