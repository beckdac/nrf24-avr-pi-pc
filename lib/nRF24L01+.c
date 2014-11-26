// do some checks to make sure at least on sane configuration is defined
#if !defined(__AVR__) && !defined(USE_SERCMD) && !defined(USE_FTDI) && !defined(USE_GPIO)
#error at least one of __AVR__, USE_SERCMD, USE_FTDI or USE_GPIO must be defined!
#endif

// headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef __AVR__
#include <util/delay.h>
#include <avr/pgmspace.h>
#else
#include <unistd.h>
#endif
#ifdef USE_FTDI
#include <ftdi.h>
#include <libusb.h>
#endif

#include "nRF24L01+.h"
//#ifdef __AVR__
//#include "usart.h"
//#endif
#ifdef USE_SERCMD
#include "serial.h"
#include "sercmd.h"
#else
#include "spi.h"
#endif
#ifdef USE_GPIO
#include "gpio.h"
#endif

#include "compatability.h"

// function mappings
#ifdef USE_FTDI
#define spi_transfer(radio, buf, len)	spi_send_packet(buf, len)
#elif USE_SERCMD
#define spi_transfer(radio, buf, len)	sercmd_spi_transfer(buf, len)
#elif USE_GPIO
extern spi_t spi[2];
#elif __AVR__
#define spi_transfer(radio, buf, len)	spi_send_packet(len, buf)
#endif

#ifdef USE_SERCMD
nrf24_t *nrf24_init(char *serial_device, int serial_baud) {
#elif USE_FTDI
nrf24_t *nrf24_init(void) {
#elif USE_GPIO
nrf24_t *nrf24_init(uint8_t spi_device, uint8_t csn_pin, uint8_t ce_pin, uint8_t irq_pin) {
#elif __AVR__
nrf24_t *nrf24_init(void) {
#endif
	nrf24_t *radio;
	uint8_t reg_buf;

	radio = malloc(sizeof(nrf24_t));
	memset(radio, 0, sizeof(nrf24_t));

#ifdef USE_SERCMD
#elif USE_GPIO
	radio->spi_device = spi_device;
	radio->ce_pin = ce_pin;
	radio->csn_pin = csn_pin;
	radio->irq_pin = irq_pin;
#endif
	radio->payload_size = NRF24__MAX_PAYLOAD_SIZE;

#ifdef USE_SERCMD
	sercmd_init(serial_device, serial_baud);
	// CSN
	sercmd_ddr_pin(DDRB, DDB0, SERCMD__DDR_OUT);
	// CE
	sercmd_ddr_pin(DDRB, DDB1, SERCMD__DDR_OUT);
	// IRQ
	sercmd_ddr_pin(DDRD, DDD3, SERCMD__DDR_IN);

//	nrf24_csn(radio,HIGH);
//	nrf24_ce(radio, LOW);
#elif USE_FTDI
	spi_init();
//	spi_csn(HIGH);
#elif USE_GPIO
	spi_init(spi_device, SPI_MODE_0, SPI_BPW_8, SPI_8MHZ);

	gpio_export(radio->csn_pin);
	gpio_direction(radio->csn_pin, GPIO_DIR_OUT);
//	gpio_write(radio->csn_pin, HIGH);

	gpio_export(radio->ce_pin);
	gpio_direction(radio->ce_pin, GPIO_DIR_OUT);
//	gpio_write(radio->ce_pin, LOW);

	gpio_export(radio->irq_pin);
	gpio_direction(radio->irq_pin, GPIO_DIR_IN);
#elif __AVR__
	spi_init(SPI_MODE_0, SPI_MSB, SPI_MASTER_CLOCK_DIV_4);
	// csn_pin is hardcoded to PB2
	DDRB |= (1 << DDB2);	// set as output
//	PORTB |= (1 << PORTB2);	// set high
	// ce_pin is hardcoded to PB1
	DDRB |= (1 << DDB1);	// set as output
//	PORTB &= ~(1 << PORTB1);// set low
	// irq_pin is hardcoded to PD2
	DDRD &= ~(1 << DDD2);	// set as input
#endif
	nrf24_csn(radio, HIGH);
	nrf24_ce(radio, LOW);

#ifndef __AVR__
	usleep(5000);
#else
	_delay_ms(5);
#endif

	// we don't know the previous state of the radio so try to 'fix it up'
	nrf24_stop_listening(radio);
	nrf24_power_down(radio);

#ifndef __AVR__
	usleep(5000);
#else
	_delay_ms(5);
#endif

	// set to reset settings
	// enable read pipes 0 and 1
	reg_buf = _BV(NRF24__ERX_P1) | _BV(NRF24__ERX_P0);
	nrf24_write_register(radio, NRF24__EN_RXADDR, &reg_buf, 1);
	// set address width to 5 bytes
	reg_buf = _BV(1) | _BV(0);
	nrf24_write_register(radio, NRF24__SETUP_AW, &reg_buf, 1);
	// set RF_SETUP to 2Mbps and 0dBm
	reg_buf = _BV(NRF24__RF_DR_HIGH) | _BV(2) | _BV(1);
	nrf24_write_register(radio, NRF24__RF_SETUP, &reg_buf, 1);
	// enable interrupts
	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf = reg_buf & ~(_BV(NRF24__MASK_RX_DR) | _BV(NRF24__MASK_TX_DS) | _BV(NRF24__MASK_MAX_RT));
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);
	// setup retries
	nrf24_set_retries(radio, 5, 15);
	nrf24_set_power_level(radio, NRF24_PA_MAX);
	// only the plus (+) model will let you use this rate
	// otherwise it will not take the setting
	if (nrf24_set_data_rate(radio, NRF24_250KBPS))
		radio->is_plus_model = 1;
	nrf24_set_data_rate(radio, NRF24_1MBPS);
	nrf24_set_CRC_len(radio, NRF24_CRC_16);
	reg_buf = 0;
	nrf24_write_register(radio, NRF24__DYNPD, &reg_buf, 1);
	reg_buf = _BV(NRF24__RX_DR) | _BV(NRF24__TX_DS) | _BV(NRF24__MAX_RT);
	nrf24_write_register(radio, NRF24__STATUS, &reg_buf, 1);
	nrf24_set_channel(radio, NRF24__MAX_CHANNEL / 2);

	nrf24_flush_rx(radio);
	nrf24_flush_tx(radio);

	return radio;
}

void nrf24_done(nrf24_t *radio) {
#ifdef USE_SERCMD
	sercmd_done();
#elif USE_FTDI
	spi_csn(LOW);
	spi_done();
#elif USE_GPIO
	gpio_unexport(radio->irq_pin);
	gpio_write(radio->ce_pin, LOW);
	gpio_unexport(radio->ce_pin);
	gpio_write(radio->csn_pin, LOW);
	gpio_unexport(radio->csn_pin);
	spi_done(radio->spi_device);
#elif __AVR__
	spi_disable();
	// set ce and irq pins to input
	DDRB &= ~(1 << DDB2);
	DDRB &= ~(1 << DDB1);
	DDRD &= ~(1 << DDD2);
#endif

	free(radio);
}

void nrf24_csn(nrf24_t *radio, uint8_t state) {
#ifdef USE_SERCMD
	sercmd_port_set(PORTB, PORTB0, state);
#elif USE_FTDI
	spi_csn(state);
#elif USE_GPIO
	gpio_write(radio->csn_pin, state);
#elif __AVR__
	PORTB = (PORTB & ~(1 << PORTB2)) | (state << PORTB2);
#endif
}

void nrf24_ce(nrf24_t *radio, uint8_t state) {
#ifdef USE_SERCMD
	sercmd_port_set(PORTB, PORTB1, state);
#elif USE_GPIO
	gpio_write(radio->ce_pin, state);
#elif __AVR__
	PORTB = (PORTB & ~(1 << PORTB1)) | (state << PORTB1);
#endif
// for USE_FTDI, CE is tied high
}

uint8_t nrf24_read_register(nrf24_t *radio, uint8_t reg, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf =  NRF24__R_REGISTER | (NRF24__REGISTER_MASK & reg);
	uint8_t status;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);
	status = reg_buf;
	spi_transfer(radio->spi_device, buf, len);

	nrf24_csn(radio, HIGH);
	return status;
}

uint8_t nrf24_write_register(nrf24_t *radio, uint8_t reg, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf =  NRF24__W_REGISTER | (NRF24__REGISTER_MASK & reg);
	uint8_t status;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);
	status = reg_buf;
	spi_transfer(radio->spi_device, buf, len);

	nrf24_csn(radio, HIGH);
	return status;
}

uint8_t nrf24_write_payload(nrf24_t *radio, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf = NRF24__W_TX_PAYLOAD;
	uint8_t status, i;
	uint8_t len_data = radio->payload_size;
	uint8_t len_pad = 0;

	if (len < radio->payload_size) {
		len_data = len;
		if (radio->dynamic_payloads_enabled)
			len_pad = 0;
		else
			len_pad = radio->payload_size - len;
	} // else handled by initialization defaults above

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);
	status = reg_buf;

	spi_transfer(radio->spi_device, buf, len_data);
	// pad out packet (if necessary)
	for (i = 0; i < len_pad; ++i) {
		reg_buf = 0;
		spi_transfer(radio->spi_device, &reg_buf, 1);
	}

	nrf24_csn(radio, HIGH);

	return status;
}

uint8_t nrf24_read_payload(nrf24_t *radio, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf = NRF24__R_RX_PAYLOAD;
	uint8_t status;
	uint8_t len_data;
	uint8_t len_pad, i;

	len_data = (radio->payload_size < len ? radio->payload_size : len);
	len_pad = (radio->dynamic_payloads_enabled ? 0 : radio->payload_size - len_data);

	nrf24_csn(radio, LOW);
	
	spi_transfer(radio->spi_device, &reg_buf, 1);
	status = reg_buf;

	spi_transfer(radio->spi_device, buf, len_data);
	// junk any packet padding
	for (i = 0; i < len_pad; ++i) {
		reg_buf = 0xFF;
		spi_transfer(radio->spi_device, &reg_buf, 1);
	}

	nrf24_csn(radio, HIGH);

	return status;
}

uint8_t nrf24_flush_rx(nrf24_t *radio) {
	uint8_t reg_buf = NRF24__FLUSH_RX;

	nrf24_csn(radio, LOW);
	
	spi_transfer(radio->spi_device, &reg_buf, 1);

	nrf24_csn(radio, HIGH);

	return reg_buf;
}

uint8_t nrf24_flush_tx(nrf24_t *radio) {
	uint8_t reg_buf = NRF24__FLUSH_TX;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);

	nrf24_csn(radio, HIGH);

	return reg_buf;
}

uint8_t nrf24_read_status(nrf24_t *radio) {
	uint8_t reg_buf = NRF24__NOP;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);

	nrf24_csn(radio, HIGH);

	return reg_buf;
}

void nrf24_toggle_features(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_csn(radio, LOW);

	reg_buf = NRF24__ACTIVATE;
	spi_transfer(radio->spi_device, &reg_buf, 1);
	reg_buf = 0x73;
	spi_transfer(radio->spi_device, &reg_buf, 1);

	nrf24_csn(radio, HIGH);
}

void nrf24_reset(nrf24_t *radio) {
	uint8_t reg_buf = 0x00;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);
	reg_buf = 0x0F;
	spi_transfer(radio->spi_device, &reg_buf, 1);

	nrf24_csn(radio, HIGH);

}

void nrf24_set_retries(nrf24_t *radio, uint8_t delay_in_250us_increments, uint8_t max_retries) {
	uint8_t reg_buf = (delay_in_250us_increments & 0xF) << NRF24__ARD | (max_retries & 0xF) << NRF24__ARC;
	nrf24_write_register(radio, NRF24__SETUP_RETR, &reg_buf, 1);

}

void nrf24_set_channel(nrf24_t *radio, uint8_t channel) {
	if (channel > NRF24__MAX_CHANNEL)
		channel = NRF24__MAX_CHANNEL;

	nrf24_write_register(radio, NRF24__RF_CH, &channel, 1);
}

void nrf24_set_payload_size(nrf24_t *radio, uint8_t size) {
	if (size > NRF24__MAX_PAYLOAD_SIZE)
		size = NRF24__MAX_PAYLOAD_SIZE;
	radio->payload_size = size;
}

uint8_t nrf24_get_payload_size(nrf24_t *radio) {
	return radio->payload_size;
}

uint8_t nrf24_get_dynamic_payload_size(nrf24_t *radio) {
	uint8_t reg_buf = NRF24__R_RX_PL_WID;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);
	reg_buf = 0xFF;
	spi_transfer(radio->spi_device, &reg_buf, 1);

	nrf24_csn(radio, HIGH);

	return reg_buf;
}

void nrf24_enable_ack_payload(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__FEATURE, &reg_buf, 1);
	reg_buf |= _BV(NRF24__EN_ACK_PAY) | _BV(NRF24__EN_DPL);
	nrf24_write_register(radio, NRF24__FEATURE, &reg_buf, 1);

	nrf24_read_register(radio, NRF24__FEATURE, &reg_buf, 1);
	// if the register is empty then the features are not enabled
	if (!reg_buf) {
		// enable and try again
		nrf24_toggle_features(radio);
		nrf24_read_register(radio, NRF24__FEATURE, &reg_buf, 1);
		reg_buf |= _BV(NRF24__EN_ACK_PAY) | _BV(NRF24__EN_DPL);
		nrf24_write_register(radio, NRF24__FEATURE, &reg_buf, 1);
	}

	// turn on dynamic payload on pipes 0 and 1
	nrf24_read_register(radio, NRF24__DYNPD, &reg_buf, 1);
	reg_buf |= _BV(NRF24__DPL_P1) | _BV(NRF24__DPL_P0);
	nrf24_write_register(radio, NRF24__DYNPD, &reg_buf, 1);

	radio->ack_payload_enabled = 1;

}

void nrf24_enable_dynamic_payloads(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__FEATURE, &reg_buf, 1);
	reg_buf |= _BV(NRF24__EN_DPL);
	nrf24_write_register(radio, NRF24__FEATURE, &reg_buf, 1);

	nrf24_read_register(radio, NRF24__FEATURE, &reg_buf, 1);
	// if the register is empty then the features are not enabled
	if (!reg_buf) {
		// enable and try again
		nrf24_toggle_features(radio);
		nrf24_read_register(radio, NRF24__FEATURE, &reg_buf, 1);
		reg_buf |= _BV(NRF24__EN_DPL);
		nrf24_write_register(radio, NRF24__FEATURE, &reg_buf, 1);
	}

	// turn on dynamic payloads on all pipes
	nrf24_read_register(radio, NRF24__DYNPD, &reg_buf, 1);
	reg_buf |= _BV(NRF24__DPL_P5) | _BV(NRF24__DPL_P4) | _BV(NRF24__DPL_P3) | _BV(NRF24__DPL_P2) | _BV(NRF24__DPL_P1) | _BV(NRF24__DPL_P0);
	nrf24_write_register(radio, NRF24__DYNPD, &reg_buf, 1);

	radio->dynamic_payloads_enabled = 1;
}

void nrf24_set_auto_ack(nrf24_t *radio, uint8_t enable) {
	uint8_t reg_buf;

	if (enable)
		reg_buf = 0b111111;
	else
		reg_buf = 0;
	nrf24_write_register(radio, NRF24__EN_AA, &reg_buf, 1);
}

void nrf24_set_auto_ack_for_pipe(nrf24_t *radio, uint8_t pipe, uint8_t enable) {
	uint8_t reg_buf;

	if (pipe < 6) {
		nrf24_read_register(radio, NRF24__EN_AA, &reg_buf, 1);
		if (enable)
			reg_buf |= _BV(pipe);
		else
			reg_buf &= ~_BV(pipe);
		nrf24_write_register(radio, NRF24__EN_AA, &reg_buf, 1);
	}
}

void nrf24_set_power_level(nrf24_t *radio, nrf24_power_level_e level) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__RF_SETUP, &reg_buf, 1);
	reg_buf &= ~(_BV(NRF24__RF_PWR_LOW) | _BV(NRF24__RF_PWR_HIGH));

	switch (level) {
		case NRF24_PA_MAX:
			reg_buf |= (_BV(NRF24__RF_PWR_LOW) | _BV(NRF24__RF_PWR_HIGH));
			break;
		case NRF24_PA_HIGH:
			reg_buf |= _BV(NRF24__RF_PWR_HIGH);
			break;
		case NRF24_PA_LOW:
			reg_buf |= _BV(NRF24__RF_PWR_LOW);
			break;
		case NRF24_PA_MIN:
			// nop as &= above has removed bits for this already
			break;
		// otherwise use maximum power
		case NRF24_PA_ERROR:
		default:
    			reg_buf |= (_BV(NRF24__RF_PWR_LOW) | _BV(NRF24__RF_PWR_HIGH));
			break;
	};
	nrf24_write_register(radio, NRF24__RF_SETUP, &reg_buf, 1);
}

nrf24_power_level_e nrf24_get_power_level(nrf24_t *radio) {
	uint8_t reg_buf;
	nrf24_power_level_e pwr_lvl;

	nrf24_read_register(radio, NRF24__RF_SETUP, &reg_buf, 1);

	reg_buf &= (_BV(NRF24__RF_PWR_LOW) | _BV(NRF24__RF_PWR_HIGH));

	switch (reg_buf) {
		case (_BV(NRF24__RF_PWR_LOW) | _BV(NRF24__RF_PWR_HIGH)):
			pwr_lvl = NRF24_PA_MAX;
			break;
		case _BV(NRF24__RF_PWR_HIGH):
			pwr_lvl = NRF24_PA_HIGH;
			break;
		case _BV(NRF24__RF_PWR_LOW):
			pwr_lvl = NRF24_PA_LOW;
			break;
		case 0:
		default:
			pwr_lvl = NRF24_PA_MIN;
			break;
	}

	return pwr_lvl;
}

uint8_t nrf24_set_data_rate(nrf24_t *radio, nrf24_data_rate_e speed) {
	uint8_t reg_buf, rf_setup_reg, reg_buf_verify;

	nrf24_read_register(radio, NRF24__RF_SETUP, &reg_buf, 1);
	reg_buf &= ~(_BV(NRF24__RF_DR_LOW) | _BV(NRF24__RF_DR_HIGH));
	radio->wide_band = 0;

	switch (speed) {
		case NRF24_250KBPS:
			radio->wide_band = 0;
			reg_buf |= _BV(NRF24__RF_DR_LOW);
			break;
		case NRF24_2MBPS:
			radio->wide_band = 1;
			reg_buf |= _BV(NRF24__RF_DR_HIGH);
			break;
		case NRF24_1MBPS:
		default:
			// reg_buf already setup for this mode, and wide_band also setup
			// nop
			break;
	};
	rf_setup_reg = reg_buf;

	nrf24_write_register(radio, NRF24__RF_SETUP, &reg_buf, 1);

	// read back to make sure
	nrf24_read_register(radio, NRF24__RF_SETUP, &reg_buf_verify, 1);
	if (reg_buf_verify == rf_setup_reg) {
		return 1;
	} else {
		radio->wide_band = 0;
	}
	return 0;
}

nrf24_data_rate_e nrf24_get_data_rate(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__RF_SETUP, &reg_buf, 1);
	reg_buf &= (_BV(NRF24__RF_DR_LOW) | _BV(NRF24__RF_DR_HIGH));
	switch (reg_buf) {
		case _BV(NRF24__RF_DR_LOW):
			return NRF24_250KBPS;
			break;
		case _BV(NRF24__RF_DR_HIGH):
			return NRF24_2MBPS;
			break;
		case 0:
		default:
			break;
	};
	return NRF24_1MBPS;
}

void nrf24_set_CRC_len(nrf24_t *radio, nrf24_CRC_length_e length) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf &= ~(_BV(NRF24__CRCO) | _BV(NRF24__EN_CRC));

	switch (length) {
		case NRF24_CRC_DISABLED:
			// nop, flags removed above @ &= ~
			break;
		case NRF24_CRC_8:
			reg_buf |= _BV(NRF24__EN_CRC);
			break;
		case NRF24_CRC_16:
		default:
			reg_buf |= (_BV(NRF24__EN_CRC) | _BV(NRF24__CRCO));
			break;
	};
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);
}

nrf24_CRC_length_e nrf24_get_CRC_len(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf &= (_BV(NRF24__CRCO) | _BV(NRF24__EN_CRC));

	if (reg_buf & _BV(NRF24__EN_CRC)) {
		if (reg_buf & _BV(NRF24__CRCO))
			return NRF24_CRC_16;
		else
			return NRF24_CRC_8;
	}
	return NRF24_CRC_DISABLED;
}

void nrf24_disable_CRC(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf &= ~_BV(NRF24__EN_CRC);
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);
}

void nrf24_start_listening(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf |= _BV(NRF24__PWR_UP) | _BV(NRF24__PRIM_RX);
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);

	reg_buf = _BV(NRF24__RX_DR) | _BV(NRF24__TX_DS) | _BV(NRF24__MAX_RT);
	nrf24_write_register(radio, NRF24__STATUS, &reg_buf, 1);

	if (radio->pipe0_reading_address) // if this has been configured then restore it
		nrf24_write_register(radio, NRF24__RX_ADDR_P0, (uint8_t *)&radio->pipe0_reading_address, 5);	

	nrf24_ce(radio, HIGH);

#ifndef __AVR__
	usleep(130);
#else
	_delay_us(130);
#endif
}

void nrf24_stop_listening(nrf24_t *radio) {
	nrf24_ce(radio, LOW);

	nrf24_flush_tx(radio);
	nrf24_flush_rx(radio);
}

void nrf24_power_down(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf &= ~_BV(NRF24__PWR_UP);
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);
}

void nrf24_power_up(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	reg_buf |= _BV(NRF24__PWR_UP);
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);

#ifndef __AVR__
	usleep(1500);
#else
	_delay_us(1500);
#endif
}

uint8_t nrf24_send(nrf24_t *radio, uint8_t *buf, uint8_t len) {
	uint8_t tx_ok, tx_fail, ack_payload_available;

	nrf24_start_send(radio, buf, len);

#ifdef NRF24_USE_IRQ
#ifndef __AVR__
	// wait for the IRQ pin to change
	gpio_poll(radio->irq_pin, NRF24_SEND_TIMEOUT_MS);
#else
	// busy wait on pin change in 1 us increments (crude)
	for (uint16_t i = 0; i < NRF24_SEND_TIMEOUT_MS; ++i) {
		for (uint16_t j = 0; j < 1000; ++j) {
			// wait until PD2 low
			if ((PORTD & (1 << PORTD2)) == 0)
				break;
			_delay_us(1);
		}
	}
#endif
#else
	uint8_t status, observe_tx;
#ifndef __AVR__
	struct timeval start, end;
	
	gettimeofday(&start, NULL);
	
	do {
		status = nrf24_read_register(radio, NRF24__OBSERVE_TX, &observe_tx, 1);
		gettimeofday(&end, NULL);
	} while(!( status & ( _BV(NRF24__TX_DS) | _BV(NRF24__MAX_RT) ) ) && ( ((end.tv_sec  - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec)/1000.0) + 0.5) < NRF24_SEND_TIMEOUT_MS);
#else
	// busy wait on change in status register
	for (uint16_t i = 0; i < NRF24_SEND_TIMEOUT_MS; ++i) {
		for (uint16_t j = 0; j < 1000; ++j) {
			status = nrf24_read_register(radio, NRF24__OBSERVE_TX, &observe_tx, 1);
			if (status & (_BV(NRF24__TX_DS) | _BV(NRF24__MAX_RT)))
				break;
			_delay_us(1);
		}
	}
#endif
#endif

#if 0
	// wait for the ack payload to arrive (2ms) - is this necessary?
	if (radio->ack_payload_enabled) {
		int i = 2;
		while (!(status & _BV(NRF24__RX_DR)) && i--) {
			status = nrf24_read_register(radio, NRF24__OBSERVE_TX, &observe_tx, 1);
#ifndef __AVR__
#ifndef USE_FTDI
			usleep(1000);
#endif
#else
			_delay_us(1000);
#endif
		}
	}
#endif

	// determine the last send result
	nrf24_last_transmission_state(radio, &tx_ok, &tx_fail, &ack_payload_available);

	// rx_ready will be an ack payload
	if (ack_payload_available) {
		radio->ack_payload_length = nrf24_get_dynamic_payload_size(radio);
		radio->ack_payload_available = 1;
	}

	return tx_ok;
}

uint8_t nrf24_payload_available_on_any_pipe(nrf24_t *radio, uint8_t *pipe) {
	uint8_t status, data_available, reg_buf;

	status = nrf24_read_status(radio);

	data_available = status & _BV(NRF24__RX_DR);

	if (data_available) {
		if (pipe)
			*pipe = (status >> NRF24__RX_P_NO) & 0b1111;
		// clear status bit
		reg_buf = _BV(NRF24__RX_DR);
		nrf24_write_register(radio, NRF24__STATUS, &reg_buf, 1);
		// if this was an ack payload clear that bit
		if (status & _BV(NRF24__TX_DS)) {
			reg_buf = _BV(NRF24__TX_DS);
			nrf24_write_register(radio, NRF24__STATUS, &reg_buf, 1);
		}
	}
	return data_available;
}	

// targetted for removal - really a useless function
uint8_t nrf24_payload_available(nrf24_t *radio) {
	uint8_t pipe;

	return nrf24_payload_available_on_any_pipe(radio, &pipe);	
}

uint8_t nrf24_read(nrf24_t *radio, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf;

	nrf24_read_payload(radio, buf, len);

	nrf24_read_register(radio, NRF24__FIFO_STATUS, &reg_buf, 1);

	return reg_buf & _BV(NRF24__RX_EMPTY);
}

void nrf24_start_send(nrf24_t *radio, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf, tmp;

	// power up the transmitter and put in PTX mode (~PRIM_RX)
	nrf24_read_register(radio, NRF24__CONFIG, &reg_buf, 1);
	tmp = reg_buf;
	reg_buf |= _BV(NRF24__PWR_UP);
	reg_buf &= ~_BV(NRF24__PRIM_RX);
	nrf24_write_register(radio, NRF24__CONFIG, &reg_buf, 1);
	// if the radio wasn't powered up, then give it a startup time
	if (!(tmp & _BV(NRF24__PWR_UP))) {
#ifndef __AVR__
#ifndef USE_FTDI
		usleep(1500);
#endif
#else
		_delay_us(1500);
#endif
	}

	nrf24_write_payload(radio, buf, len);

	// high pulse of > 10us initiates send
	nrf24_ce(radio, HIGH);
#ifndef __AVR__
#ifndef USE_FTDI
	usleep(15);
#endif
#else
	_delay_us(15);
#endif
	nrf24_ce(radio, LOW);
}

void nrf24_write_ack_payload(nrf24_t *radio, uint8_t pipe, uint8_t *buf, uint8_t len) {
	uint8_t reg_buf = NRF24__W_ACK_PAYLOAD | (pipe & 0b111);
	uint8_t len_data;

	nrf24_csn(radio, LOW);

	spi_transfer(radio->spi_device, &reg_buf, 1);
	len_data = (NRF24__MAX_PAYLOAD_SIZE < len ? NRF24__MAX_PAYLOAD_SIZE : len);
	spi_transfer(radio->spi_device, buf, len_data);

	nrf24_csn(radio, HIGH);

}

// note, getting this value resets it
uint8_t nrf24_is_ack_payload_available(nrf24_t *radio) {
	uint8_t result = radio->ack_payload_available;
	radio->ack_payload_available = 0;
	return result;
}

void nrf24_last_transmission_state(nrf24_t *radio, uint8_t *tx_ok, uint8_t *tx_fail, uint8_t *rx_ready) {
	uint8_t reg_buf = _BV(NRF24__RX_DR) | _BV(NRF24__TX_DS) | _BV(NRF24__MAX_RT);
	uint8_t status;

	status = nrf24_write_register(radio, NRF24__STATUS, &reg_buf, 1);

	*tx_ok = status & _BV(NRF24__TX_DS);
	*tx_fail = status & _BV(NRF24__MAX_RT);
	*rx_ready = status & _BV(NRF24__RX_DR);
}

uint8_t nrf24_test_carrier(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__CD, &reg_buf, 1);
	return reg_buf & 1;
}

uint8_t nrf24_test_rpd(nrf24_t *radio) {
	uint8_t reg_buf;

	nrf24_read_register(radio, NRF24__RPD, &reg_buf, 1);
	return reg_buf & 1;
}

void nrf24_open_write_pipe(nrf24_t *radio, uint64_t address) {
	uint64_t addrtmp = address;
	uint8_t reg_buf = (radio->payload_size < NRF24__MAX_PAYLOAD_SIZE ? radio->payload_size : NRF24__MAX_PAYLOAD_SIZE);
	nrf24_write_register(radio, NRF24__RX_ADDR_P0, (uint8_t *)&address, 5);
	// write register clobbers the current value of address so this line restores from backup
	address = addrtmp;
	nrf24_write_register(radio, NRF24__TX_ADDR, (uint8_t *)&address, 5);
	nrf24_write_register(radio, NRF24__RX_PW_P0, &reg_buf, 1);
}

void nrf24_open_read_pipe(nrf24_t *radio, uint8_t pipe, uint64_t address) {
	uint8_t reg_buf, len;

	if (pipe == 0)
		radio->pipe0_reading_address = address;

	if (pipe < 6) {
		// only pipes 0 and 1 specify full addresses, the others just write the LSB
		if (pipe < 2)
			len = 5;
		else
			len = 1;
		// set address
		// base address in memory map for RX_ADDR_P0 is 0x0A - each pipe gets +1
		nrf24_write_register(radio, NRF24__RX_ADDR_P0 + pipe, (uint8_t *)&address, len);
		// set payload size for pipe
		reg_buf = radio->payload_size;
		nrf24_write_register(radio, NRF24__RX_PW_P0 + pipe, &reg_buf, 1);
		// enable receive address
		nrf24_read_register(radio, NRF24__EN_RXADDR, &reg_buf, 1);
		reg_buf |= _BV(NRF24__ERX_P0 + pipe);
		nrf24_write_register(radio, NRF24__EN_RXADDR, &reg_buf, 1);
	}
}

static const char rf24_datarate_e_str_0[] PROGMEM = "1MBPS";
static const char rf24_datarate_e_str_1[] PROGMEM = "2MBPS";
static const char rf24_datarate_e_str_2[] PROGMEM = "250KBPS";
static const char * const rf24_datarate_e_str_P[] PROGMEM = {
  rf24_datarate_e_str_0,
  rf24_datarate_e_str_1,
  rf24_datarate_e_str_2,
};
static const char rf24_model_e_str_0[] PROGMEM = "nRF24L01";
static const char rf24_model_e_str_1[] PROGMEM = "nRF24L01+";
static const char * const rf24_model_e_str_P[] PROGMEM = {
  rf24_model_e_str_0,
  rf24_model_e_str_1,
};
static const char rf24_crclength_e_str_0[] PROGMEM = "Disabled";
static const char rf24_crclength_e_str_1[] PROGMEM = "8 bits";
static const char rf24_crclength_e_str_2[] PROGMEM = "16 bits" ;
static const char * const rf24_crclength_e_str_P[] PROGMEM = {
  rf24_crclength_e_str_0,
  rf24_crclength_e_str_1,
  rf24_crclength_e_str_2,
};
static const char rf24_power_level_e_str_0[] PROGMEM = "PA_MIN";
static const char rf24_power_level_e_str_1[] PROGMEM = "PA_LOW";
static const char rf24_power_level_e_str_2[] PROGMEM = "PA_HIGH";
static const char rf24_power_level_e_str_3[] PROGMEM = "PA_MAX";
static const char * const rf24_power_level_e_str_P[] PROGMEM = {
  rf24_power_level_e_str_0,
  rf24_power_level_e_str_1,
  rf24_power_level_e_str_2,
  rf24_power_level_e_str_3,
};

void nrf24_print_details(nrf24_t *radio) {
	uint8_t status, reg_buf;

#ifndef __AVR__
#ifndef USE_FTDI
#ifndef USE_SERCMD
	printf_P(PSTR("SPI device = %d\n"), radio->spi_device);
	printf_P(PSTR("SPI file   = %s\n"), (radio->spi_device ? SPI_DEV1 : SPI_DEV0));
	printf_P(PSTR("SPI speed  = %d\n"), spi[radio->spi_device].speed);
#endif
#endif
#endif

	nrf24_read_register(radio, NRF24__STATUS, &status, 1);
	nrf24_print_status(status);

	nrf24_read_register(radio, NRF24__OBSERVE_TX, &reg_buf, 1);
	nrf24_print_observe_tx(reg_buf);

	nrf24_print_address_register(radio, PSTR("RX_ADDR_P0-1"), NRF24__RX_ADDR_P0, 2);
	nrf24_print_byte_register(radio, PSTR("RX_ADDR_P2-5"), NRF24__RX_ADDR_P2, 4);
	nrf24_print_address_register(radio, PSTR("TX_ADDR"), NRF24__TX_ADDR, 1);

	nrf24_print_byte_register(radio, PSTR("RX_PW_P0-6"), NRF24__RX_PW_P0, 6);
	nrf24_print_byte_register(radio, PSTR("EN_AA") , NRF24__EN_AA, 1);
	nrf24_print_byte_register(radio, PSTR("EN_RXADDR"), NRF24__EN_RXADDR, 1);
	nrf24_print_byte_register(radio, PSTR("RF_CH"), NRF24__RF_CH, 1);
	nrf24_print_byte_register(radio, PSTR("RF_SETUP"), NRF24__RF_SETUP, 1);
	nrf24_print_byte_register(radio, PSTR("SETUP_RETR"), NRF24__SETUP_RETR, 1);
	nrf24_print_byte_register(radio, PSTR("CONFIG"), NRF24__CONFIG, 1);
	nrf24_print_byte_register(radio, PSTR("DYNPD"), NRF24__DYNPD, 1);
	nrf24_print_byte_register(radio, PSTR("FEATURE"), NRF24__FEATURE, 1);

#ifndef __AVR__
	printf_P(PSTR("data rate            = %s\n"), pgm_read_word(&rf24_datarate_e_str_P[nrf24_get_data_rate(radio)]));
	printf_P(PSTR("model                = %s\n"), pgm_read_word(&rf24_model_e_str_P[radio->is_plus_model]));
	printf_P(PSTR("CRC length           = %s\n"), pgm_read_word(&rf24_crclength_e_str_P[nrf24_get_CRC_len(radio)]));
	printf_P(PSTR("power level          = %s\n"), pgm_read_word(&rf24_power_level_e_str_P[nrf24_get_power_level(radio)]));
#else
	printf_P(PSTR("data rate            = %S\n"), pgm_read_word(&rf24_datarate_e_str_P[nrf24_get_data_rate(radio)]));
	printf_P(PSTR("model                = %S\n"), pgm_read_word(&rf24_model_e_str_P[radio->is_plus_model]));
	printf_P(PSTR("CRC length           = %S\n"), pgm_read_word(&rf24_crclength_e_str_P[nrf24_get_CRC_len(radio)]));
	printf_P(PSTR("power level          = %S\n"), pgm_read_word(&rf24_power_level_e_str_P[nrf24_get_power_level(radio)]));
#endif
}

void nrf24_print_observe_tx(uint8_t register_value) {
	printf_P(PSTR("OBSERVE_TX = %02x\n\tPLOS_CNT = %x\n\tSRC_CNT  = %x\n"), \
		register_value, (register_value >> NRF24__PLOS_CNT) & 0b1111, (register_value >> NRF24__ARC_CNT) & 0b1111);
}

void nrf24_print_byte_register(nrf24_t *radio, const char *name, uint8_t reg, uint8_t span) {
	uint8_t reg_buf;

#ifndef __AVR__
	printf_P(PSTR("%-20s ="), name);
#else
	printf_P(PSTR("%-20S ="), name);
#endif
	while (span--) {
		nrf24_read_register(radio, reg++, &reg_buf, 1);
		printf_P(PSTR(" 0x%02x"), reg_buf);
	}
	printf_P(PSTR("\n"));
}

void nrf24_print_address_register(nrf24_t *radio, const char *name, uint8_t reg, uint8_t span) {
#ifndef __AVR__
	printf_P(PSTR("%-20s ="), name);
#else
	printf_P(PSTR("%-20S ="), name);
#endif
	while (span--) {
		uint8_t reg_buf[NRF24__ADDR_LEN], i;

		nrf24_read_register(radio, reg++, reg_buf, NRF24__ADDR_LEN);

		printf_P(PSTR(" 0x"));
		for (i = 0; i < NRF24__ADDR_LEN; ++i)
			printf_P(PSTR("%02x"), reg_buf[i]);
	}
	printf_P(PSTR("\n"));
}

void nrf24_print_status(uint8_t status) {
	printf_P(PSTR("STATUS    = 0x%02x\n"), status);
	printf_P(PSTR("\tRX_DR   = %x\n"), (status & _BV(NRF24__RX_DR))?1:0);
	printf_P(PSTR("\tTX_DS   = %x\n"), (status & _BV(NRF24__TX_DS))?1:0);
	printf_P(PSTR("\tMAX_RT  = %x\n"), (status & _BV(NRF24__MAX_RT))?1:0);
	printf_P(PSTR("\tRX_P_NO = %x\n"), ((status >> NRF24__RX_P_NO) & 0b111));
	printf_P(PSTR("\tTX_FULL = %x\n"), (status & _BV(NRF24__TX_FULL))?1:0);
}
