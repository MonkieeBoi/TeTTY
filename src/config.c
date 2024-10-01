#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ini.h>

void config_init_extkeys(Config *config) {
    config->left  = 260;
    config->right = 261;
    config->sd    = 258;
    config->hd    = ' ',
    config->ccw   = 'a',
    config->cw    = 's',
    config->flip  = 'd',
    config->hold  = 57441,
    config->reset = 'r',
    config->quit  = 'q';
}

void config_init_scan(Config *config) {
    config->left  = 0x4b;
    config->right = 0x4d;
    config->sd    = 0x50;
    config->hd    = 0x39;
    config->ccw   = 0x1e;
    config->cw    = 0x1f;
    config->flip  = 0x20;
    config->hold  = 0x2a;
    config->reset = 0x13;
    config->quit  = 0x10;
}

void config_init_norm(Config *config) {
    config->left  = 'D';
    config->right = 'C';
    config->sd    = 'B';
    config->hd    = ' ';
    config->ccw   = 'a';
    config->cw    = 's';
    config->flip  = 'd';
    config->hold  = 'z';
    config->reset = 'r';
    config->quit  = 'q';
}

static int extkeys_handler(void* user, const char* section, const char* name,
                   const char* value) {
    Config *config = (Config*) user;

    #define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)
    if (MATCH("extkeys", "left")) {
        config->left = atoi(value);
    } else if (MATCH("extkeys", "right")) {
        config->right = atoi(value);
    } else if (MATCH("extkeys", "sd")) {
        config->sd = atoi(value);
    } else if (MATCH("extkeys", "hd")) {
        config->hd = atoi(value);
    } else if (MATCH("extkeys", "ccw")) {
        config->ccw = atoi(value);
    } else if (MATCH("extkeys", "cw")) {
        config->cw = atoi(value);
    } else if (MATCH("extkeys", "180")) {
        config->flip = atoi(value);
    } else if (MATCH("extkeys", "hold")) {
        config->hold = atoi(value);
    } else if (MATCH("extkeys", "reset")) {
        config->reset = atoi(value);
    } else if (MATCH("extkeys", "quit")) {
        config->quit = atoi(value);
    } else {
        return 0;
    }
    return 1;
}

static int scan_handler(void* user, const char* section, const char* name,
                   const char* value) {
    Config *config = (Config*) user;

    #define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)
    if (MATCH("scan", "left")) {
        config->left = atoi(value);
    } else if (MATCH("scan", "right")) {
        config->right = atoi(value);
    } else if (MATCH("scan", "sd")) {
        config->sd = atoi(value);
    } else if (MATCH("scan", "hd")) {
        config->hd = atoi(value);
    } else if (MATCH("scan", "ccw")) {
        config->ccw = atoi(value);
    } else if (MATCH("scan", "cw")) {
        config->cw = atoi(value);
    } else if (MATCH("scan", "180")) {
        config->flip = atoi(value);
    } else if (MATCH("scan", "hold")) {
        config->hold = atoi(value);
    } else if (MATCH("scan", "reset")) {
        config->reset = atoi(value);
    } else if (MATCH("scan", "quit")) {
        config->quit = atoi(value);
    } else {
        return 0;
    }
    return 1;
}

static int norm_handler(void* user, const char* section, const char* name,
                   const char* value) {
    Config *config = (Config*) user;

    #define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)
    if (MATCH("norm", "left")) {
        config->left = atoi(value);
    } else if (MATCH("norm", "right")) {
        config->right = atoi(value);
    } else if (MATCH("norm", "sd")) {
        config->sd = atoi(value);
    } else if (MATCH("norm", "hd")) {
        config->hd = atoi(value);
    } else if (MATCH("norm", "ccw")) {
        config->ccw = atoi(value);
    } else if (MATCH("norm", "cw")) {
        config->cw = atoi(value);
    } else if (MATCH("norm", "180")) {
        config->flip = atoi(value);
    } else if (MATCH("norm", "hold")) {
        config->hold = atoi(value);
    } else if (MATCH("norm", "reset")) {
        config->reset = atoi(value);
    } else if (MATCH("norm", "quit")) {
        config->quit = atoi(value);
    } else {
        return 0;
    }
    return 1;
}

void get_config_path(char *config_path) {
    char *config_env = getenv("XDG_CONFIG_HOME");
    char *home_env = getenv("HOME");
    if (config_env) {
        strcpy(config_path, config_env);
        strcat(config_path, "/tetty");
    } else if (home_env) {
        strcpy(config_path, home_env);
        strcat(config_path, "/.config/tetty");
    } else {
        return;
    }
    strcat(config_path, "/config.ini");
}

void config_init(Config *config) {
    char config_path[4096] = { 0 };
    get_config_path(config_path);

    switch (config->mode) {
    case EXTKEYS:
        config_init_extkeys(config);
        ini_parse(config_path, extkeys_handler, config);
        break;
    case SCANCODES:
        config_init_scan(config);
        ini_parse(config_path, scan_handler, config);
        break;
    case NORM:
        config_init_norm(config);
        ini_parse(config_path, norm_handler, config);
        break;
    }
}
