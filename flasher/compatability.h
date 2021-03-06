#ifndef __COMPATABILITY_H__
#define __COMPATABILITY_H__

#include <sys/time.h>

#define _BV(bit) (1 << (bit))

#define printf_P printf
#define pgm_read_word(p) (*(p))
#define pgm_read_byte(p) (*(p))
#ifdef __APPLE__
#define PROGMEM __attribute__(( section(".progmem.data,PROGMEM") ))
#else
#define PROGMEM __attribute__(( section(".progmem.data") ))
#endif
#define PSTR(s) (__extension__({static const char __c[] PROGMEM = (s); &__c[0];}))

#define HIGH	1
#define LOW	0

#endif /* __COMPATABILITY_H__ */
