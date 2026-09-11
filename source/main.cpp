#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#define FIFO_SIZE (256 * 1024)

static void *frameBuffer[2] = { NULL, NULL };
static GXRModeObj *rmode = NULL;
static void *gp_fifo = NULL;

// ... [Keep your car variables and resetCar() function here] ...

int main(int argc, char **argv) {
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    
    // Allocate framebuffers cleanly in cached/uncached memory
    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Allocate 32-byte aligned FIFO buffer for GX
    gp_fifo = memalign(32, FIFO_SIZE);
    if (!gp_fifo) return 0; // Guard against failed allocation
    memset(gp_fifo, 0, FIFO_SIZE);

    GX_Init(gp_fifo, FIFO_SIZE);

    GXColor background = { 135, 206, 235, 255 }; // Sky Blue
    GX_SetCopyClear(background, 0x00ffffff);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);

    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);

    GX_SetNumChans(1);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);

    // Setup Vertex Format
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);

    // Setup Perspective Matrix
    MtxP perspective;
    f32 w = rmode->fbWidth;
    f32 h = rmode->efbHeight;
    guPerspective(perspective, 60.0f, (f32)w / (f32)h, 0.1f, 300.0f);
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

    int fbIndex = 0;
    resetCar();

    while (1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        u32 held = WPAD_ButtonsHeld(0);

        if (pressed & WPAD_BUTTON_HOME) break;

        // ... [Drive logic] ...

        // Camera Setup
        Mtx view;
        guVector camPos = {
            carX - sinf(carAngle * (M_PI / 180.0f)) * 12.0f,
            5.0f,
            carZ - cosf(carAngle * (M_PI / 180.0f)) * 12.0f
        };
        guVector camTarget = { carX, 1.0f, carZ };
        guVector camUp = { 0.0f, 1.0f, 0.0f };
        guLookAt(view, &camPos, &camUp, &camTarget);
        
        // Critical: Set view matrix before drawing primitives
        GX_LoadPosMtxImm(view, GX_PNMTX0);

        // Render Scene
        drawTrackPlane();
        drawCube(carX, 1.0f, carZ, 1.2f, 0.6f, 2.0f, 220, 30, 30);

        // Copy EFB to XFB and swap
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_CopyDisp(frameBuffer[fbIndex], GX_TRUE);
        GX_DrawDone();

        VIDEO_SetNextFramebuffer(frameBuffer[fbIndex]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
        fbIndex ^= 1;
    }

    return 0;
}
