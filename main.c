#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>

#define BOARD_HEIGHT 20
#define BOARD_WIDTH 10
#define SPAWN_X 4
#define SPAWN_Y 0

typedef struct Node {
    unsigned char row[BOARD_WIDTH];
    struct Node *next;
} Node;

struct piece {
    unsigned char x;
    unsigned char y;
    unsigned char coords[4][2];
    unsigned char type;
    unsigned char rotation;
};

// TODO: figure out better way to store this
// Defined by offset from the piece center
// 7 pieces, 4 rotations, 3 coordinate pairs
const int pieces[7][4][4][2] = {
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

void draw_board(Node *n, struct piece *p) {
    clear();
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        mvprintw(i, 0, "%02d ┃", i);
        mvprintw(i, 24, "┃");
        if (n != NULL) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                if (n->row[j])
                    mvprintw(i, 2 * j + 4, "[]");
            }
            n = n->next;
        }
    }
    mvprintw(8, 30, "type: %d", p->type);
    mvprintw(9, 30, "x: %d", p->x);
    mvprintw(10, 30, "y: %d", p->y);
    mvprintw(p->y, 2 * p->x + 4, "<>");
    for (int i = 0; i < 4; i++)
        mvprintw(p->coords[i][1], 4 + 2 * (p->coords[i][0]), "[]");
    mvprintw(p->y, 2 * p->x + 4, "<>");
    mvprintw(20, 0, "   ┗━━━━━━━━━━━━━━━━━━━━┛");
}

void lock(Node *n, struct piece *p) {
    int y = 19;
    for (int i = 3; i >= 0; i--) {
        while (y > p->coords[i][1]) {
            if (n->next == NULL) {
                n->next = malloc(sizeof(Node));
                for (int j = 0; j < BOARD_WIDTH; j++)
                    n->next->row[j] = 0; 
            }
            n = n->next;
            y--;
        }
        n->row[p->coords[i][0]] = 1;
    }
}

void move_piece(Node *n, struct piece *p, int x, int y) {
    p->x += x;
    p->y += y;
    for (int i = 0; i < 4; i++) {
        p->coords[i][0] += x;
        p->coords[i][1] += y;
    }
}

void spin_piece(Node *n, struct piece *p, int spin) {
    // 0 = cw
    // 1 = 180
    // 2 = ccw
    p->rotation = (p->rotation + spin + 1) % 4;
    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[p->type][p->rotation][i][0];
        p->coords[i][1] = p->y + pieces[p->type][p->rotation][i][1];
    }

}

void gen_piece(struct piece *p, int type) {
    p->type = type;
    p->rotation = 0;
    p->x = 4;
    p->y = 0;
    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[type][0][i][0];
        p->coords[i][1] = p->y + pieces[type][0][i][1];
    }
}

int queue_pop(struct piece *p, int queue[], int queue_pos) {
    gen_piece(p, queue[queue_pos]);
    int rand = random() % 7;

    while (queue_pos != 0) {
        int dup = 0;
        rand = random() % 7;
        for (int i = 0; i < queue_pos; i++) {
            if (rand == queue[i])
                dup = 1;
        }
        if (!dup)
            break;
    }

    queue[queue_pos] = rand;
    return (queue_pos + 1) % 7;
    
}

void queue_init (int queue[]) {
    for (int i = 0; i < 7; i++) {
        int rand = random() % 7;
        while (i != 0) {
            int dup = 0;
            rand = random() % 7;
            for (int j = 0; j < i; j++) {
                if (rand == queue[j])
                    dup = 1;
            }
            if (!dup)
                break;
        }
        queue[i] = rand;
    }
}

int main() {
    Node *board = malloc(sizeof(Node));
    board->next = NULL;
    struct piece *curr = malloc(sizeof(struct piece));

    srandom(time(NULL));
    setlocale(LC_ALL, "");

    int hold = -1;
    int queue[7];
    queue_init(queue);

    int queue_pos = 0;
    queue_pos = queue_pop(curr, queue, queue_pos);

    initscr();
    raw();
    curs_set(0);
    draw_board(board, curr);

    // Game Loop
    while (1) {
        int input = getch();
        // Just proof of concept stuff
        if (input == 'q')
            break;
        else if (input == 32) {
            lock(board, curr);
            queue_pos = queue_pop(curr, queue, queue_pos);
        } else if (input == 65)
            move_piece(board, curr, 0, -1);
        else if (input == 66)
            move_piece(board, curr, 0, 1);
        else if (input == 67)
            move_piece(board, curr, 1, 0);
        else if (input == 68)
            move_piece(board, curr, -1, 0);
        else if (input == 97)
            spin_piece(board, curr, 2);
        else if (input == 115)
            spin_piece(board, curr, 0);
        else if (input == 100)
            spin_piece(board, curr, 1);
        else if (input == 110) {
            queue_pos = queue_pop(curr, queue, queue_pos);
        } else if (input == 122) {
            if (hold == -1) {
                hold = curr->type;
                queue_pos = queue_pop(curr, queue, queue_pos);
            } else {
                int tmp = hold;
                hold = curr->type;
                gen_piece(curr, tmp);
            }
        }
        draw_board(board, curr);
        mvprintw(11, 30, "input: %d", input);
        mvprintw(12, 30, "pos: %d", queue_pos);
        refresh();
    }
    endwin();
    return 0;
}
