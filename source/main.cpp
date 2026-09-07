#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// Draw a filled rectangle onto the YUYV video frame buffer
void drawRect(int x, int y, int width, int height, u32 color) {
    if (!xfb || !rmode) return;
    
    u32 *fb = (u32 *)xfb;
    int screenWidth = rmode->fbWidth / 2; // 2 pixels packed per 32-bit word
    
    for (int r = y; r < y + height; r++) {
        if (r < 0 || r >= rmode->efbHeight) continue;
        for (int c = x / 2; c < (x + width) / 2; c++) {
            if (c < 0 || c >= screenWidth) continue;
            fb[r * screenWidth + c] = color;
        }
    }
}

// Clear the video frame buffer to solid black
void clearScreen() {
    if (!xfb || !rmode) return;
    u32 *fb = (u32 *)xfb;
    int totalWords = (rmode->fbWidth * rmode->efbHeight) / 2;
    for (int i = 0; i < totalWords; i++) {
        fb[i] = 0x00800080; // Black in YUYV (Y=0, U=128, V=128)
    }
}

int main(int argc, char **argv) {
    VIDEO_Init();
    WPAD_Init();
    PAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    int playerX = 300;
    int playerY = 200;
    int playerSize = 40;
    int moveSpeed = 5;

    // Bright yellow player color in YUYV format
    u32 playerColor = 0xD010D010; 

    while (1) {
        WPAD_ScanPads();
        PAD_ScanPads();

        u32 wiiHeld = WPAD_ButtonsHeld(0);
        u32 gcHeld = PAD_ButtonsHeld(0);

        // Press HOME (Wiimote) or START (GameCube) to exit
        if ((WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) || (PAD_ButtonsDown(0) & PAD_BUTTON_START)) {
            break;
        }

        // D-Pad movement
        if ((gcHeld & PAD_BUTTON_UP) || (wiiHeld & WPAD_BUTTON_UP)) playerY -= moveSpeed;
        if ((gcHeld & PAD_BUTTON_DOWN) || (wiiHeld & WPAD_BUTTON_DOWN)) playerY += moveSpeed;
        if ((gcHeld & PAD_BUTTON_LEFT) || (wiiHeld & WPAD_BUTTON_LEFT)) playerX -= moveSpeed;
        if ((gcHeld & PAD_BUTTON_RIGHT) || (wiiHeld & WPAD_BUTTON_RIGHT)) playerX += moveSpeed;

        // Keep player on-screen
        if (playerX < 0) playerX = 0;
        if (playerY < 0) playerY = 0;
        if (playerX > rmode->fbWidth - playerSize) playerX = rmode->fbWidth - playerSize;
        if (playerY > rmode->efbHeight - playerSize) playerY = rmode->efbHeight - playerSize;

        clearScreen();
        drawRect(playerX, playerY, playerSize, playerSize, playerColor);

        VIDEO_SetNextFramebuffer(xfb);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    return 0;
}
