#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

enum InputMode {
    EXTKEYS,
    SCANCODES,
    NORM
};

typedef struct Config {
    uint32_t das;
    uint32_t arr;
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
    uint32_t grav; // units of 1/1000 blocks per frame
    uint32_t goal;
    enum InputMode mode;

} Config;

void config_init(Config *config);

#endif
