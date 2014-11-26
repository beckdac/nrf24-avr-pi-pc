#ifndef _NRF24L01P_H_
#define _NRF24L01P_H_

#define NRF24_SEND_TIMEOUT_MS 500

/* begin defines */
#define NRF24__MAX_PAYLOAD_SIZE	32
#define NRF24__MAX_CHANNEL	126
#define NRF24__ADDR_LEN		5

/* Memory Map */
#define NRF24__CONFIG      0x00
#define NRF24__EN_AA       0x01
#define NRF24__EN_RXADDR   0x02
#define NRF24__SETUP_AW    0x03
#define NRF24__SETUP_RETR  0x04
#define NRF24__RF_CH       0x05
#define NRF24__RF_SETUP    0x06
#define NRF24__STATUS      0x07
#define NRF24__OBSERVE_TX  0x08
#define NRF24__CD          0x09
#define NRF24__RX_ADDR_P0  0x0A
#define NRF24__RX_ADDR_P1  0x0B
#define NRF24__RX_ADDR_P2  0x0C
#define NRF24__RX_ADDR_P3  0x0D
#define NRF24__RX_ADDR_P4  0x0E
#define NRF24__RX_ADDR_P5  0x0F
#define NRF24__TX_ADDR     0x10
#define NRF24__RX_PW_P0    0x11
#define NRF24__RX_PW_P1    0x12
#define NRF24__RX_PW_P2    0x13
#define NRF24__RX_PW_P3    0x14
#define NRF24__RX_PW_P4    0x15
#define NRF24__RX_PW_P5    0x16
#define NRF24__FIFO_STATUS 0x17
#define NRF24__DYNPD       0x1C
#define NRF24__FEATURE     0x1D

/* Bit Mnemonics */

/* configuration register */
#define NRF24__MASK_RX_DR  6
#define NRF24__MASK_TX_DS  5
#define NRF24__MASK_MAX_RT 4
#define NRF24__EN_CRC      3
#define NRF24__CRCO        2
#define NRF24__PWR_UP      1
#define NRF24__PRIM_RX     0

/* enable auto acknowledgment */
#define NRF24__ENAA_P5     5
#define NRF24__ENAA_P4     4
#define NRF24__ENAA_P3     3
#define NRF24__ENAA_P2     2
#define NRF24__ENAA_P1     1
#define NRF24__ENAA_P0     0

/* enable rx addresses */
#define NRF24__ERX_P5      5
#define NRF24__ERX_P4      4
#define NRF24__ERX_P3      3
#define NRF24__ERX_P2      2
#define NRF24__ERX_P1      1
#define NRF24__ERX_P0      0

/* setup of address width */
#define NRF24__AW          0 /* 2 bits */

/* setup of auto re-transmission */
#define NRF24__ARD         4 /* 4 bits */
#define NRF24__ARC         0 /* 4 bits */

/* RF setup register */
#define NRF24__PLL_LOCK    4
#define NRF24__RF_DR       3
#define NRF24__RF_PWR      1 /* 2 bits */   

/* general status register */
#define NRF24__RX_DR       6
#define NRF24__TX_DS       5
#define NRF24__MAX_RT      4
#define NRF24__RX_P_NO     1 /* 3 bits */
#define NRF24__TX_FULL     0

/* transmit observe register */
#define NRF24__PLOS_CNT    4 /* 4 bits */
#define NRF24__ARC_CNT     0 /* 4 bits */

/* fifo status */
#define NRF24__TX_REUSE    6
#define NRF24__FIFO_FULL   5
#define NRF24__TX_EMPTY    4
#define NRF24__RX_FULL     1
#define NRF24__RX_EMPTY    0

/* dynamic length */
#define NRF24__DPL_P0      0
#define NRF24__DPL_P1      1
#define NRF24__DPL_P2      2
#define NRF24__DPL_P3      3
#define NRF24__DPL_P4      4
#define NRF24__DPL_P5      5
#define NRF24__EN_DPL      2
#define NRF24__EN_ACK_PAY  1
#define NRF24__EN_DYN_ACK  0

/* Instruction Mnemonics */
#define NRF24__R_REGISTER    0x00 /* last 4 bits will indicate reg. address */
#define NRF24__W_REGISTER    0x20 /* last 4 bits will indicate reg. address */
#define NRF24__REGISTER_MASK 0x1F
#define NRF24__R_RX_PAYLOAD  0x61
#define NRF24__W_TX_PAYLOAD  0xA0
#define NRF24__W_ACK_PAYLOAD 0xA8
#define NRF24__FLUSH_TX      0xE1
#define NRF24__FLUSH_RX      0xE2
#define NRF24__REUSE_TX_PL   0xE3
#define NRF24__ACTIVATE      0x50 
#define NRF24__R_RX_PL_WID   0x60
#define NRF24__NOP           0xFF

/* Non-P omissions */
#define NRF24__LNA_HCURR   0

/* P model memory Map */
#define NRF24__RPD         0x09

/* P model bit Mnemonics */
#define NRF24__RF_DR_LOW   5
#define NRF24__RF_DR_HIGH  3
#define NRF24__RF_PWR_LOW  1
#define NRF24__RF_PWR_HIGH 2
/* end defines */

/* power level */
typedef enum { NRF24_PA_MIN = 0, NRF24_PA_LOW, NRF24_PA_HIGH, NRF24_PA_MAX, NRF24_PA_ERROR } nrf24_power_level_e;
/* data rate */
typedef enum { NRF24_1MBPS = 0, NRF24_2MBPS, NRF24_250KBPS } nrf24_data_rate_e;
/* CRC length */
typedef enum { NRF24_CRC_DISABLED = 0, NRF24_CRC_8, NRF24_CRC_16 } nrf24_CRC_length_e;

typedef struct nrf24_config {
	uint8_t spi_device;
	uint8_t ce_pin;
	uint8_t csn_pin;
	uint8_t	irq_pin;
	uint8_t is_plus_model;
	uint8_t dynamic_payloads_enabled;
	uint8_t payload_size;
	uint8_t wide_band;
	uint8_t ack_payload_enabled;
	uint8_t ack_payload_length;
	uint8_t ack_payload_available;
	uint64_t pipe0_reading_address;
} nrf24_t;

#ifdef USE_SERCMD
nrf24_t *nrf24_init(char *serial_device, int serial_baud);
#elif USE_FTDI
nrf24_t *nrf24_init(void);
#elif USE_GPIO
nrf24_t *nrf24_init(uint8_t spi_device, uint8_t csn_pin, uint8_t ce_pin, uint8_t irq_pin);
#elif __AVR__
nrf24_t *nrf24_init(void);
#else
#warning at least one of USE_SERCMD, USE_FTDI, USE_GPIO and __AVR__ must be defined!
#endif
void nrf24_done(nrf24_t *radio);

// LOW selects this radio on the SPI bus, HIGH disengages
void nrf24_csn(nrf24_t *radio, uint8_t state);
// LOW puts the radio in standby, HIGH begins transmission
void nrf24_ce(nrf24_t *radio, uint8_t state);

// reads register specified by reg into buf of length len and returns status register value
uint8_t nrf24_read_register(nrf24_t *radio, uint8_t reg, uint8_t *buf, uint8_t len);
// set value of register specified by reg from buf of length len and returns status register value
uint8_t nrf24_write_register(nrf24_t *radio, uint8_t reg, uint8_t* buf, uint8_t len);
// writes the transmission payload and returns the status register value
uint8_t nrf24_write_payload(nrf24_t *radio, uint8_t *buf, uint8_t len);
// reads the receiver payload value and returns the status register value
uint8_t nrf24_read_payload(nrf24_t *radio, uint8_t *buf, uint8_t len);
// flushes the receive buffer returns the status register value
uint8_t nrf24_flush_rx(nrf24_t *radio);
// flushes the transmit buffer returns the status register value
uint8_t nrf24_flush_tx(nrf24_t *radio);
// reads the status register of the radio
uint8_t nrf24_read_status(nrf24_t *radio);

// configuration
// enables special features of NRF24L01+
void nrf24_toggle_features(nrf24_t *radio);
// resets registers to defaults
void nrf24_reset(nrf24_t *radio);
// most of these are self explanatory
void nrf24_set_retries(nrf24_t *radio, uint8_t delay_in_250us_increments, uint8_t max_retries);
void nrf24_set_channel(nrf24_t *radio, uint8_t channel);
void nrf24_set_payload_size(nrf24_t *radio, uint8_t size);
uint8_t nrf24_get_payload_size(nrf24_t *radio);
uint8_t nrf24_get_dynamic_payload_size(nrf24_t *radio);
void nrf24_enable_ack_payload(nrf24_t *radio);
void nrf24_enable_dynamic_payloads(nrf24_t *radio);
void nrf24_set_auto_ack(nrf24_t *radio, uint8_t enable) ;
void nrf24_set_auto_ack_for_pipe(nrf24_t *radio, uint8_t pipe, uint8_t enable) ;
void nrf24_set_power_level(nrf24_t *radio, nrf24_power_level_e level) ;
nrf24_power_level_e nrf24_get_power_level(nrf24_t *radio);
uint8_t nrf24_set_data_rate(nrf24_t *radio, nrf24_data_rate_e speed);
nrf24_data_rate_e nrf24_get_data_rate(nrf24_t *radio);
void nrf24_set_CRC_len(nrf24_t *radio, nrf24_CRC_length_e length);
nrf24_CRC_length_e nrf24_get_CRC_len(nrf24_t *radio);
void nrf24_disable_CRC(nrf24_t *radio);

// control
// starts the radio listening on open pipes
void nrf24_start_listening(nrf24_t *radio);
// stops listening
void nrf24_stop_listening(nrf24_t *radio);
// put in standby mode
void nrf24_power_down(nrf24_t *radio);
// return to active mode
void nrf24_power_up(nrf24_t *radio);

// transmission related
// write payload to output pipe and auto handle acks and retransmits
// blocks on transmission and returns 0 or 1 if the send was failed or successful, respectively
uint8_t nrf24_send(nrf24_t *radio, uint8_t *buf, uint8_t len);
// returns 0 if no payload or 1 if data available
uint8_t nrf24_payload_available(nrf24_t *radio);
// read data from open pipe, returns 0 if success, 1 if failure
uint8_t nrf24_read(nrf24_t *radio, uint8_t *buf, uint8_t len);
// starts a transmission, must monitor IRQ pin via poll to get done and ack
void nrf24_start_send(nrf24_t *radio, uint8_t *buf, uint8_t len);
// setup ack payload for a given pipe
void nrf24_write_ack_payload(nrf24_t *radio, uint8_t pipe, uint8_t *buf, uint8_t len);
// returns 1 if ack payload available
uint8_t nrf24_is_ack_payload_available(nrf24_t *radio);
// determine the state of the last non-blocking transmission
// tx_ok if TX_DS true
// tx_fail if MAX_RT true
// rx_rady if RX_DS true because message ready in payload
void nrf24_last_transmission_state(nrf24_t *radio, uint8_t *tx_ok, uint8_t *tx_fail, uint8_t *rx_ready);

// status
// check if another radio has a carrier for the previous start/stop listening
uint8_t nrf24_test_carrier(nrf24_t *radio);
// verify some signal at -64dbm or more
uint8_t nrf24_test_rpd(nrf24_t *radio);

// pipes
// opens a pipe for writing
// address is a 40bit value, e.g. 0xF0F0F0F0F0
void nrf24_open_write_pipe(nrf24_t *radio, uint64_t address);
void nrf24_open_read_pipe(nrf24_t *radio, uint8_t pipe, uint64_t address);
// returns 1 if data are available on any open read pipe, the pipe number is placed in pipe
// returns 0 if no data
uint8_t nrf24_payload_available_on_any_pipe(nrf24_t *radio, uint8_t *pipe);

// verbose / debugging
void nrf24_print_details(nrf24_t *radio);
void nrf24_print_status(uint8_t status);
void nrf24_print_observe_tx(uint8_t register_value);
// reads a register and span - 1 succesive registers and prints them
void nrf24_print_byte_register(nrf24_t *radio, const char *name, uint8_t reg, uint8_t span);
// reads the address registers and span - 1 successive regs and prints them
void nrf24_print_address_register(nrf24_t *radio, const char *name, uint8_t reg, uint8_t span);



#endif /* _NRF24L01P_H_ */
