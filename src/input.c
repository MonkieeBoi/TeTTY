#include <curses.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "input.h"

enum Parser {
    CODE,
    MOD,
    STATE,
    END,
    INVALID,
};

static const char *conspath[] = {
    "/proc/self/fd/0",
    "/dev/tty",
    "/dev/tty0",
    "/dev/vc/0",
    "/dev/systty",
    "/dev/console",
    NULL
};

int is_a_console(int fd) {
    char arg = 0;
    return (isatty(fd) && ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84)));
}

int getfd(struct termios* old, struct termios* new) {
    int fd = 0;

    for (int i = 0; conspath[i]; i++) {
        fd = open(conspath[i], O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (is_a_console(fd))
            break;
        close(fd);
    }

    if (fd < 0) {
        fprintf(stderr, "no fd\n");
        return -1;
    }

    if (tcgetattr(fd, old) == -1 || tcgetattr(fd, new) == -1) {
        fprintf(stderr, "tcgetattr error\n");
        return -1;
    }

    new->c_lflag &= ~((tcflag_t)(ICANON | ECHO | ISIG));
    new->c_iflag     = 0;
    new->c_cc[VMIN]  = 0;
    new->c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSAFLUSH, new) == -1) {
        fprintf(stderr, "tcsetattr error\n");
        return -1;
    }

    if (ioctl(fd, KDSKBMODE, K_RAW)) {
        fprintf(stderr, "ioctl KDSKBMODE error\n");
        return -1;
    }

    return fd;
}


enum InputMode mode_set(enum InputMode mode, struct termios* old, struct termios* new, int *fd) {
    // Setup extkeys
    if (mode == EXTKEYS) {
        fprintf(stderr, "\e[>11u");
        fprintf(stderr, "\e[?u");
        char *response = "\e[?11u";

        nodelay(stdscr, 0);
        timeout(100);
        char c = getch();
        nodelay(stdscr, 1);

        for (int8_t i = 0; i < 6; i++) {
            if (c != response[i]) {
                mode = SCANCODES;
            }
            c = getch();
        }
    }

    // Setup fd to read from console
    if (mode == SCANCODES) {
        *fd = getfd(old, new);
        if (*fd < 0) {
            mode = NORM;
        }
    }
    return mode;
}

void update_input(Config *config, int8_t inputs[KEYS], uint32_t key, int8_t pressed) {
    if (key == config->left)  { inputs[0] = pressed; return; }
    if (key == config->right) { inputs[1] = pressed; return; }
    if (key == config->sd)    { inputs[2] = pressed; return; }
    if (key == config->hd)    { inputs[3] = pressed; return; }
    if (key == config->ccw)   { inputs[4] = pressed; return; }
    if (key == config->cw)    { inputs[5] = pressed; return; }
    if (key == config->flip)  { inputs[6] = pressed; return; }
    if (key == config->hold)  { inputs[7] = pressed; return; }
    if (key == config->reset) { inputs[8] = pressed; return; }
    if (key == config->quit)  { inputs[9] = pressed; return; }
}

void input_clean(enum InputMode mode, struct termios *old, int fd) {
    // Cleanup extkeys
    if (mode == EXTKEYS)
        fprintf(stderr, "\e[<u");
    else if (mode == SCANCODES) {
        if (ioctl(fd, KDSKBMODE, K_UNICODE)) {
            fprintf(stderr, "ioctl KDSKBMODE error\n");
            close(fd);
            return;
        }
        if (tcsetattr(fd, 0, old) == -1) {
            fprintf(stderr, "tcsetattr error\n");
            close(fd);
            return;
        }
        close(fd);
    }
}

int map_arrows(char c) {
    switch (c) {
    case 'A':
        return KEY_UP;
        break;
    case 'B':
        return KEY_DOWN;
        break;
    case 'C':
        return KEY_RIGHT;
        break;
    case 'D':
        return KEY_LEFT;
        break;
    default:
        return 0;
    }
}

void get_extkeys_input(int8_t inputs[KEYS], Config *config) {
    int c;
    enum Parser state = INVALID;
    uint32_t key = 0;
    int8_t pressed = 1;
    while ((c = getch()) != ERR) {
        if (c == 27) {
            if (state < STATE)
                update_input(config, inputs, key, pressed);
            key = 0;
            pressed = 1;
            state = CODE;
            continue;
        }
        if (c == 'u')
            state = END;

        switch (state) {
        case CODE:
            if ('0' <= c && c <= '9') {
                key *= 10;
                key += (c - '0');
            } else if (c == ';') {
                state++;
            } else if ('A' <= c && c <= 'D') {
                key = map_arrows((char) c);
            } else if (c != '[')
                state = INVALID;
            break;
        case MOD:
            if (c == ':')
                state++;
            else if ('A' <= c && c <= 'D')
                key = map_arrows((char) c);
            break;
        case STATE:
            pressed = (c == '1');
            state++;
            break;
        case END:
            if (key == 1 && 'A' <= c && c <= 'D') {
                key = map_arrows((char) c);
            }
            update_input(config, inputs, key, pressed);
            state++;
            break;
        case INVALID:
            break;
        }
    }
    if (state < STATE)
        update_input(config, inputs, key, pressed);
}

void get_scan_input(int fd, int8_t inputs[KEYS], Config *config) {
    unsigned char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf));

    for (ssize_t i = 0; i < n; i++) {
        update_input(config, inputs, buf[i], 1);
        update_input(config, inputs, buf[i] ^ 0x80, 0);
    }
}

void get_norm_input(int8_t inputs[KEYS], Config *config) {
    int c;
    for (int i = 0; i < KEYS; i++) {
        inputs[i] = 0;
    }
    while ((c = getch()) != ERR) {
        update_input(config, inputs, (uint32_t) c, 1);
    }
}

void get_inputs(Config *config, int fd, int8_t inputs[KEYS]) {
    switch (config->mode) {
    case EXTKEYS:
        get_extkeys_input(inputs, config);
        break;
    case SCANCODES:
        get_scan_input(fd, inputs, config);
        break;
    case NORM:
        get_norm_input(inputs, config);
        break;
    }
}
