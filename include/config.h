#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

enum InputMode {
    EXTKEYS,
    SCANCODES,
    NORM
};

typedef struct Config {
    uint32_t left;
    uint32_t right;
    uint32_t sd;
    uint32_t hd;
    uint32_t ccw;
    uint32_t cw;
    uint32_t flip;
    uint32_t hold;
    uint32_t reset;
    uint32_t quit;
    enum InputMode mode;
} Config;

void config_init(Config *config);

#endif
