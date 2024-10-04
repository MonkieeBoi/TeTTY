#include <curses.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "input.h"
#include "config.h"

#define BOARD_HEIGHT 20
#define ARR_HEIGHT 40
#define BOARD_WIDTH 10
#define WIDTH 38 + 7 + 1 + BOARD_WIDTH * 2 + 1 + 9
#define HEIGHT BOARD_HEIGHT + 6
#define RIGHT_MARGIN 46

#define SPAWN_X ((BOARD_WIDTH - 1) / 2)
#define SPAWN_Y (BOARD_HEIGHT - 1)
#define SPAWN_ROT 0
#define FPS 60
#define DAS 5
#define CLEAR_GOAL 40
#define QUEUE_SZ 5
#define BAG_SZ 7

#define LEFT 0
#define RIGHT 1
#define SD 2
#define HD 3
#define CCW 4
#define CW 5
#define FLIP 6
#define HOLD 7
#define RESET 8
#define QUIT 9

#define COLOR_ORANGE 8

typedef struct Piece {
    int8_t x;
    int8_t y;
    int8_t coords[4][2];
    uint8_t type;
    uint8_t rot;
} Piece;

// TODO: figure out better way/place to store this
// Defined by offset from the piece center
// 7 pieces, 4 rotations, 3 coordinate pairs
const int8_t pieces[BAG_SZ][4][4][2] = {
    // I
    {
        {{-1, 0}, {0, 0}, {1, 0}, {2, 0}},
        // []<>[][]
        {{0, -1}, {0, 0}, {0, 1}, {0, 2}},
        // []
        // <>
        // []
        // []
        {{-2, 0}, {-1, 0}, {0, 0}, {1, 0}},
        // [][]<>[]
        {{0, -2}, {0, -1}, {0, 0}, {0, 1}},
        // []
        // []
        // <>
        // []
    },
    // J
    {
        {{-1, -1}, {-1, 0}, {0, 0}, {1, 0}},
         // []
         // []<>[]
        {{0, -1}, {1, -1}, {0, 0}, {0, 1}},
         // [][]
         // <>
         // []
        {{-1, 0}, {0, 0}, {1, 0}, {1, 1}},
         // []<>[]
         //     []
        {{0, -1}, {0, 0}, {-1, 1}, {0, 1}},
         //   []
         //   <>
         // [][]
    },
    // L
    {
        {{1, -1}, {-1, 0}, {0, 0}, {1, 0}},
        //     []
        // []<>[]
        {{0, -1}, {0, 0}, {0, 1}, {1, 1}},
        // []
        // <>
        // [][]
        {{-1, 0}, {0, 0}, {1, 0}, {-1, 1}},
        // []<>[]
        // []
        {{-1, -1}, {0, -1}, {0, 0}, {0, 1}},
        // [][]
        //   <>
        //   []
    },
    // O
    {
        {{0, -1}, {1, -1}, {0, 0}, {1, 0}},
        // [][]
        // <>[]
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
        // <>[]
        // [][]
        {{-1, 0}, {0, 0}, {-1, 1}, {0, 1}},
        // []<>
        // [][]
        {{-1, -1}, {0, -1}, {-1, 0}, {0, 0}}
        // [][]
        // []<>
    },
    // S
    {
        {{0, -1}, {1, -1}, {-1, 0}, {0, 0}},
        //   [][]
        // []<>
        {{0, -1}, {0, 0}, {1, 0}, {1, 1}},
        // []
        // <>[]
        //   []
        {{0, 0}, {1, 0}, {-1, 1}, {0, 1}},
        //   <>[]
        // [][]
        {{-1, -1}, {-1, 0}, {0, 0}, {0, 1}}
        // []
        // []<>
        //   []
    },
    // T
    {
        {{0, -1}, {-1, 0},{0, 0},  {1, 0}},
        //   []
        // []<>[]
        {{0, -1}, {0, 0}, {1, 0}, {0, 1}},
        // []
        // <>[]
        // []
        {{-1, 0}, {0, 0}, {1, 0}, {0, 1}},
        // []<>[]
        //   []
        {{0, -1}, {-1, 0}, {0, 0}, {0, 1}}
        //   []
        // []<>
        //   []
    },
    // Z
    {
        {{-1, -1}, {0, -1}, {0, 0}, {1, 0}},
        // [][]
        //   <>[]
        {{1, -1}, {0, 0}, {1, 0}, {0, 1}},
        //   []
        // <>[]
        // []
        {{-1, 0}, {0, 0}, {0, 1}, {1, 1}},
        // []<>
        //   [][]
        {{0, -1}, {-1, 0}, {0, 0}, {-1, 1}}
        //   []
        // []<>
        // []
    },
};

// 3 offset 'classes', 4 rotations, 5 xy offsets
const int8_t offsets[3][4][5][2] = {
    // J, L, S, T, Z
    {
        // Spawn
        {{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}},
        // CW
        {{ 0, 0}, { 1, 0}, { 1,-1}, { 0, 2}, { 1, 2}},
        // 180
        {{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0}},
        // CCW
        {{ 0, 0}, {-1, 0}, {-1,-1}, { 0, 2}, {-1, 2}},
    },
    // I
    {
        // Spawn
        {{ 0, 0}, {-1, 0}, { 2, 0}, {-1, 0}, { 2, 0}},
        // CW
        {{-1, 0}, { 0, 0}, { 0, 0}, { 0, 1}, { 0,-2}},
        // 180
        {{-1, 1}, { 1, 1}, {-2, 1}, { 1, 0}, {-2, 0}},
        // CCW
        {{ 0, 1}, { 0, 1}, { 0, 1}, { 0,-1}, { 0, 2}},
    },
    // O
    {
        // Spawn
        {{ 0, 0}},
        // CW
        {{ 0,-1}},
        // 180
        {{-1,-1}},
        // CCW
        {{-1, 0}},
    },
};

// 180 offset table
const int8_t offsets2[2][4][2][2] = {
    {
        // Spawn
        {{ 0, 0}, { 0, 1}},
        // CW
        {{ 0, 0}, { 1, 0}},
        // 180
        {{ 0, 0}, { 0, 0}},
        // CCW
        {{ 0, 0}, { 0, 0}}
    },
    {
        // Spawn
        {{ 1, 0}, { 1, 0}},
        // CW
        {{-1, 0}, { 0, 0}},
        // 180
        {{ 0, 1}, { 0, 0}},
        // CCW
        {{ 0, 1}, { 0, 1}},
    }
};

void init_curses () {
    initscr();
    raw();
    curs_set(0);
    start_color();
    noecho();
    use_default_colors();
    nodelay(stdscr, 1);

    // Base pieces
    init_pair(1,  COLOR_CYAN,    -1);
    init_pair(2,  COLOR_BLUE,    -1);
    init_pair(3,  COLOR_WHITE,   -1);
    init_pair(4,  COLOR_YELLOW,  -1);
    init_pair(5,  COLOR_GREEN,   -1);
    init_pair(6,  COLOR_MAGENTA, -1);
    init_pair(7,  COLOR_RED,     -1);

    // End screen board + pressed key bg
    init_pair(8,  COLOR_WHITE,   -1);

    // Pressed key text
    init_pair(9,  COLOR_BLUE,    COLOR_WHITE);

    // Base key text
    init_pair(10, COLOR_WHITE,   COLOR_BLUE);

    // Base key bg
    init_pair(11, COLOR_BLUE,    -1);

    // Make orange if supported
    if (COLORS > 8) {
        init_color(COLOR_ORANGE, 816, 529, 439);
        init_pair(3,  COLOR_ORANGE,  -1);
    }
}

time_t get_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
}

int8_t check_collide(int8_t board[ARR_HEIGHT][BOARD_WIDTH], int8_t x, int8_t y, int8_t type, int8_t rot) {
    for (int i = 0; i < 4; i++) {
        int minoY = y - pieces[type][rot][i][1];
        int minoX = x + pieces[type][rot][i][0];
        if (minoY >= ARR_HEIGHT
          || minoX >= BOARD_WIDTH
          || minoX < 0
          || minoY < 0
          || board[minoY][minoX])
            return 1;
    }
    return 0;
}

void move_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t h, int8_t amount) {
    int8_t collision = 0;
    int8_t last_x = p->x;
    int8_t last_y = p->y;
    int8_t step = (amount < 0) ? -1 : 1;

    for (int8_t i = step; i != amount + step; i += step) {
        int8_t x = p->x + (h ? i : 0);
        int8_t y = p->y + (h ? 0 : i);

        collision = check_collide(board, x, y, p->type, p->rot);

        if (!collision) {
            last_x = x;
            last_y = y;
        } else
            break;
    }

    p->x = last_x;
    p->y = last_y;

    for (int8_t i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[p->type][p->rot][i][0];
        p->coords[i][1] = p->y - pieces[p->type][p->rot][i][1];
    }
}

void spin_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t spin) {
    // 0 = cw
    // 1 = 180
    // 2 = ccw
    int8_t init_rot = p->rot;
    int8_t class = 0;
    if (p->type == 0) class = 1;
    if (p->type == 3) class = 2;
    p->rot = (p->rot + spin + 1) % 4;

    int8_t collision = 0;
    for (int8_t i = 0; i < 5; i++) {
        int8_t x = p->x + (offsets[class][init_rot][i][0] - offsets[class][p->rot][i][0]);
        int8_t y = p->y + (offsets[class][init_rot][i][1] - offsets[class][p->rot][i][1]);

        if (class != 2 && spin == 1) {
            x = p->x + (offsets2[class][init_rot][i][0] - offsets2[class][p->rot][i][0]);
            y = p->y + (offsets2[class][init_rot][i][1] - offsets2[class][p->rot][i][1]);
            if (i > 2)
                break;
        }

        collision = check_collide(board, x, y, p->type, p->rot);

        if (!collision) {
            p->x = x;
            p->y = y;
            break;
        }
    }

    if (collision) {
        p->rot = init_rot;
        return;
    }

    for (int8_t i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[p->type][p->rot][i][0];
        p->coords[i][1] = p->y - pieces[p->type][p->rot][i][1];
    }

}

void draw_gui(int8_t x, int8_t y) {
    for (int8_t i = BOARD_HEIGHT - 1; i >= 0; i--) {
        mvprintw(y + i, x, "█");
        mvprintw(y + i, x + 1 + BOARD_WIDTH * 2, "█");
    }
    for (int8_t i = 0; i < BOARD_WIDTH + 1; i++)
        mvprintw(y + BOARD_HEIGHT, x + i * 2, "▀▀");
    refresh();
}

void draw_piece(WINDOW *w, int8_t x, int8_t y, int8_t type, int8_t rot, int8_t ghost) {
    for (int8_t i = 0; i < 4; i++) {
        wattron(w, COLOR_PAIR(type + 1));
        mvwprintw(w,
                  y + pieces[type][rot][i][1],
                  2 * (x + pieces[type][rot][i][0]),
                  ghost ? "▓▓" : "██"
        );
        wattroff(w, COLOR_PAIR(type + 1));
    }
}

void draw_board(WINDOW *w, int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t line, int8_t mono) {
    werase(w);

    int8_t orig_y = p->y;
    move_piece(board, p, 0, -p->y);
    int8_t ghost_y = p->y;
    p->y = orig_y;

    for (int8_t i = 0; i < BOARD_HEIGHT; i++) {
        for (int8_t j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j]) {
                wattron(w, COLOR_PAIR(mono ? 8 : board[i][j]));
                mvwprintw(w, BOARD_HEIGHT - 1 - i, 2 * j, mono ? "▓▓" : "██");
                wattroff(w, COLOR_PAIR(mono ? 8 : board[i][j]));
            } else if (i == line) {
                mvwprintw(w, BOARD_HEIGHT - 1 - i, 2 * j, "__");
            }
        }
    }

    if (!mono) {
        draw_piece(w, p->x, BOARD_HEIGHT - 1 - ghost_y, p->type, p->rot, 1);
        draw_piece(w, p->x, BOARD_HEIGHT - 1 - p->y, p->type, p->rot, 0);
    }
    wrefresh(w);
}

void draw_queue(WINDOW *w, int8_t queue[], int8_t queue_pos) {
    werase(w);
    for (int8_t i = 0; i < QUEUE_SZ; i++) {
        draw_piece(w, 1, 2 + 3 * i, queue[queue_pos], 0, 0);
        queue_pos = (queue_pos + 1) % BAG_SZ;
    }
    wrefresh(w);
}

void draw_hold(WINDOW *w, int8_t p, int8_t held) {
    werase(w);
    if (p != -1) {
        draw_piece(w, 1, 1, p, 0, held);
    }
    wrefresh(w);
}

void draw_keys(WINDOW *w, int8_t inputs[KEYS]) {
    werase(w);
    // by top left corner (y, x)
    const int key_pos[KEYS - 2][2] = {
        { 4, 23 },
        { 4, 28 },
        { 4, 33 },
        { 4, 15 },
        { 0,  5 },
        { 0, 10 },
        { 0, 15 },
        { 2,  0 },
    };

    const char *key_chars[KEYS - 2] = {
        "←",
        "→",
        "↓",
        "▼",
        "(",
        ")",
        "/",
        "↕"
    };

    // base key display
    wattron(w, COLOR_PAIR(11));
    for (int i = 0; i < KEYS - 2; i++) {
        mvwprintw(w, key_pos[i][0]    , key_pos[i][1], "▄▄▄▄▄");
        mvwprintw(w, key_pos[i][0] + 2, key_pos[i][1], "▀▀▀▀▀");
    }
    wattroff(w, COLOR_PAIR(11));

    wattron(w, COLOR_PAIR(10));
    for (int i = 0; i < KEYS - 2; i++) {
        mvwprintw(w, key_pos[i][0] + 1, key_pos[i][1], "  %s  ", key_chars[i]);
    }
    wattroff(w, COLOR_PAIR(10));

    // pressed keys
    wattron(w, COLOR_PAIR(8));
    for (int i = 0; i < KEYS - 2; i++) {
        if (inputs[i]) {
            mvwprintw(w, key_pos[i][0]    , key_pos[i][1], "▄▄▄▄▄");
            mvwprintw(w, key_pos[i][0] + 2, key_pos[i][1], "▀▀▀▀▀");
        }
    }
    wattroff(w, COLOR_PAIR(8));

    wattron(w, COLOR_PAIR(9));
    for (int i = 0; i < KEYS - 2; i++) {
        if (inputs[i]) {
            mvwprintw(w, key_pos[i][0] + 1, key_pos[i][1], "  %s  ", key_chars[i]);
        }
    }
    wattroff(w, COLOR_PAIR(9));

    wrefresh(w);
}

void draw_stats(WINDOW *w, int time, int pieces, int keys, int holds) {
    werase(w);

    int min = time / 60000;
    int sec = (time / 1000) % 60;
    int csec = (time / 10) % 100;

    if (min)
        mvwprintw(w, 0, 0, "%6s %d:%02d.%d", "Time", min, sec, csec);
    else
        mvwprintw(w, 0, 0, "%6s %d.%d", "Time", sec, csec);

    mvwprintw(w, 1, 0, "%6s %.2f", "PPS", pieces ? pieces / ((float) time / 1000) : 0);
    mvwprintw(w, 2, 0, "%6s %.2f", "KPP", pieces ? (float) keys / pieces : 0);
    mvwprintw(w, 3, 0, "%6s %d", "Hold", holds);
    mvwprintw(w, 4, 0, "%6s %d", "#", pieces);
    wrefresh(w);
}

void lock_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p) {
    for (int8_t i = 0; i < 4; i++)
        board[p->coords[i][1]][p->coords[i][0]] = p->type + 1;
}

void gen_piece(Piece *p, int8_t type) {
    p->type = type;
    p->rot = SPAWN_ROT;
    p->x = SPAWN_X;
    p->y = SPAWN_Y;
    for (int8_t i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[type][0][i][0];
        p->coords[i][1] = p->y - pieces[type][0][i][1];
    }
}

int8_t queue_pop(Piece *p, int8_t queue[], int8_t queue_pos) {
    gen_piece(p, queue[queue_pos]);

    int8_t rand = random() % (BAG_SZ - queue_pos);
    int8_t used[BAG_SZ] = { 0 };
    int8_t bag[BAG_SZ];
    int8_t bag_pos = 0;

    for (int8_t i = 0; i < queue_pos; i++)
        used[queue[i]] = 1;

    for (int8_t i = 0; i < BAG_SZ; i++)
        if (!used[i])
            bag[bag_pos++] = i;

    queue[queue_pos] = bag[rand];
    return (queue_pos + 1) % BAG_SZ;

}

void queue_init (int8_t queue[]) {
    int8_t bag[BAG_SZ];
    for (int8_t i = 0; i < BAG_SZ; i++)
        bag[i] = i;

    for (int8_t i = BAG_SZ - 1; i > 0; i--) {
        int8_t rand = random() % (i + 1);
        queue[BAG_SZ - 1 - i] = bag[rand];
        bag[rand] = bag[i];
    }

    queue[BAG_SZ - 1] = bag[0];
}

int8_t clear_lines(int8_t board[ARR_HEIGHT][BOARD_WIDTH]) {
    int8_t cleared = 0;
    for (int8_t i = 0; i < ARR_HEIGHT; i++) {
        int8_t clear = 1;
        for (int8_t j = 0; j < BOARD_WIDTH; j++) {
            if (!board[i][j]) {
                clear = 0;
                break;
            }
        }
        if (clear) {
            cleared++;
            continue;
        } if (cleared) {
            for (int8_t j = 0; j < BOARD_WIDTH; j++) {
                board[i-cleared][j] = board[i][j];
                if (i >= ARR_HEIGHT - cleared)
                    board[i][j] = 0;
                board[ARR_HEIGHT-1][j] = 0;
            }
        }
    }
    return cleared;
}

int8_t game(Config *config, int fd) {
    if (COLS < WIDTH || LINES < HEIGHT) {
        return 2;
    }

    // center board
    int offset_x = (COLS - BOARD_WIDTH * 2) / 2 - RIGHT_MARGIN;
    int offset_y = (LINES - HEIGHT) / 2 - 6;

    if (offset_x < 0)
        offset_x = 0;

    if (offset_y < 0)
        offset_y = 0;

    WINDOW *board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH * 2, offset_y, offset_x + RIGHT_MARGIN);
    WINDOW *queue_win = newwin(15, 4 * 2, offset_y, offset_x + RIGHT_MARGIN + BOARD_WIDTH * 2 + 2);
    WINDOW *hold_win = newwin(2, 4 * 2, offset_y + 1, offset_x + 36);
    WINDOW *key_win = newwin(7, 38, offset_y + 3, offset_x);
    WINDOW *stat_win = newwin(5, 14, offset_y + BOARD_HEIGHT + 1, offset_x + RIGHT_MARGIN + 3);

    Piece *curr = malloc(sizeof(Piece));
    int8_t board[ARR_HEIGHT][BOARD_WIDTH];
    for (int8_t i = 0; i < ARR_HEIGHT; i++)
        for (int8_t j = 0; j < BOARD_WIDTH; j++)
            board[i][j] = 0;

    int8_t hold = -1;
    int8_t hold_used = 0;
    int8_t queue[BAG_SZ];
    queue_init(queue);
    int8_t queue_pos = 0;
    int8_t inputs[KEYS] = { 0 };
    int8_t last_inputs[KEYS] = { 0 };

    uint32_t grav = 20; // units of 1/1000 blocks per frame
    uint32_t grav_c = 0;
    uint8_t ldas_c = 0;
    uint8_t rdas_c = 0;

    uint32_t pieces = 0;
    uint32_t holds = 0;
    uint32_t keys = 0;
    uint32_t keys_tmp = 0;
    uint32_t cleared = 0;

    mvprintw(offset_y + 11, offset_x + 53, "READY");
    draw_gui(offset_x + 45, offset_y);

    draw_queue(queue_win, queue, queue_pos);
    draw_hold(hold_win, hold, hold_used);
    draw_keys(key_win, inputs);
    draw_stats(stat_win, 0, 0, 0, 0);

    usleep(500000);
    mvprintw(offset_y + 11, offset_x + 53, " GO! ");
    refresh();
    usleep(500000);

    time_t start_time = get_ms();
    time_t game_time;

    queue_pos = queue_pop(curr, queue, 0);

    // Game Loop
    while (1) {
        game_time = get_ms();
        for (int8_t i = 0; i < KEYS; i++)
            last_inputs[i] = inputs[i];
        get_inputs(config, fd, inputs);

        for (int8_t i = 0; i < 8; i++) {
            keys_tmp += inputs[i] && !last_inputs[i];
        }

        if (inputs[RESET] || inputs[QUIT])
            break;
        if (inputs[HD] && !last_inputs[HD]) {
            move_piece(board, curr, 0, -curr->y);
            lock_piece(board, curr);
            queue_pos = queue_pop(curr, queue, queue_pos);
            cleared += clear_lines(board);
            hold_used = 0;
            grav_c = 0;
            pieces++;
            keys += keys_tmp;
            keys_tmp = 0;
            if (cleared >= CLEAR_GOAL)
                break;
        }

        if (inputs[LEFT] && rdas_c != DAS - 1) {
            ldas_c++;
        } else if (!inputs[LEFT] && ldas_c)
            ldas_c = 0;

        if (inputs[RIGHT] && ldas_c != DAS - 1) {
            rdas_c++;
        } else if (!inputs[RIGHT] && rdas_c)
            rdas_c = 0;

        if (ldas_c > DAS && (rdas_c == 0 || rdas_c > ldas_c))
            move_piece(board, curr, 1, -BOARD_WIDTH);
        if (rdas_c > DAS && (ldas_c == 0 || ldas_c > rdas_c))
            move_piece(board, curr, 1, BOARD_WIDTH);

        if (inputs[LEFT] && !last_inputs[LEFT])
            move_piece(board, curr, 1, -1);
        if (inputs[RIGHT] && !last_inputs[RIGHT])
            move_piece(board, curr, 1, 1);

        if (inputs[SD])
            move_piece(board, curr, 0, -curr->y);
        if (inputs[CCW] && !last_inputs[CCW])
            spin_piece(board, curr, 2);
        if (inputs[CW] && !last_inputs[CW])
            spin_piece(board, curr, 0);
        if (inputs[FLIP] && !last_inputs[FLIP])
            spin_piece(board, curr, 1);
        if (inputs[HOLD] && !last_inputs[HOLD]) {
            if (hold == -1) {
                hold = curr->type;
                queue_pos = queue_pop(curr, queue, queue_pos);
                holds++;
            } else if (!hold_used) {
                int8_t tmp = hold;
                hold = curr->type;
                gen_piece(curr, tmp);
                holds++;
            }
            hold_used = 1;
            grav_c = 0;
        }

        // Updates
        draw_board(board_win, board, curr, CLEAR_GOAL - cleared, 0);
        draw_queue(queue_win, queue, queue_pos);
        draw_hold(hold_win, hold, hold_used);
        draw_keys(key_win, inputs);
        draw_stats(stat_win, game_time - start_time, pieces, keys, holds);

        // Gravity Movement
        grav_c += grav;
        move_piece(board, curr, 0, -(grav_c / 1000));
        grav_c %= 1000;

        usleep(1000000 / FPS - (get_ms() - game_time));
    }

    // Post game screen
    if (cleared >= CLEAR_GOAL) {
        draw_board(board_win, board, curr, 21, 1);
        draw_stats(stat_win, game_time - start_time, pieces, keys, holds);
        while (1) {
            get_inputs(config, fd, inputs);
            if (inputs[RESET] || inputs[QUIT])
                break;
            draw_keys(key_win, inputs);
            usleep(1000000 / FPS);
        }
    }

    free(curr);
    delwin(board_win);
    delwin(queue_win);
    delwin(hold_win);
    delwin(key_win);
    clear();

    return inputs[QUIT];
}

int main() {
    srandom(time(NULL));
    setlocale(LC_ALL, "");

    struct termios old;
    struct termios new;
    int fd = -1;
    Config config = { 0 };
    config.mode = EXTKEYS;

    init_curses();

    config.mode = mode_set(config.mode, &old, &new, &fd);
    config_init(&config);

    // Main loop
    int8_t status = 0;
    while (!(status = game(&config, fd)));

    // Cleanup 
    input_clean(config.mode, &old, fd);

    endwin();

    if (status == 2) {
        fprintf(stderr, "Screen dimensions smaller than %dx%d\n", WIDTH, HEIGHT);
    }

    return 0;
}
