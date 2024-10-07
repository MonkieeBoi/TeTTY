#ifndef GAME_H
#define GAME_H

#include "config.h"
#include "input.h"
#include <stdint.h>

#define SPAWN_X ((BOARD_WIDTH - 1) / 2)
#define SPAWN_Y (BOARD_HEIGHT - 1)
#define SPAWN_ROT 0
#define QUEUE_SZ 5
#define BAG_SZ 7
#define BOARD_HEIGHT 20
#define ARR_HEIGHT 40
#define BOARD_WIDTH 10

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

typedef struct Piece {
    int8_t x;
    int8_t y;
    int8_t coords[4][2];
    uint8_t type;
    uint8_t rot;
} Piece;

struct game_data {
    int8_t hold;
    int8_t hold_used;
    int8_t queue[BAG_SZ];
    int8_t queue_pos;
    int8_t inputs[KEYS];
    int8_t last_inputs[KEYS];
    uint8_t ldas_c;
    uint8_t rdas_c;
    uint32_t grav_c;
    uint32_t pieces;
    uint32_t holds;
    uint32_t keys;
    uint32_t keys_buf;
    uint32_t cleared;
    Piece *curr;
};

// Defined by offset from the piece center
// 7 pieces, 4 rotations, 3 coordinate pairs
extern const int8_t PIECES[BAG_SZ][4][4][2];

int8_t check_collide(int8_t board[ARR_HEIGHT][BOARD_WIDTH], int8_t x, int8_t y, int8_t type, int8_t rot);

void move_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t h, int8_t amount);

void spin_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t spin);

void lock_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p);

int8_t queue_pop(Piece *p, int8_t queue[], int8_t queue_pos);

void gen_piece(Piece *p, int8_t type);

void queue_init (int8_t queue[]);

int8_t clear_lines(int8_t board[ARR_HEIGHT][BOARD_WIDTH]);

void game_init(struct game_data* game);

int8_t game_tick(Config *config, struct game_data* game, int8_t board[ARR_HEIGHT][BOARD_WIDTH], int fd);

#endif
