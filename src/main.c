#include <curses.h>
#include <termios.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "input.h"
#include "config.h"
#include "game.h"

#define WIDTH 38 + 7 + 1 + BOARD_WIDTH * 2 + 1 + 9
#define HEIGHT BOARD_HEIGHT + 6
#define RIGHT_MARGIN 46

#define FPS 60

#define COLOR_ORANGE 8

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
                  y + PIECES[type][rot][i][1],
                  2 * (x + PIECES[type][rot][i][0]),
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

    struct game_data data = { 0 };
    game_init(&data);
    int8_t board[ARR_HEIGHT][BOARD_WIDTH] = { 0 };

    mvprintw(offset_y + 11, offset_x + 53, "READY");
    draw_gui(offset_x + 45, offset_y);

    draw_queue(queue_win, data.queue, data.queue_pos);
    draw_hold(hold_win, data.hold, data.hold_used);
    draw_keys(key_win, data.inputs);
    draw_stats(stat_win, 0, 0, 0, 0);

    usleep(500000);
    mvprintw(offset_y + 11, offset_x + 53, " GO! ");
    refresh();
    usleep(500000);

    time_t start_time = get_ms();
    time_t game_time;

    data.queue_pos = queue_pop(data.curr, data.queue, data.bag, 0);

    // Game Loop
    while (1) {
        game_time = get_ms();
        if (game_tick(config, &data, board, fd)) break;

        // Updates
        draw_board(board_win, board, data.curr, config->goal - data.cleared, 0);
        draw_queue(queue_win, data.queue, data.queue_pos);
        draw_hold(hold_win, data.hold, data.hold_used);
        draw_keys(key_win, data.inputs);
        draw_stats(stat_win, game_time - start_time, data.pieces, data.keys, data.holds);

        usleep(1000000 / FPS - (get_ms() - game_time));
    }

    // Post game screen
    if (data.cleared >= config->goal) {
        draw_board(board_win, board, data.curr, 21, 1);
        draw_stats(stat_win, game_time - start_time, data.pieces, data.keys, data.holds);
        while (1) {
            get_inputs(config, fd, data.inputs);
            if (data.inputs[RESET] || data.inputs[QUIT])
                break;
            draw_keys(key_win, data.inputs);
            usleep(1000000 / FPS);
        }
    }

    free(data.curr);
    delwin(board_win);
    delwin(queue_win);
    delwin(hold_win);
    delwin(key_win);
    clear();

    return data.inputs[QUIT];
}

int main() {
    setlocale(LC_ALL, "");
    srandom(time(NULL));

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
