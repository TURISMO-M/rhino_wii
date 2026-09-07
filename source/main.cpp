#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>

static void *frameBuffer = NULL;
static GXRModeObj *rmode = NULL;

int main(int argc, char **argv) {
    VIDEO_Init();
    PAD_Init();
    
    rmode = VIDEO_GetPreferredMode(NULL);
    frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    
    CON_Init(frameBuffer, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    printf("\x1b[20;0H");
    printf("Hello Creeper Wii!");

    while(1) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsDown(0);
        if (pressed & PAD_BUTTON_START) exit(0);
        VIDEO_WaitVSync();
    }
    return 0;
}
