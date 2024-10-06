#include <stdlib.h>
#include <time.h>
#include "game.h"

// TODO: figure out better way/place to store this
const int8_t PIECES[BAG_SZ][4][4][2] = {
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

const int8_t OFFSETS[3][4][5][2] = {
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

const int8_t OFFSETS2[2][4][2][2] = {
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

void game_init() {
    srandom(time(NULL));
}

int8_t check_collide(int8_t board[ARR_HEIGHT][BOARD_WIDTH], int8_t x, int8_t y, int8_t type, int8_t rot) {
    for (int i = 0; i < 4; i++) {
        int minoY = y - PIECES[type][rot][i][1];
        int minoX = x + PIECES[type][rot][i][0];
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
        p->coords[i][0] = p->x + PIECES[p->type][p->rot][i][0];
        p->coords[i][1] = p->y - PIECES[p->type][p->rot][i][1];
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
        int8_t x = p->x + (OFFSETS[class][init_rot][i][0] - OFFSETS[class][p->rot][i][0]);
        int8_t y = p->y + (OFFSETS[class][init_rot][i][1] - OFFSETS[class][p->rot][i][1]);

        if (class != 2 && spin == 1) {
            x = p->x + (OFFSETS2[class][init_rot][i][0] - OFFSETS2[class][p->rot][i][0]);
            y = p->y + (OFFSETS2[class][init_rot][i][1] - OFFSETS2[class][p->rot][i][1]);
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
        p->coords[i][0] = p->x + PIECES[p->type][p->rot][i][0];
        p->coords[i][1] = p->y - PIECES[p->type][p->rot][i][1];
    }

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
        p->coords[i][0] = p->x + PIECES[type][0][i][0];
        p->coords[i][1] = p->y - PIECES[type][0][i][1];
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
