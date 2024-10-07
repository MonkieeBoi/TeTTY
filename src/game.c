#include <stdlib.h>
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

void game_init(struct game_data* game) {
    game->hold              = -1;
    game->hold_used         = 0;
    game->queue_pos         = 0;
    game->ldas_c            = 0;
    game->rdas_c            = 0;
    game->grav_c            = 0;
    game->pieces            = 0;
    game->holds             = 0;
    game->keys              = 0;
    game->keys_buf          = 0;
    game->cleared           = 0;
    game->curr              = malloc(sizeof(Piece));

    queue_init(game->queue);
    for (int i = 0; i < KEYS; i++) {
        game->inputs[i] = 0;
        game->last_inputs[i] = 0;
    }
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

int8_t game_tick(Config *config, struct game_data *data, int8_t board[ARR_HEIGHT][BOARD_WIDTH], int fd) {
    for (int8_t i = 0; i < KEYS; i++)
        data->last_inputs[i] = data->inputs[i];
    get_inputs(config, fd, data->inputs);

    for (int8_t i = 0; i < 8; i++) {
        data->keys_buf += data->inputs[i] && !data->last_inputs[i];
    }

    if (data->inputs[RESET] || data->inputs[QUIT])
        return 1;
    if (data->inputs[HD] && !data->last_inputs[HD]) {
        move_piece(board, data->curr, 0, -data->curr->y);
        lock_piece(board, data->curr);
        data->queue_pos = queue_pop(data->curr, data->queue, data->queue_pos);
        data->cleared += clear_lines(board);
        data->hold_used = 0;
        data->grav_c = 0;
        data->pieces++;
        data->keys += data->keys_buf;
        data->keys_buf = 0;
        if (data->cleared >= config->goal)
            return 1;
    }

    if (data->inputs[LEFT] && data->rdas_c != config->das - 1) {
        data->ldas_c++;
    } else if (!data->inputs[LEFT] && data->ldas_c)
        data->ldas_c = 0;

    if (data->inputs[RIGHT] && data->ldas_c != config->das - 1) {
        data->rdas_c++;
    } else if (!data->inputs[RIGHT] && data->rdas_c)
        data->rdas_c = 0;

    if (data->ldas_c > config->das && (data->rdas_c == 0 || data->rdas_c > data->ldas_c))
        move_piece(board, data->curr, 1, -BOARD_WIDTH);
    if (data->rdas_c > config->das && (data->ldas_c == 0 || data->ldas_c > data->rdas_c))
        move_piece(board, data->curr, 1, BOARD_WIDTH);

    if (data->inputs[LEFT] && !data->last_inputs[LEFT])
        move_piece(board, data->curr, 1, -1);
    if (data->inputs[RIGHT] && !data->last_inputs[RIGHT])
        move_piece(board, data->curr, 1, 1);

    if (data->inputs[SD])
        move_piece(board, data->curr, 0, -data->curr->y);
    if (data->inputs[CCW] && !data->last_inputs[CCW])
        spin_piece(board, data->curr, 2);
    if (data->inputs[CW] && !data->last_inputs[CW])
        spin_piece(board, data->curr, 0);
    if (data->inputs[FLIP] && !data->last_inputs[FLIP])
        spin_piece(board, data->curr, 1);
    if (data->inputs[HOLD] && !data->last_inputs[HOLD]) {
        if (data->hold == -1) {
            data->hold = data->curr->type;
            data->queue_pos = queue_pop(data->curr, data->queue, data->queue_pos);
            data->holds++;
        } else if (!data->hold_used) {
            int8_t tmp = data->hold;
            data->hold = data->curr->type;
            gen_piece(data->curr, tmp);
            data->holds++;
        }
        data->hold_used = 1;
        data->grav_c = 0;
    }

    // Gravity Movement
    data->grav_c += config->grav;
    move_piece(board, data->curr, 0, -(data->grav_c / 1000));
    data->grav_c %= 1000;
    return 0;
}
