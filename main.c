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
    unsigned char type;
    unsigned char rotation;
};

// TODO: figure out better way to store this
// Defined by offset from the piece center
// 7 pieces, 4 rotations, 3 coordinate pairs
const int pieces[7][4][3][2] = {
    // I
    {
        {{-1, 0}, {1, 0}, {2, 0}},
         // []<>[][]
        {{0, 1}, {0, -1}, {0, -2}},
        // []
        // <>
        // []
        // []
        {{-2, 0}, {-1, 0}, {1, 0}},
         // [][]<>[]
        {{0, 2}, {0, 1}, {0, -1}},
        // []
        // []
        // <>
        // []
    },
    // J
    {
        {{-1, 1}, {-1, 0}, {1, 0}},
         // []
         // []<>[]
        {{0, 1}, {1, 1}, {0, -1}},
         // [][]
         // <>
         // []
        {{-1, 0}, {1, 0}, {1, -1}},
         // []<>[]
         //     []
        {{0, 1}, {-1, -1}, {0, -1}},
         //   []
         //   <>
         // [][]
    },
    // L
    {
        {{1, 1}, {-1, 0}, {1, 0}},
        //     []
        // []<>[]
        {{0, 1}, {0, -1}, {1, -1}},
        // []
        // <>
        // [][]
        {{-1, 0}, {1, 0}, {-1, -1}},
        // []<>[]
        // []
        {{-1, 1}, {0, 1}, {0, -1}},
        // [][]
        //   <>
        //   []
    },
    // O
    {
        {{0, 1}, {1, 1}, {1, 0}},
        // [][]
        // <>[]
        {{1, 0}, {0, -1}, {1, -1}},
        // <>[]
        // [][]
        {{-1, 0}, {-1, -1}, {0, -1}},
        // []<>
        // [][]
        {{-1, 1}, {0, 1}, {-1, 0}}
        // [][]
        // []<>
    },
    // S
    {
        {{0, 1}, {1, 1}, {-1, 0}},
        //   [][]
        // []<>
        {{0, 1}, {1, 0}, {1, -1}},
        // []
        // <>[]
        //   []
        {{1, 0}, {-1, -1}, {0, -1}},
        //   <>[]
        // [][]
        {{-1, 1}, {-1, 0}, {0, -1}}
        // []
        // []<>
        //   []
    },
    // T
    {
        {{0, 1}, {-1, 0}, {1, 0}},
        //   []
        // []<>[]
        {{0, 1}, {1, 0}, {0, -1}},
        // []
        // <>[]
        // []
        {{-1, 0}, {1, 0}, {0, -1}},
        // []<>[]
        //   []
        {{0, 1}, {-1, 0}, {0, -1}}
        //   []
        // []<>
        //   []
    },
    // Z
    {
        {{-1, 1}, {0, 1}, {1, 0}},
        // [][]
        //   <>[]
        {{1, 1}, {1, 0}, {0, -1}},
        //   []
        // <>[]
        // []
        {{-1, 0}, {0, -1}, {1, -1}},
        // []<>
        //   [][]
        {{0, 1}, {-1, 0}, {-1, -1}}
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
                    mvprintw(i, j, "[]");
            }
            n = n->next;
        }
    }
    mvprintw(8, 30, "type: %d", p->type);
    mvprintw(9, 30, "x: %d", p->x+4);
    mvprintw(10, 30, "y: %d", p->y);
    mvprintw(p->y, 2 * p->x + 4, "<>");
    for (int k = 0; k < 3; k++) {
        mvprintw(p->y - pieces[p->type][p->rotation][k][1],
                 4 + 2 * (p->x + pieces[p->type][p->rotation][k][0]),
                 "[]");
    }
    mvprintw(20, 0, "   ┗━━━━━━━━━━━━━━━━━━━━┛");
}

int queue_pop(struct piece *p, int queue[], int queue_pos) {
    p->type = queue[queue_pos];
    p->x = SPAWN_X;
    p->y = SPAWN_Y;
    p->rotation = 0;
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

    int queue[7];
    queue_init(queue);

    int queue_pos = 0;
    queue_pos = queue_pop(curr, queue, queue_pos);

    initscr();
    raw();
    draw_board(board, curr);

    // Game Loop
    while (1) {
        int input = getch();
        if (input == 'q')
            break;
        else if (input == 65)
            curr->y = (curr->y > 0) ? curr->y - 1 : curr->y;
        else if (input == 66)
            curr->y = (curr->y < 19) ? curr->y + 1 : curr->y;
        else if (input == 67)
            curr->x = (curr->x < 9) ? curr->x + 1 : curr->x;
        else if (input == 68)
            curr->x = (curr->x > 0) ? curr->x - 1 : curr->x;
        else if (input == 110) {
            queue_pos = queue_pop(curr, queue, queue_pos);
        }
        draw_board(board, curr);
        mvprintw(11, 30, "input: %d", input);
        mvprintw(12, 30, "pos: %d", queue_pos);
        refresh();
    }
    endwin();
    return 0;
}
