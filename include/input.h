#define KEYS 10
#include <stdint.h>
#include <termios.h>

enum InputMode {
    EXTKEYS,
    SCANCODES,
    NORM
};

enum InputMode mode_set(enum InputMode, struct termios *, struct termios *, int *);

void input_clean(enum InputMode, struct termios *, int);

void get_inputs(enum InputMode, int, int8_t []);

