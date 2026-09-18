#include <gccore.h>
#include <fat.h>
#include <ogc/usbstorage.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <wiiuse/wpad.h>

#define CORE_PATH "usb:/spiky/core/boot.dol"
#define DOL_MAX_SECTIONS 18

typedef void (*entrypoint)(void);

typedef struct dol_header {
    u32 text_pos[7];
    u32 data_pos[11];
    u32 text_start[7];
    u32 data_start[11];
    u32 text_size[7];
    u32 data_size[11];
    u32 bss_start;
    u32 bss_size;
    u32 entry_point;
} dol_header;

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void wait_for_exit(void)
{
    while (true) {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) {
            break;
        }

        VIDEO_WaitVSync();
    }
}

static void init_console(void)
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

static void show_error(const char *message)
{
    printf("\x1b[2J");
    printf("\x1b[5;5HThe Spiky Channel Forwarder");
    printf("\x1b[8;5H%s", message);
    printf("\x1b[11;5HExpected:");
    printf("\x1b[12;5H%s", CORE_PATH);
    printf("\x1b[17;5HHOME = Exit");
    wait_for_exit();
}

static bool read_file(const char *path, u8 **buffer, size_t *size)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    long length = ftell(file);

    if (length <= 0) {
        fclose(file);
        return false;
    }

    rewind(file);

    u8 *data = memalign(32, (size_t)length);

    if (data == NULL) {
        fclose(file);
        return false;
    }

    size_t read = fread(data, 1, (size_t)length, file);
    fclose(file);

    if (read != (size_t)length) {
        free(data);
        return false;
    }

    *buffer = data;
    *size = (size_t)length;
    return true;
}

static bool load_dol_image(const u8 *dol, size_t dol_size, entrypoint *entry)
{
    if (dol_size < sizeof(dol_header)) {
        return false;
    }

    const dol_header *header = (const dol_header *)dol;
    const u32 *section_pos[DOL_MAX_SECTIONS];
    const u32 *section_start[DOL_MAX_SECTIONS];
    const u32 *section_size[DOL_MAX_SECTIONS];

    for (u32 i = 0; i < 7; i++) {
        section_pos[i] = &header->text_pos[i];
        section_start[i] = &header->text_start[i];
        section_size[i] = &header->text_size[i];
    }

    for (u32 i = 0; i < 11; i++) {
        section_pos[i + 7] = &header->data_pos[i];
        section_start[i + 7] = &header->data_start[i];
        section_size[i + 7] = &header->data_size[i];
    }

    for (u32 i = 0; i < DOL_MAX_SECTIONS; i++) {
        u32 pos = *section_pos[i];
        u32 addr = *section_start[i];
        u32 size = *section_size[i];

        if (size == 0) {
            continue;
        }

        if ((size_t)pos + (size_t)size > dol_size) {
            return false;
        }

        memmove((void *)addr, dol + pos, size);
        DCFlushRange((void *)addr, size);
        ICInvalidateRange((void *)addr, size);
    }

    if (header->bss_size > 0) {
        memset((void *)header->bss_start, 0, header->bss_size);
        DCFlushRange((void *)header->bss_start, header->bss_size);
    }

    *entry = (entrypoint)header->entry_point;
    return header->entry_point != 0;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    init_console();

    printf("\x1b[2J");
    printf("\x1b[5;5HThe Spiky Channel Forwarder");
    printf("\x1b[8;5HLoading USB core...");

    if (!fatMountSimple("usb", &__io_usbstorage)) {
        show_error("USB device not found.");
        WPAD_Shutdown();
        return 0;
    }

    u8 *dol = NULL;
    size_t dol_size = 0;

    if (!read_file(CORE_PATH, &dol, &dol_size)) {
        fatUnmount("usb:/");
        show_error("Spiky core not found.");
        WPAD_Shutdown();
        return 0;
    }

    entrypoint entry = NULL;

    if (!load_dol_image(dol, dol_size, &entry)) {
        free(dol);
        fatUnmount("usb:/");
        show_error("Spiky core is not a valid DOL.");
        WPAD_Shutdown();
        return 0;
    }

    free(dol);
    fatUnmount("usb:/");

    WPAD_Shutdown();
    VIDEO_SetBlack(TRUE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    entry();
    return 0;
}
