#include <curses.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <locale.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define BOARD_HEIGHT 20
#define BOARD_WIDTH 10
#define SPAWN_X 4
#define SPAWN_Y 0
#define SPAWN_ROT 0
#define FPS 60
#define DAS 5

// scancodes
unsigned char keys[10] = {
    0x4b,  // Left  | ←
    0x4d,  // Right | →
    0x50,  // SD    | ↓
    0x39, // HD    | Space
    0x1e, // CCW   | a
    0x1f, // CW    | s
    0x20, // 180   | d
    0x2a, // Hold  | z
    0x13, // Reset | r
    0x10  // Quit  | q
};

typedef struct Node {
    unsigned char row[BOARD_WIDTH];
    struct Node *next;
} Node;

typedef struct Piece {
    unsigned char x;
    char y;
    char coords[4][2];
    unsigned char type;
    unsigned char rot;
} Piece;

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

static const char *conspath[] = {
	"/proc/self/fd/0",
	"/dev/tty",
	"/dev/tty0",
	"/dev/vc/0",
	"/dev/systty",
	"/dev/console",
	NULL
};

int is_a_console(int fd) {
	char arg = 0;
	return (isatty(fd) && ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84)));
}

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

void move_piece(Node *n, Piece *p, int h, int amount) {
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

void spin_piece(Node *n, Piece *p, int spin) {
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

void draw_gui(Node *n, Piece *p, int x, int y) {
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

void draw_board(WINDOW *w, Node *n, Piece *p) {
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

    wattron(w, COLOR_PAIR(12));
    mvwprintw(w, 0, 5, "▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄");
    mvwprintw(w, 2, 5, "▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
    mvwprintw(w, 4, 23, "▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄");
    mvwprintw(w, 6, 23, "▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
    mvwprintw(w, 2, 0, "▄▄▄▄▄");
    mvwprintw(w, 4, 0, "▀▀▀▀▀");
    mvwprintw(w, 4, 15, "▄▄▄▄▄");
    mvwprintw(w, 6, 15, "▀▀▀▀▀");
    wattroff(w, COLOR_PAIR(12));

    wattron(w, COLOR_PAIR(10));
    mvwprintw(w, 1, 5, "  (    )    /  ");
    mvwprintw(w, 3, 0, "  ↕  ");
    mvwprintw(w, 5, 15, "  ▼  ");
    mvwprintw(w, 5, 23, "  ←    ↓    →  ");
    wattroff(w, COLOR_PAIR(10));

    wattron(w, COLOR_PAIR(11));
    if (inputs[0]) {
        mvwprintw(w, 4, 23, "▄▄▄▄▄");
        mvwprintw(w, 6, 23, "▀▀▀▀▀");
    }
    if (inputs[1]) {
        mvwprintw(w, 4, 33, "▄▄▄▄▄");
        mvwprintw(w, 6, 33, "▀▀▀▀▀");
    }
    if (inputs[2]) {
        mvwprintw(w, 4, 28, "▄▄▄▄▄");
        mvwprintw(w, 6, 28, "▀▀▀▀▀");
    }
    if (inputs[3]) {
        mvwprintw(w, 4, 15, "▄▄▄▄▄");
        mvwprintw(w, 6, 15, "▀▀▀▀▀");
    }
    if (inputs[4]) {
        mvwprintw(w, 0, 5, "▄▄▄▄▄");
        mvwprintw(w, 2, 5, "▀▀▀▀▀");
    }
    if (inputs[5]) {
        mvwprintw(w, 0, 10, "▄▄▄▄▄");
        mvwprintw(w, 2, 10, "▀▀▀▀▀");
    }
    if (inputs[6]) {
        mvwprintw(w, 0, 15, "▄▄▄▄▄");
        mvwprintw(w, 2, 15, "▀▀▀▀▀");
    }
    if (inputs[7]) {
        mvwprintw(w, 2, 0, "▄▄▄▄▄");
        mvwprintw(w, 4, 0, "▀▀▀▀▀");
    }
    wattroff(w, COLOR_PAIR(11));

    wattron(w, COLOR_PAIR(9));
    if (inputs[0]) {
        mvwprintw(w, 5, 23, "  ←  ");
    }
    if (inputs[1]) {
        mvwprintw(w, 5, 33, "  →  ");
    }
    if (inputs[2]) {
        mvwprintw(w, 5, 28, "  ↓  ");
    }
    if (inputs[3]) {
        mvwprintw(w, 5, 15, "  ▼  ");
    }
    if (inputs[4]) {
        mvwprintw(w, 1, 5, "  (  ");
    }
    if (inputs[5]) {
        mvwprintw(w, 1, 10, "  )  ");
    }
    if (inputs[6]) {
        mvwprintw(w, 1, 15, "  /  ");
    }
    if (inputs[7]) {
        mvwprintw(w, 3, 0, "  ↕  ");
    }
    wattroff(w, COLOR_PAIR(9));

    wrefresh(w);
}

void lock_piece(Node *n, Piece *p) {
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

void gen_piece(Piece *p, int type) {
    p->type = type;
    p->rot = SPAWN_ROT;
    p->x = SPAWN_X;
    p->y = SPAWN_Y;
    for (int i = 0; i < 4; i++) {
        p->coords[i][0] = p->x + pieces[type][0][i][0];
        p->coords[i][1] = p->y + pieces[type][0][i][1];
    }
}

int queue_pop(Piece *p, int queue[], int queue_pos) {
    gen_piece(p, queue[queue_pos]);

    int rand = random() % (7 - queue_pos);
    int used[7] = {0};
    int bag[7];
    int bag_pos = 0;

    for (int i = 0; i < queue_pos; i++)
        used[queue[i]] = 1;

    for (int i = 0; i < 7; i++)
        if (!used[i])
            bag[bag_pos++] = i;

    queue[queue_pos] = bag[rand];
    return (queue_pos + 1) % 7;

}

void queue_init (int queue[]) {
    int bag[7] = {0, 1, 2, 3, 4, 5, 6};

    for (int i = 6; i > 0; i--) {
        int rand = random() % (i + 1);
        queue[6 - i] = bag[rand];

        for (int j = rand; j < i; j++)
            bag[j] = bag[j + 1];
    }

    queue[6] = bag[0];
}

int clear_lines(Node *n) {
    Node *head = n;
    int head_full = 1;
    n = n->next;
    int cleared = 0;

    // Check head
    for (int i = 0; i < BOARD_WIDTH && head_full; i++)
        if (head->row[i] == 0)
            head_full = 0;

    // Set head to next non-full row
    while (head_full && n != NULL) {
        int full = 1;
        // Check if current row full
        for (int i = 0; full && i < BOARD_WIDTH; i++)
            if (n->row[i] == 0)
                full = 0;
        // Copy row to head
        for (int i = 0; !full && i < BOARD_WIDTH; i++)
            head->row[i] = n->row[i];
        if (!full) {
            head_full = 0;
            head->next = n->next;
        }
        Node *tmp = n;
        n = n->next;
        free(tmp);
        cleared++;
    }

    cleared += head_full;

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
            cleared++;
        } else {
            last->next = n;
            last = n;
            n = n->next;
        }
    }
    last->next = NULL;

    return cleared;
}

void get_inputs(int fd, int inputs[]) {
	unsigned char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf));

    for (ssize_t i = 0; i < n; i++) {
        for (int j = 0; j < 10; j++) {
            if (keys[j] == buf[i])
                inputs[j] = 1;
            if ((keys[j] | 0x80) == buf[i])
                inputs[j] = 0;
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

int game(int fd) {
    if (COLS < 76 || LINES < 21) {
        printf("Screen too small\n");
        return 1;
    }

    int width = 44;
    int height = 21;
    int offset_x = (COLS - width) / 2;
    int offset_y = (LINES - height) / 2;

    if (offset_x < 35)
        offset_x = 35;

    WINDOW *board_win;
    board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH * 2, offset_y, offset_x + 11);
    nodelay(board_win, 1);

    WINDOW *queue_win;
    queue_win = newwin(15, 4 * 2, offset_y, offset_x + 33);

    WINDOW *hold_win;
    hold_win = newwin(2, 4 * 2, offset_y + 1, offset_x + 1);

    WINDOW *key_win;
    key_win = newwin(7, 38, offset_y + 3, offset_x - 35);

    Node *board = malloc(sizeof(Node));
    board->next = NULL;
    Piece *curr = malloc(sizeof(Piece));

    for (int i = 0; i < BOARD_WIDTH; i++)
        board->row[i] = 0;

    int hold = -1;
    int hold_used = 0;
    int queue[7];
    queue_init(queue);
    int queue_pos = 0;
    int inputs[10] = {0};
    int last_inputs[10] = {0};

    float grav = 0.02;
    float grav_c = 0;
    int ldas_c = 0;
    int rdas_c = 0;

    draw_gui(board, curr, offset_x + 10, offset_y);
    mvprintw(offset_y + 11, offset_x + 18, "READY");
    refresh();

    draw_queue(queue_win, queue, queue_pos);
    draw_hold(hold_win, hold);
    draw_keys(key_win, inputs);

    usleep(500000);
    mvprintw(offset_y + 11, offset_x + 18, " GO! ");
    refresh();
    usleep(500000);

    queue_pos = queue_pop(curr, queue, 0);

    // Game Loop
    while (1) {
        for (int i = 0; i < 10; i++)
            last_inputs[i] = inputs[i];
        get_inputs(fd, inputs);

        if (inputs[8] || inputs[9])
            break;
        if (inputs[3] && !last_inputs[3]) {
            move_piece(board, curr, 0, BOARD_HEIGHT - curr->y);
            lock_piece(board, curr);
            queue_pos = queue_pop(curr, queue, queue_pos);
            hold_used = 0;
            grav_c = 0;
        }

        if (inputs[0])
            ldas_c++;
        else
            ldas_c = 0;
        if (inputs[1])
            rdas_c++;
        else
            rdas_c = 0;

        if (ldas_c > DAS && rdas_c == 0)
            move_piece(board, curr, 1, -BOARD_WIDTH);
        if (rdas_c > DAS && ldas_c == 0)
            move_piece(board, curr, 1, BOARD_WIDTH);

        if (rdas_c > ldas_c && ldas_c > DAS)
            move_piece(board, curr, 1, -BOARD_WIDTH);

        if (ldas_c > rdas_c && rdas_c > DAS)
            move_piece(board, curr, 1, BOARD_WIDTH);

        if (inputs[0] && !last_inputs[0])
            move_piece(board, curr, 1, -1);
        if (inputs[1] && !last_inputs[1])
            move_piece(board, curr, 1, 1);

        if (inputs[2])
            move_piece(board, curr, 0, BOARD_HEIGHT - curr->y);
        if (inputs[4] && !last_inputs[4])
            spin_piece(board, curr, 2);
        if (inputs[5] && !last_inputs[5])
            spin_piece(board, curr, 0);
        if (inputs[6] && !last_inputs[6])
            spin_piece(board, curr, 1);
        if (inputs[7] && !last_inputs[7]) {
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

    free_nodes(board);
    free(curr);
    delwin(board_win);
    delwin(queue_win);
    delwin(hold_win);
    delwin(key_win);
    clear();

    return inputs[9];
}

int main() {
    srandom(time(NULL));
    setlocale(LC_ALL, "");

    struct termios old;
    struct termios new;

    int fd = -1;
    for (int i = 0; conspath[i]; i++) {
        fd = open(conspath[i], O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (is_a_console(fd))
            break;
        close(fd);
    }

    if (fd < 0)
        return 1;

	if (tcgetattr(fd, &old) == -1 || tcgetattr(fd, &new) == -1) {
        printf("tcgetattr error\n");
        return 1;
    }

	new.c_lflag &= ~((tcflag_t)(ICANON | ECHO | ISIG));
	new.c_iflag     = 0;
	new.c_cc[VMIN]  = 0;
	new.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &new) == -1) {
        printf("tcsetattr error\n");
        return 1;
    }

	if (ioctl(fd, KDSKBMODE, K_RAW)) {
        printf("ioctl KDSKBMODE error\n");
        return 1;
	}

    initscr();
    raw();
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
    init_pair(11, COLOR_WHITE,   -1);
    init_pair(12, COLOR_BLUE,   -1);

    while (!game(fd));

    endwin();

    if (ioctl(fd, KDSKBMODE, K_UNICODE)) {
        printf("ioctl KDSKBMODE error\n");
        return 1;
    }
    if (tcsetattr(fd, 0, &old) == -1) {
        printf("tcsetattr error\n");
        return 1;
    }
    close(fd);

    return 0;
}
