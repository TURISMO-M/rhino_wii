#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

extern "C" {
    extern const u8 player[];
    extern const u32 player_size;
}

int main(int argc, char **argv) {
    GRRLIB_Init();
    WPAD_Init();
    PAD_Init();

    // Decode the compressed PNG into a usable texture
    GRRLIB_texImg *tex_player = GRRLIB_LoadTexture(player);

    int playerX = 200;
    int playerY = 150;
    int moveSpeed = 4;

    while(1) {
        WPAD_ScanPads();
        PAD_ScanPads();

        u32 wiiHeld = WPAD_ButtonsHeld(0);
        u32 gcHeld = PAD_ButtonsHeld(0);

        // Exit on Home or Start
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME || PAD_ButtonsDown(0) & PAD_BUTTON_START) break;

        // Movement controls
        if ((gcHeld & PAD_BUTTON_UP) || (wiiHeld & WPAD_BUTTON_UP)) playerY -= moveSpeed;
        if ((gcHeld & PAD_BUTTON_DOWN) || (wiiHeld & WPAD_BUTTON_DOWN)) playerY += moveSpeed;
        if ((gcHeld & PAD_BUTTON_LEFT) || (wiiHeld & WPAD_BUTTON_LEFT)) playerX -= moveSpeed;
        if ((gcHeld & PAD_BUTTON_RIGHT) || (wiiHeld & WPAD_BUTTON_RIGHT)) playerX += moveSpeed;

        GRRLIB_FillScreen(0x000000FF); // Clear screen to black
        
        // Draw our decoded PNG texture (X, Y, texture, rotation, scaleX, scaleY, color)
        GRRLIB_DrawImg(playerX, playerY, tex_player, 0, 1, 1, 0xFFFFFFFF);
        
        GRRLIB_Render();
    }

    GRRLIB_FreeTexture(tex_player);
    GRRLIB_Exit();
    exit(0);
}
