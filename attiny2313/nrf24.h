#ifndef __NRF24_H__
#define __NRF24_H__

#define NRF24_MAX_PAYLOAD_SIZE 32

void nrf24_init(void);
void nrf24_done(void);

uint8_t nrf24_rx_read(uint8_t *buf, uint8_t *pkt_len);
void nrf24_tx_ack_payload(uint8_t *buf, uint8_t len);
uint8_t nrf24_read_status(void);
void nrf24_write_reg(uint8_t addr, uint8_t value);

#endif /* __NRF24_H__ */
