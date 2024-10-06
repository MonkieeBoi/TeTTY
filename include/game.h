#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define SPAWN_X ((BOARD_WIDTH - 1) / 2)
#define SPAWN_Y (BOARD_HEIGHT - 1)
#define SPAWN_ROT 0
#define QUEUE_SZ 5
#define BAG_SZ 7
#define BOARD_HEIGHT 20
#define ARR_HEIGHT 40
#define BOARD_WIDTH 10

typedef struct Piece {
    int8_t x;
    int8_t y;
    int8_t coords[4][2];
    uint8_t type;
    uint8_t rot;
} Piece;

// Defined by offset from the piece center
// 7 pieces, 4 rotations, 3 coordinate pairs
extern const int8_t PIECES[BAG_SZ][4][4][2];

void game_init();

int8_t check_collide(int8_t board[ARR_HEIGHT][BOARD_WIDTH], int8_t x, int8_t y, int8_t type, int8_t rot);

void move_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t h, int8_t amount);

void spin_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p, int8_t spin);

void lock_piece(int8_t board[ARR_HEIGHT][BOARD_WIDTH], Piece *p);

int8_t queue_pop(Piece *p, int8_t queue[], int8_t queue_pos);

void gen_piece(Piece *p, int8_t type);

void queue_init (int8_t queue[]);

int8_t clear_lines(int8_t board[ARR_HEIGHT][BOARD_WIDTH]);

#endif
