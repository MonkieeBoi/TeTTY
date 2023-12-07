#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BOARD_HEIGHT 20
#define BOARD_WIDTH 10

struct node {
    unsigned char row[BOARD_WIDTH];
    struct node *next;
};

int count_nodes(struct node *n) {
    int length = 0;
    while (n != NULL) {
        length++;
        n = n->next;
    }
    return length;
}

void draw_board(struct node *n) {
    int height = count_nodes(n);
    printf("   ┏━━━━━━━━━━━━━━━━━━━━┓\n");
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        printf("%02d ┃", i);
        if (BOARD_HEIGHT - height <= i && n != NULL) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                if (n->row[j])
                    printf("[]");
                else
                    printf("  ");
            }
            printf("┃\n");
            n = n->next;
        } else {
            printf("                    ┃\n");
        }
    }
    printf("   ┗━━━━━━━━━━━━━━━━━━━━┛\n");
}

int main() {
    struct node *board;
    board = malloc(sizeof(struct node));
    board->row[5] = 1;
    board->next = NULL;

    // Game Loop

    while (1) {
        printf("\e[1;1H\e[2J");
        draw_board(board);
        usleep(500 * 1000);
    }

    return 0;
}
