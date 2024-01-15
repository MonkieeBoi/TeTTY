#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>

#define BOARD_HEIGHT 20
#define BOARD_WIDTH 10
#define SPAWN_X 4
#define SPAWN_Y 0
#define SPAWN_ROT 0
#define FPS 60

int keys[10] = {
    68,  // Left  | ←
    67,  // Right | →
    66,  // SD    | ↓
    ' ', // HD    | Space
    'a', // CCW   | a
    's', // CW    | s
    'd', // 180   | d
    'z', // Hold  | z
    'r', // Reset | r
    'q'  // Quit  | q
};

typedef struct Node {
    unsigned char row[BOARD_WIDTH];
    struct Node *next;
} Node;

struct piece {
    unsigned char x;
    char y;
    char coords[4][2];
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

// 3 offset 'classes', 4 rotation states, 5 x & y offsets
int offsets[3][4][5][2] = {
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
int offsets2[2][4][2][2] = {
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

int count_nodes(Node *n) {
    int length = 0;
    while (n != NULL) {
        n = n->next;
        length++;
    }
    return length;
}

int check_collide(int board[][10], int height, int x, int y, int type, int rot) {
    for (int i = 0; i < 4; i++) {
        int minoY = y + pieces[type][rot][i][1];
        int minoX = x + pieces[type][rot][i][0];
        if (minoY >= BOARD_HEIGHT
          || minoX >= BOARD_WIDTH
          || minoX < 0
          || (BOARD_HEIGHT - 1 - minoY < height
              && board[BOARD_HEIGHT - 1 - minoY][minoX]))
            return 1;
    }
    return 0;
}

void move_piece(Node *n, struct piece *p, int h, int amount) {
    int height = count_nodes(n);

    int board[height][BOARD_WIDTH];
    for (int i = 0; n != NULL; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++)
            board[i][j] = n->row[j];
        n = n->next;
    }

    int collision = 0;
    int last_x = p->x;
    int last_y = p->y;
    int step = (amount < 0) ? -1 : 1;

    for (int i = step; i != amount + step; i += step) {
        int x = p->x + (h ? i : 0);
        int y = p->y + (h ? 0 : i);

        collision = check_collide(board, height, x, y, p->type, p->rot);

        if (!collision) {
            last_x = x;
            last_y = y;
        } else
            break;
    }

    p->x = last_x;
    p->y = last_y;

    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[p->type][p->rot][i][0];
        p->coords[i][1] = p->y + pieces[p->type][p->rot][i][1];
    }
}

void spin_piece(Node *n, struct piece *p, int spin) {
    // 0 = cw
    // 1 = 180
    // 2 = ccw
    int init_rot = p->rot;
    int class = 0;
    if (p->type == 0) class = 1;
    if (p->type == 3) class = 2;
    p->rot = (p->rot + spin + 1) % 4;

    int height = count_nodes(n);
    int board[height][BOARD_WIDTH];
    for (int i = 0; n != NULL; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++)
            board[i][j] = n->row[j];
        n = n->next;
    }

    int collision = 0;
    for (int i = 0; i < 5; i++) {
        int x = p->x + (offsets[class][init_rot][i][0] - offsets[class][p->rot][i][0]);
        int y = p->y - (offsets[class][init_rot][i][1] - offsets[class][p->rot][i][1]);

        if (class != 2 && spin == 1) {
            x = p->x + (offsets2[class][init_rot][i][0] - offsets2[class][p->rot][i][0]);
            y = p->y - (offsets2[class][init_rot][i][1] - offsets2[class][p->rot][i][1]);
            if (i > 2)
                break;
        }

        collision = check_collide(board, height, x, y, p->type, p->rot);

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

    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[p->type][p->rot][i][0];
        p->coords[i][1] = p->y + pieces[p->type][p->rot][i][1];
    }

}

void draw_gui(Node *n, struct piece *p, int x, int y) {
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        mvprintw(y + i, x, "│");
        mvprintw(y + i, x + 1 + BOARD_WIDTH * 2, "│");
    }
    mvprintw(y + BOARD_HEIGHT, x, "└────────────────────┘");
}

void draw_piece(WINDOW *w, int x, int y, int type, int rot, int ghost) {
    for (int i = 0; i < 4; i++) {
        wattron(w, COLOR_PAIR(ghost ? 8 : (type + 1)));
        mvwprintw(w,
                  y + pieces[type][rot][i][1],
                  2 * (x + pieces[type][rot][i][0]),
                  "[]"
        );
        wattroff(w, COLOR_PAIR(ghost ? 8 : (type + 1)));
    }
}

void draw_board(WINDOW *w, Node *n, struct piece *p) {
    werase(w);

    int orig_y = p->y;
    move_piece(n, p, 0, BOARD_HEIGHT - p->y);
    draw_piece(w, p->x, p->y, p->type, p->rot, 1);
    p->y = orig_y;

    int y = BOARD_HEIGHT - 1;
    while (n != NULL) {
        for (int i = 0; i < BOARD_WIDTH; i++) {
            if (n->row[i]) {
                wattron(w, COLOR_PAIR(n->row[i]));
                mvwprintw(w, y, 2 * i, "[]");
                wattroff(w, COLOR_PAIR(n->row[i]));
            }
        }
        n = n->next;
        y--;
    }
    draw_piece(w, p->x, p->y, p->type, p->rot, 0);
    wrefresh(w);
}

void draw_queue(WINDOW *w, int queue[], int queue_pos) {
    werase(w);
    for (int i = 0; i < 5; i++) {
        draw_piece(w, 1, 2 + 3 * i, queue[queue_pos], 0, 0);
        queue_pos = (queue_pos + 1) % 7;
    }
    wrefresh(w);
}

void draw_hold(WINDOW *w, int p) {
    werase(w);
    if (p != -1)
        draw_piece(w, 1, 1, p, 0, 0);
    wrefresh(w);
}

void draw_keys(WINDOW *w, int inputs[]) {
    werase(w);

    wattron(w, COLOR_PAIR(10));

    mvwprintw(w, 0, 0, "                            ");
    mvwprintw(w, 1, 0, "   ↕      (      )      /   ");
    mvwprintw(w, 2, 0, "                            ");
    mvwprintw(w, 3, 0, "                            ");
    mvwprintw(w, 4, 0, "   ←      ↓      →      ▼   ");
    mvwprintw(w, 5, 0, "                            ");

    wattroff(w, COLOR_PAIR(10));
    wattron(w, COLOR_PAIR(9));

    if (inputs[0]) {
        mvwprintw(w, 3, 0, "       ");
        mvwprintw(w, 4, 0, "   ←   ");
        mvwprintw(w, 5, 0, "       ");
    }
    if (inputs[1]) {
        mvwprintw(w, 3, 14, "       ");
        mvwprintw(w, 4, 14, "   →   ");
        mvwprintw(w, 5, 14, "       ");
    }
    if (inputs[2]) {
        mvwprintw(w, 3, 7, "       ");
        mvwprintw(w, 4, 7, "   ↓   ");
        mvwprintw(w, 5, 7, "       ");
    }
    if (inputs[3]) {
        mvwprintw(w, 3, 21, "       ");
        mvwprintw(w, 4, 21, "   ▼   ");
        mvwprintw(w, 5, 21, "       ");
    }
    if (inputs[4]) {
        mvwprintw(w, 0, 7, "       ");
        mvwprintw(w, 1, 7, "   (   ");
        mvwprintw(w, 2, 7, "       ");
    }
    if (inputs[5]) {
        mvwprintw(w, 0, 14, "       ");
        mvwprintw(w, 1, 14, "   )   ");
        mvwprintw(w, 2, 14, "       ");
    }
    if (inputs[6]) {
        mvwprintw(w, 0, 21, "       ");
        mvwprintw(w, 1, 21, "   /   ");
        mvwprintw(w, 2, 21, "       ");
    }
    if (inputs[7]) {
        mvwprintw(w, 0, 0, "       ");
        mvwprintw(w, 1, 0, "   ↕   ");
        mvwprintw(w, 2, 0, "       ");
    }

    wattroff(w, COLOR_PAIR(9));
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
        n->row[p->coords[i][0]] = p->type + 1;
    }
}

void gen_piece(struct piece *p, int type) {
    p->type = type;
    p->rot = SPAWN_ROT;
    p->x = SPAWN_X;
    p->y = SPAWN_Y;
    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[type][0][i][0];
        p->coords[i][1] = p->y + pieces[type][0][i][1];
    }
}

int queue_pop(struct piece *p, int queue[], int queue_pos) {
    gen_piece(p, queue[queue_pos]);
    int rand = random() % 7;

    int used[7] = {0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < queue_pos; i++) {
        used[queue[i]] = 1;
    }

    while (used[rand] && queue_pos != 0)
        rand = random() % 7;

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

void get_inputs(WINDOW *w, int inputs[]) {
    int input = 0;
    for (int i = 0; i < 10; i++)
        inputs[i] = 0;
    while ((input = wgetch(w)) != ERR) {
        for (int i = 0; i < 10; i++) {
            if (keys[i] == input)
                inputs[i] = 1;
        }
    }
}

void free_nodes(Node *n) {
    Node *last;
    while (n != NULL) {
        last = n;
        n = n->next;
        free(last);
    }
}

int main() {
    srandom(time(NULL));
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    curs_set(0);
    start_color();
    use_default_colors();

    init_pair(1, COLOR_CYAN,    COLOR_CYAN);
    init_pair(2, COLOR_BLUE,    COLOR_BLUE);
    init_pair(3, COLOR_WHITE,   COLOR_WHITE);
    init_pair(4, COLOR_YELLOW,  COLOR_YELLOW);
    init_pair(5, COLOR_GREEN,   COLOR_GREEN);
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(7, COLOR_RED,     COLOR_RED);
    init_pair(8, COLOR_WHITE,   -1);
    init_pair(9, COLOR_BLUE,   COLOR_WHITE);
    init_pair(10, COLOR_WHITE,   COLOR_BLUE);

    if (COLS < 71 || LINES < 21) {
        endwin();
        printf("Screen too small\n");
        return 1;
    }

    int width = 44;
    int height = 21;
    int offset_x = (COLS - width) / 2;
    int offset_y = (LINES - height) / 2;

    if (offset_x < 29)
        offset_x = 29;

    WINDOW *board_win;
    board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH * 2, offset_y, offset_x + 11);
    nodelay(board_win, 1);

    WINDOW *queue_win;
    queue_win = newwin(15, 4 * 2, offset_y, offset_x + 33);

    WINDOW *hold_win;
    hold_win = newwin(2, 4 * 2, offset_y + 1, offset_x + 1);

    WINDOW *key_win;
    key_win = newwin(9, 28, offset_y + 6, offset_x - 29);

    Node *board = malloc(sizeof(Node));
    board->next = NULL;
    struct piece *curr = malloc(sizeof(struct piece));

    for (int i = 0; i < BOARD_WIDTH; i++)
        board->row[i] = 0;

    int hold = -1;
    int hold_used = 0;
    int queue[7];
    queue_init(queue);
    int queue_pos = queue_pop(curr, queue, 0);
    int inputs[10];
    
    float grav = 0.02;
    float grav_c = 0;

    draw_gui(board, curr, offset_x + 10, offset_y);
    refresh();

    // Game Loop
    while (1) {
        get_inputs(board_win, inputs);
        // Just proof of concept stuff
        if (inputs[0])
            move_piece(board, curr, 1, -1);
        if (inputs[1])
            move_piece(board, curr, 1, 1);
        if (inputs[2])
            move_piece(board, curr, 0, 1);
        if (inputs[3]) {
            move_piece(board, curr, 0, BOARD_HEIGHT - curr->y);
            lock_piece(board, curr);
            queue_pos = queue_pop(curr, queue, queue_pos);
            hold_used = 0;
            grav_c = 0;
        }
        if (inputs[4])
            spin_piece(board, curr, 2);
        if (inputs[5])
            spin_piece(board, curr, 0);
        if (inputs[6])
            spin_piece(board, curr, 1);
        if (inputs[7]) {
            if (hold == -1) {
                hold = curr->type;
                queue_pos = queue_pop(curr, queue, queue_pos);
            } else if (!hold_used) {
                int tmp = hold;
                hold = curr->type;
                gen_piece(curr, tmp);
            }
            hold_used = 1;
            grav_c = 0;
        }
        if (inputs[8]) {
            free_nodes(board);
            board = malloc(sizeof(Node));
            board->next = NULL;
            curr = malloc(sizeof(struct piece));
            for (int i = 0; i < BOARD_WIDTH; i++)
                board->row[i] = 0;
            hold = -1;
            queue_init(queue);
            queue_pos = queue_pop(curr, queue, 0);
            grav_c = 0;
        }
        if (inputs[9])
            break;

        clear_lines(board);

        // Updates
        draw_board(board_win, board, curr);
        draw_queue(queue_win, queue, queue_pos);
        draw_hold(hold_win, hold);
        draw_keys(key_win, inputs);

        // Gravity Movement
        grav_c += grav;
        move_piece(board, curr, 0, (int) grav_c);
        grav_c = grav_c - (int) grav_c;

        usleep(1000000 / FPS);
    }
    endwin();
    return 0;
}
