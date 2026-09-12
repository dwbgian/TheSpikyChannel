#include <gccore.h>
#include <fat.h>
#include <ogc/usbstorage.h>
#include <stdbool.h>
#include <stdio.h>
#include <wiiuse/wpad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void init_video(void)
{
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);

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

static void draw_screen(bool usb_connected, const char *status)
{
    printf("\x1b[2J");
    printf("\x1b[6;8HThe Spiky Channel");
    printf("\x1b[8;8HDevelopment Build");
    printf("\x1b[11;8HUSB: %s", usb_connected ? "Connected" : "Not Connected");
    printf("\x1b[15;8HA = Continue");
    printf("\x1b[16;8HHOME = Exit");

    if (status != NULL && status[0] != '\0') {
        printf("\x1b[19;8H%s", status);
    }

    VIDEO_WaitVSync();
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    init_video();

    bool usb_connected = detect_usb();
    const char *status = "";

    while (true) {
        draw_screen(usb_connected, status);

        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            break;
        }

        if (pressed & WPAD_BUTTON_A) {
            usb_connected = detect_usb();
            status = "USB status refreshed.";
        }

        VIDEO_WaitVSync();
    }

    WPAD_Shutdown();
    return 0;
}
