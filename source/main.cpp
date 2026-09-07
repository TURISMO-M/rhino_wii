#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Screen and Grid Dimensions
#define TILE_SIZE 20
#define GRID_WIDTH 32  // 640 / 20
#define GRID_HEIGHT 24 // 480 / 20
#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)

// YUYV Colors
#define COLOR_BLACK 0x00800080
#define COLOR_GREEN 0xD000D000
#define COLOR_RED   0x50505050
#define COLOR_WHITE 0xFF80FF80

struct Point {
    int x;
    int y;
};

struct Point snake[MAX_SNAKE_LENGTH];
int snakeLength = 1;
struct Point food;
int dx = 1, dy = 0;
int score = 0;
int highScore = 0;
bool isPaused = false;
bool gameOver = false;

// Draw a filled rectangle onto the frame buffer
void drawRect(int x, int y, int width, int height, u32 color) {
    if (!xfb || !rmode) return;
    u32 *fb = (u32 *)xfb;
    int screenWidth = rmode->fbWidth / 2;

    for (int r = y; r < y + height; r++) {
        if (r < 0 || r >= rmode->efbHeight) continue;
        for (int c = x / 2; c < (x + width) / 2; c++) {
            if (c < 0 || c >= screenWidth) continue;
            fb[r * screenWidth + c] = color;
        }
    }
}

void clearScreen(u32 color) {
    if (!xfb || !rmode) return;
    u32 *fb = (u32 *)xfb;
    int totalWords = (rmode->fbWidth * rmode->efbHeight) / 2;
    for (int i = 0; i < totalWords; i++) {
        fb[i] = color;
    }
}

void spawnFood() {
    food.x = rand() % GRID_WIDTH;
    food.y = rand() % GRID_HEIGHT;
}

void resetGame() {
    snakeLength = 1;
    snake[0].x = 10;
    snake[0].y = 10;
    dx = 1;
    dy = 0;
    score = 0;
    gameOver = false;
    isPaused = false;
    spawnFood();
}

void updateSnake() {
    if (isPaused || gameOver) return;

    // Calculate next head position
    int nextX = snake[0].x + dx;
    int nextY = snake[0].y + dy;

    // Screen wrapping (matching HTML game behavior)
    if (nextX < 0) nextX = GRID_WIDTH - 1;
    if (nextX >= GRID_WIDTH) nextX = 0;
    if (nextY < 0) nextY = GRID_HEIGHT - 1;
    if (nextY >= GRID_HEIGHT) nextY = 0;

    // Check self-collision
    for (int i = 1; i < snakeLength; i++) {
        if (snake[i].x == nextX && snake[i].y == nextY) {
            gameOver = true;
            if (score > highScore) {
                highScore = score;
            }
            return;
        }
    }

    // Move body segments
    for (int i = snakeLength - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0].x = nextX;
    snake[0].y = nextY;

    // Check food collision
    if (snake[0].x == food.x && snake[0].y == food.y) {
        score += 10;
        if (snakeLength < MAX_SNAKE_LENGTH) {
            snakeLength++;
        }
        spawnFood();
    }
}

int main(int argc, char **argv) {
    VIDEO_Init();
    WPAD_Init();
    PAD_Init();
    srand(time(NULL));

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    resetGame();

    int frameCounter = 0;

    while (1) {
        WPAD_ScanPads();
        PAD_ScanPads();

        u32 wiiDown = WPAD_ButtonsDown(0);
        u32 gcDown = PAD_ButtonsDown(0);

        // Exit game
        if ((wiiDown & WPAD_BUTTON_HOME) || (gcDown & PAD_BUTTON_START)) {
            break;
        }

        // Toggle Pause
        if ((wiiDown & WPAD_BUTTON_PLUS) || (gcDown & PAD_BUTTON_START)) {
            isPaused = !isPaused;
        }

        // Restart on Game Over
        if (gameOver && ((wiiDown & WPAD_BUTTON_A) || (gcDown & PAD_BUTTON_A))) {
            resetGame();
        }

        // Direction Controls
        if (!isPaused && !gameOver) {
            if (((wiiDown & WPAD_BUTTON_UP) || (gcDown & PAD_BUTTON_UP)) && dy != 1) {
                dx = 0; dy = -1;
            }
            if (((wiiDown & WPAD_BUTTON_DOWN) || (gcDown & PAD_BUTTON_DOWN)) && dy != -1) {
                dx = 0; dy = 1;
            }
            if (((wiiDown & WPAD_BUTTON_LEFT) || (gcDown & PAD_BUTTON_LEFT)) && dx != 1) {
                dx = -1; dy = 0;
            }
            if (((wiiDown & WPAD_BUTTON_RIGHT) || (gcDown & PAD_BUTTON_RIGHT)) && dx != -1) {
                dx = 1; dy = 0;
            }
        }

        // Game speed tick (runs every ~8 frames to match HTML timing)
        frameCounter++;
        if (frameCounter >= 8) {
            updateSnake();
            frameCounter = 0;
        }

        // Rendering
        clearScreen(COLOR_BLACK);

        // Draw Food
        drawRect(food.x * TILE_SIZE, food.y * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1, COLOR_RED);

        // Draw Snake
        for (int i = 0; i < snakeLength; i++) {
            drawRect(snake[i].x * TILE_SIZE, snake[i].y * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1, COLOR_GREEN);
        }

        // Draw Pause/Game Over visual indicator bar across top
        if (gameOver) {
            drawRect(0, 0, rmode->fbWidth, 10, COLOR_RED);
        } else if (isPaused) {
            drawRect(0, 0, rmode->fbWidth, 10, COLOR_WHITE);
        }

        VIDEO_SetNextFramebuffer(xfb);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    return 0;
}
