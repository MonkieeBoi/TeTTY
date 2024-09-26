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

static const uint32_t keys[][KEYS] = {
    // extended keyboard protocol keycodes
    {
        KEY_LEFT,  // Left  | ←
        KEY_RIGHT, // Right | →
        KEY_DOWN,  // SD    | ↓
        ' ',       // HD    | Space
        'a',       // CCW   | a
        's',       // CW    | s
        'd',       // 180   | d
        57441,     // Hold  | shift
        'r',       // Reset | r
        'q'        // Quit  | q
    },
    // scancodes
    {
        0x4b,      // Left  | ←
        0x4d,      // Right | →
        0x50,      // SD    | ↓
        0x39,      // HD    | Space
        0x1e,      // CCW   | a
        0x1f,      // CW    | s
        0x20,      // 180   | d
        0x2a,      // Hold  | shift
        0x13,      // Reset | r
        0x10       // Quit  | q
    },
    // normal keys
    {
        'D',       // Left  | ←
        'C',       // Right | →
        'B',       // SD    | ↓
        ' ',       // HD    | Space
        'a',       // CCW   | a
        's',       // CW    | s
        'd',       // 180   | d
        'z',       // Hold  | z
        'r',       // Reset | r
        'q'        // Quit  | q
    }
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

void get_extkeys_input(int8_t inputs[]) {
    char c;
    enum Parser state = INVALID;
    uint32_t key = 0;
    int8_t pressed = 0;
    while ((c = getch()) != ERR) {
        if (c == 27) {
            key = 0;
            pressed = 0;
            state = CODE;
            continue;
        }

        switch (state) {
        case CODE:
            if ('0' <= c && c <= '9') {
                key *= 10;
                key += (c - '0');
            } else if (c == ';')
                state++;
            else if (c != '[')
                state = INVALID;
            break;
        case MOD:
            if (c == ':') state++;
            break;
        case STATE:
            pressed = (c == '1');
            state++;
            break;
        case END:
            if (key == 1 && 'A' <= c && c <= 'D') {
                switch (c) {
                case 'A':
                    key = KEY_UP;
                    break;
                case 'B':
                    key = KEY_DOWN;
                    break;
                case 'C':
                    key = KEY_RIGHT;
                    break;
                case 'D':
                    key = KEY_LEFT;
                    break;
                }
            }
            for (int8_t i = 0; i < KEYS; i++) {
                if (keys[EXTKEYS][i] == key) {
                    inputs[i] = pressed;
                }
            }
            state++;
            break;
        case INVALID:
            break;
        }
    }
}

void get_scan_input(int fd, int8_t inputs[]) {
    unsigned char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf));

    for (ssize_t i = 0; i < n; i++) {
        for (int8_t j = 0; j < KEYS; j++) {
            if (keys[SCANCODES][j] == buf[i]) {
                inputs[j] = 1;
                continue;
            }
            if ((keys[SCANCODES][j] | 0x80) == buf[i])
                inputs[j] = 0;
        }
    }
}

void get_norm_input(int8_t inputs[]) {
    int c;
    for (int i = 0; i < KEYS; i++) {
        inputs[i] = 0;
    }
    while ((c = getch()) != ERR) {
        for (int i = 0; i < KEYS; i++) {
            if ((char) keys[NORM][i] == c) {
                inputs[i] = 1;
            }
        }
    }
}

void get_inputs(enum InputMode mode, int fd, int8_t inputs[]) {
    switch (mode) {
    case EXTKEYS:
        get_extkeys_input(inputs);
        break;
    case SCANCODES:
        get_scan_input(fd, inputs);
        break;
    case NORM:
        get_norm_input(inputs);
        break;
    }
}

