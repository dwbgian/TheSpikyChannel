#include <gccore.h>
#include <fat.h>
#include <ogc/usbstorage.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "ui_background.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void init_video(void)
{
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if (rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }
}

static bool detect_usb(void)
{
    bool connected = false;

    if (fatMountSimple("usb", &__io_usbstorage)) {
        FILE *probe = fopen("usb:/", "r");
        connected = true;

        if (probe != NULL) {
            fclose(probe);
        }

        fatUnmount("usb:/");
    }

    return connected;
}

static void draw_screen(bool usb_connected)
{
    const u32 *src = usb_connected ? spiky_ui_connected : spiky_ui_disconnected;
    u32 *dst = (u32 *)xfb;
    u32 dst_words_per_row = rmode->fbWidth / 2;
    u32 copy_words_per_row = SPIKY_UI_WIDTH / 2;
    u32 rows = rmode->xfbHeight < SPIKY_UI_HEIGHT ? rmode->xfbHeight : SPIKY_UI_HEIGHT;

    for (u32 y = 0; y < rows; y++) {
        memcpy(dst + y * dst_words_per_row,
               src + y * copy_words_per_row,
               copy_words_per_row * sizeof(u32));
    }

    DCFlushRange(xfb, rmode->fbWidth * rmode->xfbHeight * VI_DISPLAY_PIX_SZ);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    init_video();

    bool usb_connected = detect_usb();

    while (true) {
        draw_screen(usb_connected);

        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            break;
        }

        if (pressed & WPAD_BUTTON_A) {
            usb_connected = detect_usb();
        }

        VIDEO_WaitVSync();
    }

    WPAD_Shutdown();
    return 0;
}
