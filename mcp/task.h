#ifndef _TASK_H_
#define _TASK_H_

typedef struct tasks_table {
	char *name;
        int (*function)(int argc, char *argv[]);
} tasks_table_t;

int task_parse(int level, int argc, char *argv[]);
int (*task_lookup(const tasks_table_t *lookup_table, int argv_index, int argc, char *argv[]))(int argc, char *argv[]);
// utility functions
nrf24_t *task_radio_setup(nrf24_power_level_e power_level, nrf24_data_rate_e data_rate, uint8_t channel, uint64_t write_pipe_addr, uint64_t read_pipe_addr);
void task_radio_done(nrf24_t *radio);
// in send_packet, verbose should be the uint64_t address of the recipient if anything should be printed, other this function is stdout silent
uint8_t task_send_packet(nrf24_t *radio, char *packet_type_name, uint8_t *packet, uint8_t len, uint32_t delay_in_us, uint64_t verbose);
uint8_t task_read_ack_payload(nrf24_t *radio, uint8_t *buffer, uint8_t packet_type, uint8_t packet_len);

// task_program.c
#define PROGRAM_SEND_POST_DELAY_US 10000
int task_program(int argc, char *argv[]);
int task_program_parse(int argc, char *argv[]);
int task_program_upload(int argc, char *argv[]);
int task_program_stop(int argc, char *argv[]);
int task_program_start(int argc, char *argv[]);
int task_program_rgb(int argc, char *argv[]);
int task_program_reset(int argc, char *argv[]);
int task_program_rainbow(int argc, char *argv[]);
int task_program_download(int argc, char *argv[]);
int task_program_lightfreq(int argc, char *argv[]);
void task_program_print_ack_payload(nrf24_t *radio, uint8_t *packet);

// task_flash.c
#define FLASH_SEND_POST_DELAY_US 10000
int task_flash(int argc, char *argv[]);
int task_flash_hex(int argc, char *argv[]);
int task_flash_test(int argc, char *argv[]);
int task_flash_upload(int argc, char *argv[]);
int task_flash_download(int argc, char *argv[]);
int task_flash_eeprom(int argc, char *argv[]);

// task_nRF24.c
int task_nRF24(int argc, char *argv[]);
int task_nRF24_print(int argc, char *argv[]);
int task_nRF24_scanner(int argc, char *argv[]);
int task_nRF24_ping(int argc, char *argv[]);

// task_gpio.c
int task_gpio(int argc, char *argv[]);
int task_gpio_test(int argc, char *argv[]);

#endif
