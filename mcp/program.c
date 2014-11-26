#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "error.h"
#include "file.h"
#include "program.h"

program_t *program_load_from_file(char *filename) {
	file_t *f;
	program_t *p;
	char line[1024];
	int line_no = 0;
	program_step_t *s;

	f = file_open(filename, "r");

	printf("reading program from file '%s'\n", f->filename);

	p = malloc(sizeof(program_t));
	memset(p, 0, sizeof(program_t));
	p->size = sizeof(uint16_t) + sizeof(uint16_t); // preample in eeprom + # of steps in program

	while (fgets(line, 1024, f->fp) != NULL) {
		char *token, *next;
		++line_no;
		if (line[0] == '#')
			continue;
		s = malloc(sizeof(program_step_t));
		//memset(s, 0, sizeof(program_step_t));
		// red
		token = strtok_r(line, "\t", &next);
		if (token == NULL) {
			fatal_error("error while parsing program file '%s' one line %d: no red token found!\n", f->filename, line_no);
		} else if (atoi(token) > 255) {
			fatal_error("error while parsing program file '%s' one line %d: red value > 255 (was %d)\n", f->filename, line_no, atoi(token));
		}
		s->rgb[0] = atoi(token);
		// green
		token = strtok_r(NULL, "\t", &next);
		if (token == NULL) {
			fatal_error("error while parsing program file '%s' one line %d: no green token found!\n", f->filename, line_no);
		} else if (atoi(token) > 255) {
			fatal_error("error while parsing program file '%s' one line %d: green value > 255 (was %d)\n", f->filename, line_no, atoi(token));
		}
		s->rgb[1] = atoi(token);
		// blue
		token = strtok_r(NULL, "\t", &next);
		if (token == NULL) {
			fatal_error("error while parsing program file '%s' one line %d: no blue token found!\n", f->filename, line_no);
		} else if (atoi(token) > 255) {
			fatal_error("error while parsing program file '%s' one line %d: blue value > 255 (was %d)\n", f->filename, line_no, atoi(token));
		}
		s->rgb[2] = atoi(token);
		// delay
		token = strtok_r(NULL, "]t", &next);
		if (token == NULL) {
			fatal_error("error while parsing program file '%s' one line %d: no delay token found!\n", f->filename, line_no);
		} else if (atoi(token) > 65534) {
			fatal_error("error while parsing program file '%s' one line %d: delay in ms was > 65534 (was %d)\n", f->filename, line_no, atoi(token));
		}
		s->delay_in_ms = atoi(token);
		p->steps++;
		p->step = realloc(p->step, sizeof(program_step_t *) * p->steps);
		p->step[p->steps - 1] = s;
		p->size += (3 * sizeof(uint8_t)) + sizeof(uint16_t);
	}

	if (p->size > PROGRAM_MAX_SIZE)
		fatal_error("program in file '%s' exceeds maximum program size of %d bytes\n", f->filename, PROGRAM_MAX_SIZE);

	printf("program in file '%s' has %d steps and will use %d bytes of eeprom\n", f->filename, p->steps, p->size);

	file_close(f);

	return p;
}

void program_free(program_t *p) {
	int i;

	for (i = 0; i < p->steps; ++i)
		free(p->step[i]);
	free(p->step);
	free(p);
}
