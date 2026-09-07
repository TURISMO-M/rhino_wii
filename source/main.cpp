#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h> // This is the missing header for Wiimote controls

// Extern declaration for the raw image data created by raw2c
extern "C" {
    extern const u8 player[];
    extern const u32 player_size;
}

static void *frameBuffer = NULL;
static GXRModeObj *rmode = NULL;

// Helper function to draw raw RGBA/RGB image data directly to the framebuffer
void DrawImage(int posX, int posY, int width, int height, const u8* imgData) {
    if (!frameBuffer || !rmode) return;

    u32 *fb = (u32*)frameBuffer;
    int index = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int currentX = posX + x;
            int currentY = posY + y;

            if (currentX >= 0 && currentX < rmode->fbWidth && currentY >= 0 && currentY < rmode->xfbHeight) {
                // Get color bytes (Assuming 4 bytes per pixel: R, G, B, A)
                u8 r = imgData[index];
                u8 g = imgData[index + 1];
                u8 b = imgData[index + 2];

                // Convert RGB to YCbCr / YUY2 format for Wii Framebuffer
                u8 y_val = (u8)(0.299 * r + 0.587 * g + 0.114 * b);
                u8 u_val = 128;
                u8 v_val = 128;

                // Pack into 32-bit pixel data (Y1 U Y2 V)
                u32 color = (y_val << 24) | (u_val << 16) | (y_val << 8) | v_val;
                fb[currentY * (rmode->fbWidth / 2) + (currentX / 2)] = color;
            }
            index += 4; // Advance 4 bytes per pixel
        }
    }
}

int main(int argc, char **argv) {
    VIDEO_Init();
    PAD_Init();
    WPAD_Init();
    
    rmode = VIDEO_GetPreferredMode(NULL);
    frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    
    CON_Init(frameBuffer, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Initial position for your player sprite
    int playerX = 200;
    int playerY = 150;
    int moveSpeed = 4;

    while(1) {
        PAD_ScanPads();
        WPAD_ScanPads();

        u32 gcPressed = PAD_ButtonsDown(0);
        u32 gcHeld = PAD_ButtonsHeld(0);
        u32 wiiHeld = WPAD_ButtonsHeld(0);

        // Exit application with Start or Home button
        if ((gcPressed & PAD_BUTTON_START) || (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)) {
            exit(0);
        }

        // Movement with GameCube D-Pad or Wiimote D-Pad
        if ((gcHeld & PAD_BUTTON_UP) || (wiiHeld & WPAD_BUTTON_UP)) playerY -= moveSpeed;
        if ((gcHeld & PAD_BUTTON_DOWN) || (wiiHeld & WPAD_BUTTON_DOWN)) playerY += moveSpeed;
        if ((gcHeld & PAD_BUTTON_LEFT) || (wiiHeld & WPAD_BUTTON_LEFT)) playerX -= moveSpeed;
        if ((gcHeld & PAD_BUTTON_RIGHT) || (wiiHeld & WPAD_BUTTON_RIGHT)) playerX += moveSpeed;

        // Clear screen / refresh text
        printf("\x1b[2;2H");
        printf("Hello Wii! Move player with D-Pad.");

        // Draw player image at current coordinates (assuming 250x250 size)
        DrawImage(playerX, playerY, 250, 250, player);

        VIDEO_WaitVSync();
    }
    return 0;
}
