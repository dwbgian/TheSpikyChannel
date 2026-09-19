#include "ui.h"

#include "ui_background.h"

#include <gccore.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct Rect {
    s32 x;
    s32 y;
    s32 w;
    s32 h;
} Rect;

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static const char *screen_labels[SCREEN_COUNT] = {
    "Home",
    "Apps",
    "Downloads",
    "Updates",
    "Account",
    "Pairing",
    "Settings",
    "Storage"
};

static const char *screen_subtitles[SCREEN_COUNT] = {
    "USB-only Spiky system core",
    "USB and SD homebrew scan",
    "Coming soon: public repo",
    "Coming soon: safe USB updates",
    "Coming soon: optional Spiky account",
    "Coming soon: device pairing",
    "Local core preferences",
    "Storage and diagnostics"
};

static const Rect home_tiles[SCREEN_COUNT] = {
    { 28, 152, 138, 78 },
    { 178, 152, 138, 78 },
    { 328, 152, 138, 78 },
    { 478, 152, 138, 78 },
    { 28, 244, 138, 78 },
    { 178, 244, 138, 78 },
    { 328, 244, 138, 78 },
    { 478, 244, 138, 78 }
};

static u32 rgb_to_yuyv(u8 r, u8 g, u8 b)
{
    int y = (int)(0.299f * r + 0.587f * g + 0.114f * b);
    int cb = (int)(128.0f - 0.168736f * r - 0.331264f * g + 0.5f * b);
    int cr = (int)(128.0f + 0.5f * r - 0.418688f * g - 0.081312f * b);

    if (y < 0) y = 0;
    if (y > 255) y = 255;
    if (cb < 0) cb = 0;
    if (cb > 255) cb = 255;
    if (cr < 0) cr = 0;
    if (cr > 255) cr = 255;

    return ((u32)y << 24) | ((u32)cb << 16) | ((u32)y << 8) | (u32)cr;
}

static void draw_rect(s32 x, s32 y, s32 w, s32 h, u8 r, u8 g, u8 b)
{
    u32 color;
    u32 *dst;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 row;
    s32 col;
    s32 words_per_row;

    if (xfb == NULL || rmode == NULL) {
        return;
    }

    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + w > (s32)rmode->fbWidth ? (s32)rmode->fbWidth : x + w;
    y1 = y + h > (s32)rmode->xfbHeight ? (s32)rmode->xfbHeight : y + h;

    if (x1 <= x0 || y1 <= y0) {
        return;
    }

    x0 &= ~1;
    x1 &= ~1;
    color = rgb_to_yuyv(r, g, b);
    dst = (u32 *)xfb;
    words_per_row = rmode->fbWidth / 2;

    for (row = y0; row < y1; row++) {
        for (col = x0 / 2; col < x1 / 2; col++) {
            dst[row * words_per_row + col] = color;
        }
    }
}

static void draw_border(Rect rect, bool selected)
{
    u8 r = selected ? 238 : 178;
    u8 g = selected ? 0 : 184;
    u8 b = selected ? 18 : 192;
    s32 width = selected ? 4 : 2;
    s32 i;

    for (i = 0; i < width; i++) {
        draw_rect(rect.x + i, rect.y + i, rect.w - i * 2, 2, r, g, b);
        draw_rect(rect.x + i, rect.y + rect.h - 2 - i, rect.w - i * 2, 2, r, g, b);
        draw_rect(rect.x + i, rect.y + i, 2, rect.h - i * 2, r, g, b);
        draw_rect(rect.x + rect.w - 2 - i, rect.y + i, 2, rect.h - i * 2, r, g, b);
    }
}

static void draw_background(bool connected)
{
    const u32 *src = connected ? spiky_ui_connected : spiky_ui_disconnected;
    u32 *dst = (u32 *)xfb;
    u32 dst_words_per_row = rmode->fbWidth / 2;
    u32 copy_words_per_row = SPIKY_UI_WIDTH / 2;
    u32 rows = rmode->xfbHeight < SPIKY_UI_HEIGHT ? rmode->xfbHeight : SPIKY_UI_HEIGHT;
    u32 y;

    for (y = 0; y < rows; y++) {
        memcpy(dst + y * dst_words_per_row,
               src + y * copy_words_per_row,
               copy_words_per_row * sizeof(u32));
    }
}

static void print_at(int row, int col, const char *text)
{
    printf("\x1b[%d;%dH%s", row, col, text);
}

static void print_fixed(int row, int col, const char *text, int width)
{
    char buffer[96];

    snprintf(buffer, sizeof(buffer), "%-*.*s", width, width, text);
    print_at(row, col, buffer);
}

bool ui_init(void)
{
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
    if (rmode == NULL) {
        return false;
    }

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    if (xfb == NULL) {
        return false;
    }

    console_init(xfb, 20, 18, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if (rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }

    return true;
}

static void draw_header(const AppState *app, const StorageState *storage)
{
    char line[96];

    draw_rect(18, 22, 604, 104, 252, 246, 247);
    draw_border((Rect){ 18, 22, 604, 104 }, false);

    snprintf(line, sizeof(line), "The Spiky Channel  |  %s", screen_labels[app->screen]);
    print_fixed(2, 3, line, 70);
    print_fixed(4, 3, screen_subtitles[app->screen], 70);

    snprintf(line, sizeof(line), "USB:%s  SD:%s  Core:%s",
             storage->usb_mounted ? "Connected" : "No",
             storage->sd_mounted ? "Connected" : "No",
             storage->usb_core_found ? "USB OK" :
             (storage->sd_core_found ? "SD OK" : "Missing"));
    print_fixed(6, 3, line, 70);

    if (app->status_line[0] != '\0') {
        print_fixed(8, 3, app->status_line, 70);
    }
}

static void draw_home(const AppState *app, const InputState *input)
{
    u32 i;

    for (i = 0; i < SCREEN_COUNT; i++) {
        Rect rect = home_tiles[i];
        bool selected = i == app->home_selected;

        draw_rect(rect.x, rect.y, rect.w, rect.h,
                  selected ? 224 : 255,
                  selected ? 0 : 250,
                  selected ? 18 : 252);
        draw_border(rect, selected);
    }

    print_fixed(11, 6, "Home", 14);
    print_fixed(11, 25, "Apps", 14);
    print_fixed(11, 44, "Downloads", 14);
    print_fixed(11, 63, "Updates", 14);
    print_fixed(17, 6, "Account", 14);
    print_fixed(17, 25, "Pairing", 14);
    print_fixed(17, 44, "Settings", 14);
    print_fixed(17, 63, "Storage", 14);

    print_fixed(24, 3, "A Open   B Back   +/- Page   HOME Exit", 74);

    if (input != NULL && input->pointer_valid) {
        char pointer[64];
        snprintf(pointer, sizeof(pointer), "Pointer %ld,%ld",
                 (long)input->pointer_x, (long)input->pointer_y);
        print_fixed(26, 3, pointer, 30);
    }
}

static void draw_apps(const AppState *app, const HomebrewList *apps)
{
    u32 i;
    u32 visible = SPIKY_APP_PAGE_SIZE;
    char line[128];

    draw_rect(24, 144, 592, 252, 255, 250, 252);
    draw_border((Rect){ 24, 144, 592, 252 }, false);

    if (apps->count == 0) {
        print_fixed(12, 6, "No apps found on USB or SD.", 60);
        print_fixed(14, 6, "Scanned: /apps and /spiky/apps on both devices.", 60);
        print_fixed(16, 6, "This screen only lists apps for now. It does not launch them.", 68);
    } else {
        snprintf(line, sizeof(line), "%lu app(s) found%s",
                 (unsigned long)apps->count,
                 apps->truncated ? " - list truncated" : "");
        print_fixed(10, 5, line, 70);

        for (i = 0; i < visible && app->app_scroll + i < apps->count; i++) {
            const HomebrewApp *entry = &apps->items[app->app_scroll + i];
            const char *boot = entry->has_boot_dol ? "DOL" : "NO DOL";
            const char *meta = entry->has_meta_xml ? "META" : "NO META";
            const char *space = entry->spiky_space ? "spiky" : "hbc";
            bool selected = (app->app_scroll + i) == app->app_selected;

            draw_rect(38, 170 + (s32)i * 25, 560, 21,
                      selected ? 226 : 255,
                      selected ? 0 : 255,
                      selected ? 18 : 255);
            draw_border((Rect){ 38, 170 + (s32)i * 25, 560, 21 }, selected);

            snprintf(line, sizeof(line), "%c %-3s %-7s %-8s %-28s",
                     selected ? '>' : ' ',
                     entry->device, space, boot, entry->name);
            print_fixed(12 + (int)i * 2, 6, line, 68);
            snprintf(line, sizeof(line), "    %s | %s", entry->path, meta);
            print_fixed(13 + (int)i * 2, 6, line, 68);
        }
    }

    print_fixed(24, 3, "D-Pad Select   +/- Scroll   B Home   HOME Exit", 74);
}

static void draw_coming_soon(const AppState *app)
{
    char line[96];

    draw_rect(42, 150, 556, 206, 255, 250, 252);
    draw_border((Rect){ 42, 150, 556, 206 }, false);

    snprintf(line, sizeof(line), "%s is prepared, but offline in this build.",
             screen_labels[app->screen]);
    print_fixed(12, 8, line, 64);
    print_fixed(14, 8, "No downloads, accounts, pairing, or web calls run yet.", 64);
    print_fixed(16, 8, "The screen exists so the full Spiky system shape is visible.", 64);
    print_fixed(18, 8, "Next step later: manifest, checksums, and opt-in network code.", 64);
    print_fixed(24, 3, "B Home   HOME Exit", 74);
}

static void draw_settings(void)
{
    draw_rect(42, 150, 556, 206, 255, 250, 252);
    draw_border((Rect){ 42, 150, 556, 206 }, false);

    print_fixed(12, 8, "Settings", 64);
    print_fixed(14, 8, "Theme: Spiky Red", 64);
    print_fixed(16, 8, "Launch mode: USB-only core", 64);
    print_fixed(18, 8, "Network: disabled in this build", 64);
    print_fixed(20, 8, "NAND/WAD/IOS actions: not present", 64);
    print_fixed(24, 3, "B Home   HOME Exit", 74);
}

static void draw_storage(const StorageState *storage)
{
    draw_rect(42, 140, 556, 244, 255, 250, 252);
    draw_border((Rect){ 42, 140, 556, 244 }, false);

    print_fixed(11, 8, "Storage / Diagnostics", 64);
    print_fixed(13, 8, storage->usb_mounted ? "USB: mounted" : "USB: not mounted", 64);
    print_fixed(15, 8, storage->sd_mounted ? "SD: mounted" : "SD: not mounted", 64);
    print_fixed(17, 8, storage->usb_core_found ? "USB core: usb:/spiky/core/boot.dol found"
                                               : "USB core: missing", 64);
    print_fixed(19, 8, storage->usb_folders_ready ? "USB folders: ready"
                                                  : "USB folders: incomplete", 64);
    print_fixed(21, 8, "A Create missing USB /spiky folders", 64);
    print_fixed(24, 3, "A Create folders   B Home   HOME Exit", 74);
}

void ui_draw(const AppState *app, const StorageState *storage,
             const HomebrewList *apps, const InputState *input)
{
    bool connected = storage->usb_mounted || storage->sd_mounted;

    draw_background(connected);
    draw_header(app, storage);

    switch (app->screen) {
        case SCREEN_HOME:
            draw_home(app, input);
            break;
        case SCREEN_APPS:
            draw_apps(app, apps);
            break;
        case SCREEN_SETTINGS:
            draw_settings();
            break;
        case SCREEN_STORAGE:
            draw_storage(storage);
            break;
        case SCREEN_DOWNLOADS:
        case SCREEN_UPDATES:
        case SCREEN_ACCOUNT:
        case SCREEN_PAIRING:
        default:
            draw_coming_soon(app);
            break;
    }

    DCFlushRange(xfb, rmode->fbWidth * rmode->xfbHeight * VI_DISPLAY_PIX_SZ);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_Flush();
    fflush(stdout);
}

void ui_shutdown(void)
{
    VIDEO_SetBlack(1);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

int ui_home_hit_test(s32 x, s32 y)
{
    u32 i;

    for (i = 0; i < SCREEN_COUNT; i++) {
        Rect rect = home_tiles[i];
        if (x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h) {
            return (int)i;
        }
    }

    return -1;
}
