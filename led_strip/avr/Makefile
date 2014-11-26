## deployment specific defines
DEVICE     = atmega328
#CLOCK      = 16000000L
#CLOCK      = 20000000L
# for internal oscillator
CLOCK      = 8000000L

## compile time defines
USART_BAUD=38400

## sources
SRCS_AVR0 = avr0.c
SRCS_AVR1 = avr1.c
SRCS_AVR2 = avr2.c
SRCS_UNIVERSAL = usart.c circbuf.c spi.c timer0.c program.c rgb.c nRF24L01+.c timer1.c
SRCS = $(SRCS_CORE) $(SRCS_UNIVERSAL)

## objects
OBJS_AVR0 = $(SRCS_AVR0:.c=.o)
OBJS_AVR1 = $(SRCS_AVR1:.c=.o)
OBJS_AVR2 = $(SRCS_AVR2:.c=.o)
OBJS_UNIVERSAL = $(SRCS_UNIVERSAL:.c=.o)
OBJS = $(OBJS_AVR0) $(OBJS_AVR1) $(OBJS_UNIVERSAL)

## bins
ELFS = avr0.elf avr1.elf avr2.elf
BINS = $(ELFS:.elf=.hex)

DEFINES = -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -DUSART_BAUD=$(USART_BAUD)
DEFINES = -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -DUSART_BAUD=$(USART_BAUD) -DDEBUG=1
CFLAGS = -Wall -std=gnu99 -funsigned-char -funsigned-bitfields -ffunction-sections -fpack-struct -fshort-enums -I. -Os $(DEFINES)
CC = avr-gcc $(CFLAGS)
LINK = -Wl,-u,vfprintf -lprintf_flt -lm

all: $(BINS)

.c.o: Makefile
	$(CC) -c $< -o $@

clean: depend
	rm -f $(ELFS) $(BINS) $(OBJS)

avr0.elf: $(OBJS_AVR0) $(OBJS_UNIVERSAL)
	$(CC) -o $@ $(OBJS_AVR0) $(OBJS_UNIVERSAL) $(LINK)

avr1.elf: $(OBJS_AVR1) $(OBJS_UNIVERSAL)
	$(CC) -o $@ $(OBJS_AVR1) $(OBJS_UNIVERSAL) $(LINK)

avr2.elf: $(OBJS_AVR2) $(OBJS_UNIVERSAL)
	$(CC) -o $@ $(OBJS_AVR2) $(OBJS_UNIVERSAL) $(LINK)

%.hex: %.elf
	\rm -rf $@
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size $@
	\ls -lh $@

Makefile.depend:
	touch Makefile.depend

depend: Makefile.depend
	makedepend -fMakefile.depend -- $(CFLAGS) -- $(SRCS) 

include Makefile.depend
