#include <avr/io.h>
#include <util/delay.h>

#include "spi.h"
#include "nrf24.h"
#include "nRF24L01.h"
#include "flash.h"

#define nrf24_csn(state) PORTB = (PORTB & ~(1 << PORTB0)) | (state << PORTB0)

#define nrf24_ce(state) PORTB = (PORTB & ~(1 << PORTB1)) | (state << PORTB1)

static uint8_t nrf24_read_reg(uint8_t addr) {
	uint8_t ret;

	nrf24_csn(0);

	spi_transfer(addr | R_REGISTER);
	ret = spi_transfer(0);

	nrf24_csn(1);

	return ret;
}

static void nrf24_write_addr_reg(uint8_t addr, const uint8_t address[5]) {
	nrf24_csn(0);

	spi_transfer(addr | W_REGISTER);
	spi_transfer(address[0]);
	spi_transfer(address[1]);
	spi_transfer(address[2]);
	spi_transfer(address[3]);
	spi_transfer(address[4]);

	nrf24_csn(1);
}

static uint8_t nrf24_tx_flush(void) {
        uint8_t ret;

        nrf24_csn(0);

        ret = spi_transfer(FLUSH_TX);

        nrf24_csn(1);

        return ret;
}

static uint8_t nrf24_rx_flush(void) {
	uint8_t ret;

	nrf24_csn(0);

	ret = spi_transfer(FLUSH_RX);

	nrf24_csn(1);

	return ret;
}

void nrf24_init(void) {
	const uint8_t tx_addr[5] = { (uint8_t)(FLASH_TARGET), (uint8_t)(FLASH_TARGET >> 8), (uint8_t)(FLASH_TARGET >> 16), (uint8_t)(FLASH_TARGET >> 24), (uint8_t)(FLASH_TARGET >> 32) };
	const uint8_t rx_addr[5] = { (uint8_t)(FLASH_SOURCE), (uint8_t)(FLASH_SOURCE >> 8), (uint8_t)(FLASH_SOURCE >> 16), (uint8_t)(FLASH_SOURCE >> 24), (uint8_t)(FLASH_SOURCE >> 32) };

	// setup spi
	spi_init();

	// setup csn and ce
	// csn_pin is hardcoded to PB0
	// ce_pin is hardcoded to PB1
	DDRB |= (1 << DDB0) | (1 << DDB1);
	PORTB |= (1 << PORTB0);	// set csn high
	PORTB &= ~(1 << PORTB1);// set ce low
	// setup IRQ on PD3 as input
	DDRD &= ~(1 << DDD3);	// set IRQ as input

	_delay_ms(5);

	// wait 4000us and 15 retries
	nrf24_write_reg(SETUP_RETR, 0xff);
	// max power level and 1 MBPS data rate
	nrf24_write_reg(RF_SETUP, (1 << RF_PWR_LOW) | (1 << RF_PWR_HIGH));
	// dynamic payload length for pipes 0 and 1
	nrf24_write_reg(DYNPD, (1 << DPL_P1) | (1 << DPL_P0));
	// enable dynamic payloads and ack payload
	nrf24_write_reg(FEATURE, (1 << EN_DPL) | (1 << EN_ACK_PAY));
	// set the channel
	nrf24_write_reg(RF_CH, FLASH_CHANNEL);
	// set the address width to 5 bytes
	nrf24_write_reg(SETUP_AW, (1 << 1) | (1 << 0));
	// enable auto ack on pipes 0 and 1
	nrf24_write_reg(EN_AA, (1 << ENAA_P1) | (1 << ENAA_P0));

	// set addresses
	// write (tx)
	nrf24_write_addr_reg(RX_ADDR_P0, tx_addr);
	nrf24_write_reg(RX_PW_P0, NRF24_MAX_PAYLOAD_SIZE);
	nrf24_write_addr_reg(TX_ADDR, tx_addr);
	// read (rx)
	nrf24_write_addr_reg(RX_ADDR_P1, rx_addr);
	nrf24_write_reg(RX_PW_P1, NRF24_MAX_PAYLOAD_SIZE);
	nrf24_write_reg(EN_RXADDR, (1 << ERX_P1));

	// enable IRQ and 16 bit CRC
	// also power down and put in rx
	// ((1 << EN_CRC) | (1 << CRCO) | (1 << PRIM_RX) & ~((1 << MASK_RX_DR) | (1 << MASK_TX_DS) | (1 << MASK_MAX_RT) | (1 << PWR_UP))
	nrf24_write_reg(CONFIG, (1 << EN_CRC) | (1 << CRCO) | (1 << PRIM_RX));

	// power up and put in receive
	nrf24_write_reg(CONFIG, nrf24_read_reg(CONFIG) | (1 << PWR_UP) | (1 << PRIM_RX));

	// reset status bits
	nrf24_write_reg(STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));

	nrf24_ce(1);

	// wait for stabilization
	_delay_us(130);
}

void nrf24_done(void) {
	nrf24_ce(0);

	nrf24_tx_flush();
	nrf24_rx_flush();
}

static uint8_t nrf24_rx_data_avail(void) {
        uint8_t ret;

        nrf24_csn(0);

        spi_transfer(R_RX_PL_WID);
        ret = spi_transfer(0);

        nrf24_csn(1);

        return ret;
}

uint8_t nrf24_rx_read(uint8_t *buf, uint8_t *pkt_len) {
        uint8_t len;

        nrf24_write_reg(STATUS, 1 << RX_DR);

        len = nrf24_rx_data_avail();
        *pkt_len = len;

        nrf24_csn(0);

        spi_transfer(R_RX_PAYLOAD);
        while (len --)
                *buf++ = spi_transfer(0);

        nrf24_csn(1);

	return(nrf24_read_reg(FIFO_STATUS) & (1 << RX_EMPTY));
}

void nrf24_tx_ack_payload(uint8_t *buf, uint8_t len) {
	nrf24_csn(0);

	spi_transfer(W_ACK_PAYLOAD | 1);	// pipe 1
	while (len --)
		spi_transfer(*buf++);

	nrf24_csn(1);
}

uint8_t nrf24_read_status(void) {
	uint8_t ret;

	nrf24_csn(0);

	ret = spi_transfer(NOP);

	nrf24_csn(1);

	return ret;
}

void nrf24_write_reg(uint8_t addr, uint8_t value) {
	nrf24_csn(0);

	spi_transfer(addr | W_REGISTER);
	spi_transfer(value);

	nrf24_csn(1);
}
