#ifndef INPUT_H
#define INPUT_H

#define KEYS 10
#include <stdint.h>
#include <termios.h>
#include "config.h"

enum InputMode mode_set(enum InputMode, struct termios *, struct termios *, int *);

void input_clean(enum InputMode, struct termios *, int);

void get_inputs(Config *, int, int8_t []);

#endif
