#ifndef INPUT_H
#define INPUT_H

#define KEYS 10
#include <termios.h>
#include "config.h"

enum InputMode mode_set(enum InputMode mode, struct termios *old, struct termios *new, int *fd);

void input_clean(enum InputMode mode, struct termios *old, int fd);

void get_inputs(Config *config, int fd, int8_t inputs[KEYS]);

#endif
