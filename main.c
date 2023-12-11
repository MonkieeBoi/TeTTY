#include <curses.h>
#include <stdlib.h>
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
    unsigned char rot;
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

void draw_gui(Node *n, struct piece *p) {
    clear();
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        mvprintw(i, 10, "┃");
        mvprintw(i, 31, "┃");
    }
    mvprintw(20, 10, "┗━━━━━━━━━━━━━━━━━━━━┛");
}

void draw_piece(WINDOW *w, int x, int y, int type, int rot) {
    for (int i = 0; i < 4; i++)
        mvwprintw(w,
                  y + pieces[type][rot][i][1],
                  2 * (x + pieces[type][rot][i][0]),
                  "[]"
        );
}

void draw_board(WINDOW *w, Node *n, struct piece *p) {
    wclear(w);
    int y = BOARD_HEIGHT - 1;
    while (n != NULL) {
        for (int i = 0; i < BOARD_WIDTH; i++) {
            if (n->row[i])
                mvwprintw(w, y, 2 * i, "[]");
        }
        n = n->next;
        y--;
    }
    draw_piece(w, p->x, p->y, p->type, p->rot);
    mvwprintw(w, p->y, 2 * p->x, "<>");
    wrefresh(w);
}

void draw_queue(WINDOW *w, int queue[], int queue_pos) {
    wclear(w);
    for (int i = 0; i < 5; i++) {
        draw_piece(w, 1, 2 + 3 * i, queue[queue_pos], 0);
        queue_pos = (queue_pos + 1) % 7;
    }
    wrefresh(w);
}

void draw_hold(WINDOW *w, int p) {
    wclear(w);
    if (p != -1)
        draw_piece(w, 1, 1, p, 0);
    wrefresh(w);
}

void lock_piece(Node *n, struct piece *p) {
    int y = BOARD_HEIGHT - 1;
    for (int i = 3; i >= 0; i--) {
        while (y > p->coords[i][1]) {
            if (n->next == NULL) {
                n->next = malloc(sizeof(Node));
                n->next->next = NULL;
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
    p->rot = (p->rot + spin + 1) % 4;
    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[p->type][p->rot][i][0];
        p->coords[i][1] = p->y + pieces[p->type][p->rot][i][1];
    }

}

void gen_piece(struct piece *p, int type) {
    p->type = type;
    p->rot = 0;
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

void clear_lines(Node *n) {
    Node *head = n;
    int head_full = 1;
    n = n->next;

    // Check head
    for (int i = 0; i < BOARD_WIDTH && head_full; i++)
        if (head->row[i] == 0)
            head_full = 0;

    // Set head to next non-full row
    while (head_full && n != NULL) {
        int full = 1;
        for (int i = 0; full && i < BOARD_WIDTH; i++)
            if (n->row[i] == 0)
                full = 0;
        for (int i = 0; !full && i < BOARD_WIDTH; i++)
            head->row[i] = n->row[i];
        if (!full) {
            head_full = 0;
            head->next = n->next;
        }
        Node *tmp = n;
        n = n->next;
        free(tmp);
    }

    // Whole board full
    if (head_full) {
        for (int i = 0; i < BOARD_WIDTH; i++)
            head->row[i] = 0;
        head->next = NULL;
    }

    // Last non-full row
    Node *last = head;
    while (n != NULL) {
        int full = 1;
        for (int i = 0; full && i < BOARD_WIDTH; i++)
            if (n->row[i] == 0)
                full = 0;
        if (full) {
            Node *tmp = n;
            n = n->next;
            free(tmp);
        } else {
            last->next = n;
            last = n;
            n = n->next;
        }
    }
    last->next = NULL;
}

int main() {
    srandom(time(NULL));
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    curs_set(0);

    WINDOW *board_win;
    board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH * 2, 0, 11);

    WINDOW *queue_win;
    queue_win = newwin(15, 4 * 2, 0, 33);

    WINDOW *hold_win;
    hold_win = newwin(2, 4 * 2, 1, 1);

    Node *board = malloc(sizeof(Node));
    board->next = NULL;
    struct piece *curr = malloc(sizeof(struct piece));

    for (int i = 0; i < BOARD_WIDTH; i++)
        board->row[i] = 0;

    int hold = -1;
    int queue[7];
    queue_init(queue);

    int queue_pos = 0;
    queue_pos = queue_pop(curr, queue, queue_pos);

    draw_gui(board, curr);
    refresh();
    draw_board(board_win, board, curr);
    draw_queue(queue_win, queue, queue_pos);

    // Game Loop
    while (1) {
        int input = wgetch(board_win);
        // Just proof of concept stuff
        if (input == 'q')
            break;
        else if (input == 32) {
            lock_piece(board, curr);
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
        clear_lines(board);
        draw_board(board_win, board, curr);
        draw_queue(queue_win, queue, queue_pos);
        draw_hold(hold_win, hold);
    }
    endwin();
    return 0;
}
