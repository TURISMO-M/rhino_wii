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

// Player Car Transform Data
float carX = 0.0f;
float carZ = 0.0f;
float carAngle = 0.0f;
float carSpeed = 0.0f;

// Game State & Menu
bool isPaused = false;
int pauseSelection = 0;

void resetCar() {
    carX = 0.0f;
    carZ = 0.0f;
    carAngle = 0.0f;
    carSpeed = 0.0f;
    isPaused = false;
    pauseSelection = 0;
}

// Draw a colored 3D Cube
void drawCube(float x, float y, float z, float sx, float sy, float sz, u8 r, u8 g, u8 b) {
    Mtx model, rot, trans;
    guMtxIdentity(model);

    guVector axis = { 0.0f, 1.0f, 0.0f };
    guMtxRotAxisDeg(rot, &axis, carAngle);

    guMtxTrans(trans, x, y, z);
    guMtxConcat(trans, rot, model);

    GX_LoadPosMtxImm(model, GX_PNMTX0);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);

    // Front Face
    GX_Position3f32(-sx, -sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx, -sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx,  sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx,  sy,  sz); GX_Color3u8(r, g, b);

    // Back Face
    GX_Position3f32(-sx, -sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx,  sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx,  sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx, -sy, -sz); GX_Color3u8(r, g, b);

    // Top Face
    GX_Position3f32(-sx,  sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx,  sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx,  sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx,  sy, -sz); GX_Color3u8(r, g, b);

    // Bottom Face
    GX_Position3f32(-sx, -sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx, -sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx, -sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx, -sy,  sz); GX_Color3u8(r, g, b);

    // Right Face
    GX_Position3f32( sx, -sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx,  sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx,  sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32( sx, -sy,  sz); GX_Color3u8(r, g, b);

    // Left Face
    GX_Position3f32(-sx, -sy, -sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx, -sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx,  sy,  sz); GX_Color3u8(r, g, b);
    GX_Position3f32(-sx,  sy, -sz); GX_Color3u8(r, g, b);

    GX_End();
}

// Draw Ground Plane
void drawTrackPlane() {
    Mtx model;
    guMtxIdentity(model);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(-200.0f, -0.1f, -200.0f); GX_Color3u8(40, 140, 40);
        GX_Position3f32( 200.0f, -0.1f, -200.0f); GX_Color3u8(40, 140, 40);
        GX_Position3f32( 200.0f, -0.1f,  200.0f); GX_Color3u8(40, 140, 40);
        GX_Position3f32(-200.0f, -0.1f,  200.0f); GX_Color3u8(40, 140, 40);
    GX_End();
}

int main(int argc, char **argv) {
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Allocate 32-byte aligned FIFO buffer for GX GPU
    gp_fifo = memalign(32, FIFO_SIZE);
    if (!gp_fifo) return 0;
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

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);

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

        if (pressed & WPAD_BUTTON_PLUS) {
            isPaused = !isPaused;
            pauseSelection = 0;
        }

        if (isPaused) {
            if (pressed & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN)) {
                pauseSelection = !pauseSelection;
            }
            if (pressed & WPAD_BUTTON_A) {
                if (pauseSelection == 0) {
                    isPaused = false;
                } else {
                    resetCar();
                }
            }
        } else {
            if (held & WPAD_BUTTON_2) {
                carSpeed += 0.02f;
                if (carSpeed > 1.2f) carSpeed = 1.2f;
            } else if (held & WPAD_BUTTON_1) {
                carSpeed -= 0.015f;
                if (carSpeed < -0.4f) carSpeed = -0.4f;
            } else {
                carSpeed *= 0.95f;
            }

            if (held & WPAD_BUTTON_LEFT) {
                carAngle += 2.5f;
            }
            if (held & WPAD_BUTTON_RIGHT) {
                carAngle -= 2.5f;
            }

            float rad = carAngle * (M_PI / 180.0f);
            carX += sinf(rad) * carSpeed;
            carZ += cosf(rad) * carSpeed;
        }

        Mtx view;
        guVector camPos = {
            carX - sinf(carAngle * (M_PI / 180.0f)) * 12.0f,
            5.0f,
            carZ - cosf(carAngle * (M_PI / 180.0f)) * 12.0f
        };
        guVector camTarget = { carX, 1.0f, carZ };
        guVector camUp = { 0.0f, 1.0f, 0.0f };
        guLookAt(view, &camPos, &camUp, &camTarget);
        
        GX_LoadPosMtxImm(view, GX_PNMTX0);

        // Draw 3D elements
        drawTrackPlane();
        drawCube(carX, 1.0f, carZ, 1.2f, 0.6f, 2.0f, 220, 30, 30);

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
