#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <ini.h>

void config_init_extkeys(Config *config) {
    config->left  = 260;
    config->right = 261;
    config->sd    = 258;
    config->hd    = ' ';
    config->ccw   = 'a';
    config->cw    = 's';
    config->flip  = 'd';
    config->hold  = 57441;
    config->reset = 'r';
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

static int handler(void* user, const char* section, const char* name,
                   const char* value) {
    Config *config = (Config*) user;

    #define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)
    char *mode_section = NULL;

    switch (config->mode) {
    case EXTKEYS:
        mode_section = "extkeys";
        break;
    case SCANCODES:
        mode_section = "scan";
        break;
    case NORM:
        mode_section = "norm";
        break;
    }

    if (MATCH("handling", "das")) {
        config->das = atoi(value);
    } else if (MATCH("handling", "arr")) {
        config->arr = atoi(value);
    } else if (MATCH(mode_section, "left")) {
        config->left = atoi(value);
    } else if (MATCH(mode_section, "right")) {
        config->right = atoi(value);
    } else if (MATCH(mode_section, "sd")) {
        config->sd = atoi(value);
    } else if (MATCH(mode_section, "hd")) {
        config->hd = atoi(value);
    } else if (MATCH(mode_section, "ccw")) {
        config->ccw = atoi(value);
    } else if (MATCH(mode_section, "cw")) {
        config->cw = atoi(value);
    } else if (MATCH(mode_section, "180")) {
        config->flip = atoi(value);
    } else if (MATCH(mode_section, "hold")) {
        config->hold = atoi(value);
    } else if (MATCH(mode_section, "reset")) {
        config->reset = atoi(value);
    } else if (MATCH(mode_section, "quit")) {
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

    config->arr = 0;
    config->das = 4;
    config->grav = 20;
    config->goal = 40;
    switch (config->mode) {
    case EXTKEYS:
        config_init_extkeys(config);
        break;
    case SCANCODES:
        config_init_scan(config);
        break;
    case NORM:
        config_init_norm(config);
        break;
    }
    ini_parse(config_path, handler, config);
}
