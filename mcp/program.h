#ifndef _PROGRAM_H_
#define _PROGRAM_H_

#define PROGRAM_MAX_SIZE 1024

typedef struct program_step {
	uint8_t rgb[3];
	uint16_t delay_in_ms;
} program_step_t;

typedef struct program {
	uint16_t steps;
	program_step_t **step;
	uint16_t size;
} program_t;

program_t *program_load_from_file(char *filename);
void program_free(program_t *p);

#endif /* _PROGRAM_H_ */
