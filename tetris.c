#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 4
#define FALL_INTERVAL 2.0

typedef struct {
    int shape[BLOCK_SIZE][BLOCK_SIZE];
    char displayChar;
} Tetrimino;

Tetrimino tetriminos[7] = {
    {{{1, 1, 1, 0},
      {0, 1, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}, // T

    {{{1, 1, 1, 1},
      {0, 0, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}, // I

    {{{1, 1, 0, 0},
      {1, 1, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}, // O

    {{{1, 1, 0, 0},
      {0, 1, 1, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}, // S

    {{{0, 1, 1, 0},
      {1, 1, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}, // Z

    {{{1, 1, 1, 0},
      {1, 0, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}, // J

    {{{1, 1, 1, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0}}, '@'}  // L
};

int board[BOARD_HEIGHT][BOARD_WIDTH];

Tetrimino *currentBlock;
Tetrimino *nextBlock = NULL;

int blockX, blockY;
int score = 0;
clock_t lastFallTime;

struct termios oldt, newt;

void initializeBoard() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = 0;
        }
    }
    srand(time(NULL));
    currentBlock = &tetriminos[rand() % 7];
    blockX = BOARD_WIDTH / 2 - BLOCK_SIZE / 2;
    blockY = 0;
}

void initializeNextBlock() {
    nextBlock = &tetriminos[rand() % 7];
}

void selectNextBlock() {
    nextBlock = &tetriminos[rand() % 7];
}

void printNextBlockLine(int y) {
    if (y == 0) {
        printf("Next : ");
    } else {
        printf("       ");
    }

    if (y > 0) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            if (nextBlock != NULL && nextBlock->shape[y - 1][x] == 1) {
                printf("%c", nextBlock->displayChar);
            } else {
                printf(" ");
            }
        }
    }
    printf(" ");
}




void printBoard() {
//    system("clear");
    printf("\033[H\033[J");
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                if (y >= blockY && y < blockY + BLOCK_SIZE && x >= blockX && x < blockX + BLOCK_SIZE && currentBlock->shape[y - blockY][x - blockX] == 1) {
                    printf("@ ");
                } else {
                    printf(". ");
                }
            } else {
                printf("# ");
            }
        }
        
        if (y < BLOCK_SIZE) {
            printNextBlockLine(y);
        } else if (y == BLOCK_SIZE) {
            printf("       Score: %d", score);
        }
        printf("\n");
    }
}

void rotateBlock() {
    Tetrimino rotatedBlock;
    for (int y = 0; y < BLOCK_SIZE; y++) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            rotatedBlock.shape[y][x] = currentBlock->shape[BLOCK_SIZE - x - 1][y];
        }
    }
    for (int y = 0; y < BLOCK_SIZE; y++) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            currentBlock->shape[y][x] = rotatedBlock.shape[y][x];
        }
    }
}

int canMove(int dx, int dy) {
    for (int y = 0; y < BLOCK_SIZE; y++) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            if (currentBlock->shape[y][x] && (board[blockY + y + dy][blockX + x + dx] || blockX + x + dx < 0 || blockX + x + dx >= BOARD_WIDTH || blockY + y + dy >= BOARD_HEIGHT)) {
                return 0;
            }
        }
    }
    return 1;
}

void moveBlock(int dx, int dy) {
    if (canMove(dx, dy)) {
        blockX += dx;
        blockY += dy;
    }
}

void placeBlock() {
    for (int y = 0; y < BLOCK_SIZE; y++) {
        for (int x = 0; x < BLOCK_SIZE; x++) {
            if (currentBlock->shape[y][x]) {
                board[blockY + y][blockX + x] = 1;
            }
        }
    }
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        int isFull = 1;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!board[y][x]) {
                isFull = 0;
                break;
            }
        }
        if (isFull) {
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    board[yy][x] = board[yy - 1][x];
                }
            }
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[0][x] = 0;
            }
            y++;
        }
    }
    currentBlock = nextBlock;
    blockX = BOARD_WIDTH / 2 - BLOCK_SIZE / 2;
    blockY = 0;
    if (!canMove(0, 0)) {
        printf("Game Over\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(0);
    }
    selectNextBlock();
}


int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    // 端末設定を取得
    tcgetattr(STDIN_FILENO, &oldt);

    // 新しい設定をコピー
    newt = oldt;

    // ノンカノニカルモード、エコー無し
    newt.c_lflag &= ~(ICANON | ECHO);

    // 設定を適用
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

    // ノンブロッキングモードを有効
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    // 端末設定を元に戻す
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int getch(void) {
    int c;
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    c = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return c;
}



int main() {

    lastFallTime = clock();

    initializeBoard();
    initializeNextBlock();

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (1) {
        printBoard();
        if (canMove(0, 1)) {
            moveBlock(0, 1);
        } else {
            placeBlock();
        }

        clock_t currentTime = clock();

        double elapsedTime = (double)(currentTime - lastFallTime) / CLOCKS_PER_SEC;
        printf("%f",elapsedTime);
        if (elapsedTime >= FALL_INTERVAL) {
            if (canMove(0, 1)) {
                blockY++;
            } else {
                placeBlock();
                selectNextBlock();
            }
            lastFallTime = currentTime;
        }

        if (kbhit()) {
            char c = getch();
            switch (c) {
                        case 'a':
                            moveBlock(-1, 0);
                            break;
                        case 'd':
                            moveBlock(1, 0);
                            break;
                        case 's':
                            if (canMove(0, 1)) {
                                moveBlock(0, 1);
                            }
                            break;
                        case 'w':
                            rotateBlock();
                            if (!canMove(0, 0)) {
                                rotateBlock();
                                rotateBlock();
                                rotateBlock();
                            }
                            break;
                        case 'q':
                            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                            return 0;
                    }
        }

    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}
