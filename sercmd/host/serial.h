#ifndef _SERIAL_H
#define _SERIAL_H

int serial_init(char *device, int baud);
void serial_send(uint8_t *buff, uint8_t bytes);
uint8_t serial_recv(uint8_t *buff, uint8_t bytes);
void serial_close(void);

#endif /* _SERIAL_H */
