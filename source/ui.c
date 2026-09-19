#include "ui.h"

#include "ui_background.h"
#include "ui_font.h"

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

typedef struct Color {
    u8 r;
    u8 g;
    u8 b;
} Color;

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static const Color C_BLACK = { 18, 20, 24 };
static const Color C_MUTED = { 92, 98, 108 };
static const Color C_RED = { 228, 0, 18 };
static const Color C_RED_DARK = { 184, 0, 16 };
static const Color C_WHITE = { 255, 255, 255 };
static const Color C_PANEL = { 252, 252, 253 };
static const Color C_PANEL_SOFT = { 244, 246, 248 };
static const Color C_BORDER = { 188, 198, 208 };
static const Color C_GREEN = { 36, 190, 64 };

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
    { 34, 154, 132, 82 },
    { 184, 154, 132, 82 },
    { 334, 154, 132, 82 },
    { 484, 154, 132, 82 },
    { 34, 252, 132, 82 },
    { 184, 252, 132, 82 },
    { 334, 252, 132, 82 },
    { 484, 252, 132, 82 }
};

static u8 clamp_u8(int value)
{
    if (value < 0) {
        return 0;
    }

    if (value > 255) {
        return 255;
    }

    return (u8)value;
}

static void rgb_to_ycbcr(Color color, u8 *y, u8 *cb, u8 *cr)
{
    *y = clamp_u8((int)(0.299f * color.r + 0.587f * color.g + 0.114f * color.b));
    *cb = clamp_u8((int)(128.0f - 0.168736f * color.r - 0.331264f * color.g + 0.5f * color.b));
    *cr = clamp_u8((int)(128.0f + 0.5f * color.r - 0.418688f * color.g - 0.081312f * color.b));
}

static u32 rgb_to_yuyv(Color color)
{
    u8 y;
    u8 cb;
    u8 cr;

    rgb_to_ycbcr(color, &y, &cb, &cr);
    return ((u32)y << 24) | ((u32)cb << 16) | ((u32)y << 8) | (u32)cr;
}

static void draw_rect(s32 x, s32 y, s32 w, s32 h, Color color)
{
    u32 yuyv;
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
    yuyv = rgb_to_yuyv(color);
    dst = (u32 *)xfb;
    words_per_row = rmode->fbWidth / 2;

    for (row = y0; row < y1; row++) {
        for (col = x0 / 2; col < x1 / 2; col++) {
            dst[row * words_per_row + col] = yuyv;
        }
    }
}

static void draw_pixel(s32 x, s32 y, Color color)
{
    u8 yy;
    u8 cb;
    u8 cr;
    u32 *dst;
    u32 word;
    u32 y0;
    u32 y1;
    s32 index;

    if (xfb == NULL || rmode == NULL ||
        x < 0 || y < 0 ||
        x >= (s32)rmode->fbWidth || y >= (s32)rmode->xfbHeight) {
        return;
    }

    rgb_to_ycbcr(color, &yy, &cb, &cr);
    dst = (u32 *)xfb;
    index = y * (s32)(rmode->fbWidth / 2) + x / 2;
    word = dst[index];
    y0 = (word >> 24) & 0xff;
    y1 = (word >> 8) & 0xff;

    if ((x & 1) == 0) {
        y0 = yy;
    } else {
        y1 = yy;
    }

    dst[index] = (y0 << 24) | ((u32)cb << 16) | (y1 << 8) | cr;
}

static void draw_border(Rect rect, Color color, s32 width)
{
    s32 i;

    for (i = 0; i < width; i++) {
        draw_rect(rect.x + i, rect.y + i, rect.w - i * 2, 2, color);
        draw_rect(rect.x + i, rect.y + rect.h - 2 - i, rect.w - i * 2, 2, color);
        draw_rect(rect.x + i, rect.y + i, 2, rect.h - i * 2, color);
        draw_rect(rect.x + rect.w - 2 - i, rect.y + i, 2, rect.h - i * 2, color);
    }
}

static void draw_panel(Rect rect, bool selected)
{
    Color border = selected ? C_RED : C_BORDER;

    draw_rect(rect.x + 4, rect.y + 6, rect.w, rect.h, (Color){ 210, 216, 224 });
    draw_rect(rect.x, rect.y, rect.w, rect.h, selected ? C_WHITE : C_PANEL);
    draw_border(rect, border, selected ? 4 : 2);
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

static int text_width(const SpikyFont *font, const char *text)
{
    int width = 0;

    while (*text != '\0') {
        unsigned char ch = (unsigned char)*text++;
        if (ch < SPIKY_FONT_FIRST || ch > SPIKY_FONT_LAST) {
            ch = '?';
        }
        width += font->widths[ch - SPIKY_FONT_FIRST];
    }

    return width;
}

static void draw_text(const SpikyFont *font, s32 x, s32 y, const char *text, Color color)
{
    while (*text != '\0') {
        unsigned char ch = (unsigned char)*text++;
        u32 glyph_index;
        u8 glyph_width;
        u32 offset;
        u32 row_bytes;
        u32 row;
        u32 col;

        if (ch < SPIKY_FONT_FIRST || ch > SPIKY_FONT_LAST) {
            ch = '?';
        }

        glyph_index = ch - SPIKY_FONT_FIRST;
        glyph_width = font->widths[glyph_index];
        offset = font->offsets[glyph_index];
        row_bytes = (glyph_width + 7) / 8;

        for (row = 0; row < font->height; row++) {
            for (col = 0; col < glyph_width; col++) {
                u8 byte = font->bits[offset + row * row_bytes + col / 8];
                if (byte & (0x80 >> (col & 7))) {
                    draw_pixel(x + (s32)col, y + (s32)row, color);
                }
            }
        }

        x += glyph_width;
    }
}

static void draw_text_center(const SpikyFont *font, Rect rect, const char *text, Color color)
{
    s32 x = rect.x + (rect.w - text_width(font, text)) / 2;
    s32 y = rect.y + (rect.h - font->height) / 2;

    draw_text(font, x, y, text, color);
}

static void draw_status_dot(s32 x, s32 y, Color color)
{
    draw_rect(x + 3, y, 8, 14, color);
    draw_rect(x, y + 3, 14, 8, color);
}

static void draw_control_button(s32 x, s32 y, const char *key, const char *label)
{
    draw_rect(x, y, 34, 30, C_PANEL_SOFT);
    draw_border((Rect){ x, y, 34, 30 }, (Color){ 104, 112, 124 }, 2);
    draw_text_center(&spiky_font_body, (Rect){ x, y + 1, 34, 26 }, key, C_BLACK);
    draw_text(&spiky_font_body, x + 44, y + 5, label, C_BLACK);
}

static void draw_icon_bar(Rect rect, Color color)
{
    draw_rect(rect.x + 22, rect.y + 22, rect.w - 44, 12, color);
    draw_rect(rect.x + 22, rect.y + 46, rect.w - 44, 12, color);
    draw_rect(rect.x + 22, rect.y + 70, rect.w - 44, 12, color);
}

static void draw_icon_grid(Rect rect, Color color)
{
    s32 size = 20;
    draw_rect(rect.x + 28, rect.y + 24, size, size, color);
    draw_rect(rect.x + 58, rect.y + 24, size, size, color);
    draw_rect(rect.x + 28, rect.y + 54, size, size, color);
    draw_rect(rect.x + 58, rect.y + 54, size, size, color);
}

static void draw_icon_storage(Rect rect, Color color)
{
    draw_rect(rect.x + 38, rect.y + 20, 52, 56, color);
    draw_rect(rect.x + 46, rect.y + 12, 36, 14, color);
    draw_rect(rect.x + 48, rect.y + 82, 32, 8, color);
}

static void draw_icon_download(Rect rect, Color color)
{
    draw_rect(rect.x + 58, rect.y + 18, 16, 48, color);
    draw_rect(rect.x + 42, rect.y + 50, 48, 16, color);
    draw_rect(rect.x + 34, rect.y + 80, 64, 10, color);
}

static void draw_icon_settings(Rect rect, Color color)
{
    draw_rect(rect.x + 34, rect.y + 28, 64, 12, color);
    draw_rect(rect.x + 34, rect.y + 58, 64, 12, color);
    draw_rect(rect.x + 50, rect.y + 20, 12, 28, color);
    draw_rect(rect.x + 74, rect.y + 50, 12, 28, color);
}

static void draw_icon_for_screen(Rect rect, SpikyScreen screen, Color color)
{
    switch (screen) {
        case SCREEN_APPS:
            draw_icon_grid(rect, color);
            break;
        case SCREEN_STORAGE:
            draw_icon_storage(rect, color);
            break;
        case SCREEN_DOWNLOADS:
        case SCREEN_UPDATES:
            draw_icon_download(rect, color);
            break;
        case SCREEN_SETTINGS:
            draw_icon_settings(rect, color);
            break;
        case SCREEN_HOME:
        case SCREEN_ACCOUNT:
        case SCREEN_PAIRING:
        default:
            draw_icon_bar(rect, color);
            break;
    }
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

    draw_text(&spiky_font_title, 112, 34, "The Spiky Channel", C_BLACK);
    draw_rect(112, 70, 170, 4, C_RED);
    draw_text(&spiky_font_body, 114, 82, screen_subtitles[app->screen], C_RED_DARK);

    draw_panel((Rect){ 408, 26, 202, 76 }, false);
    snprintf(line, sizeof(line), "USB %s", storage->usb_mounted ? "Connected" : "Missing");
    draw_status_dot(426, 42, storage->usb_mounted ? C_GREEN : C_RED);
    draw_text(&spiky_font_body, 448, 38, line, C_BLACK);

    snprintf(line, sizeof(line), "SD %s", storage->sd_mounted ? "Connected" : "Missing");
    draw_status_dot(426, 68, storage->sd_mounted ? C_GREEN : C_RED);
    draw_text(&spiky_font_body, 448, 64, line, C_BLACK);

    if (app->status_line[0] != '\0') {
        draw_text(&spiky_font_body, 36, 112, app->status_line, C_BLACK);
    }
}

static void draw_home(const AppState *app)
{
    u32 i;

    for (i = 0; i < SCREEN_COUNT; i++) {
        Rect rect = home_tiles[i];
        bool selected = i == app->home_selected;
        Color ink = selected ? C_WHITE : C_BLACK;

        draw_panel(rect, selected);
        if (selected) {
            draw_rect(rect.x + 8, rect.y + 8, rect.w - 16, rect.h - 16, C_RED);
        }

        draw_icon_for_screen((Rect){ rect.x + 14, rect.y + 8, rect.w - 28, 52 },
                             (SpikyScreen)i, selected ? C_WHITE : C_RED);
        draw_text_center(&spiky_font_body,
                         (Rect){ rect.x + 8, rect.y + 56, rect.w - 16, 22 },
                         screen_labels[i], ink);
    }

    draw_control_button(34, 372, "A", "Open");
    draw_control_button(178, 372, "B", "Back");
    draw_control_button(322, 372, "+/-", "Page");
    draw_control_button(484, 372, "HOME", "Exit");
}

static void draw_apps(const AppState *app, const HomebrewList *apps)
{
    u32 i;
    char line[128];

    draw_panel((Rect){ 36, 136, 568, 254 }, false);
    snprintf(line, sizeof(line), "%lu app(s) found%s",
             (unsigned long)apps->count,
             apps->truncated ? " - list truncated" : "");
    draw_text(&spiky_font_title, 58, 152, "Apps", C_BLACK);
    draw_text(&spiky_font_body, 146, 160, line, C_MUTED);

    if (apps->count == 0) {
        draw_text(&spiky_font_body, 68, 214, "No apps found on USB or SD.", C_BLACK);
        draw_text(&spiky_font_body, 68, 244, "Scanned: /apps and /spiky/apps.", C_MUTED);
        draw_text(&spiky_font_body, 68, 274, "This build lists apps only. It does not launch them.", C_MUTED);
    } else {
        for (i = 0; i < SPIKY_APP_PAGE_SIZE && app->app_scroll + i < apps->count; i++) {
            const HomebrewApp *entry = &apps->items[app->app_scroll + i];
            bool selected = (app->app_scroll + i) == app->app_selected;
            Rect row = { 58, 198 + (s32)i * 30, 524, 25 };

            draw_rect(row.x, row.y, row.w, row.h, selected ? C_RED : C_PANEL_SOFT);
            draw_border(row, selected ? C_RED_DARK : (Color){ 220, 226, 232 }, 1);

            snprintf(line, sizeof(line), "%s  %s", entry->device, entry->name);
            draw_text(&spiky_font_body, row.x + 12, row.y + 3,
                      line, selected ? C_WHITE : C_BLACK);

            snprintf(line, sizeof(line), "%s | %s | %s",
                     entry->spiky_space ? "spiky" : "hbc",
                     entry->has_boot_dol ? "boot.dol" : "missing dol",
                     entry->has_meta_xml ? "meta.xml" : "no meta");
            draw_text(&spiky_font_body, row.x + 270, row.y + 3,
                      line, selected ? C_WHITE : C_MUTED);
        }
    }

    draw_control_button(34, 410, "+/-", "Scroll");
    draw_control_button(220, 410, "B", "Home");
    draw_control_button(404, 410, "HOME", "Exit");
}

static void draw_message_screen(const AppState *app)
{
    char line[96];

    draw_panel((Rect){ 54, 146, 532, 210 }, false);
    draw_text(&spiky_font_title, 82, 166, screen_labels[app->screen], C_BLACK);
    snprintf(line, sizeof(line), "%s is prepared, but offline in this build.",
             screen_labels[app->screen]);
    draw_text(&spiky_font_body, 84, 216, line, C_BLACK);
    draw_text(&spiky_font_body, 84, 246, "No downloads, accounts, pairing, or web calls run yet.", C_MUTED);
    draw_text(&spiky_font_body, 84, 276, "Next: manifest, checksums, and opt-in network code.", C_MUTED);
    draw_control_button(220, 386, "B", "Home");
    draw_control_button(404, 386, "HOME", "Exit");
}

static void draw_settings(void)
{
    draw_panel((Rect){ 54, 146, 532, 210 }, false);
    draw_text(&spiky_font_title, 82, 166, "Settings", C_BLACK);
    draw_text(&spiky_font_body, 84, 216, "Theme: Spiky Red", C_BLACK);
    draw_text(&spiky_font_body, 84, 246, "Launch mode: USB-only core", C_MUTED);
    draw_text(&spiky_font_body, 84, 276, "Network: disabled in this build", C_MUTED);
    draw_text(&spiky_font_body, 84, 306, "NAND/WAD/IOS actions: not present", C_RED_DARK);
    draw_control_button(220, 386, "B", "Home");
    draw_control_button(404, 386, "HOME", "Exit");
}

static void draw_storage(const StorageState *storage)
{
    draw_panel((Rect){ 54, 136, 532, 238 }, false);
    draw_text(&spiky_font_title, 82, 154, "Storage", C_BLACK);
    draw_text(&spiky_font_body, 84, 204,
              storage->usb_mounted ? "USB: mounted" : "USB: not mounted", C_BLACK);
    draw_text(&spiky_font_body, 84, 234,
              storage->sd_mounted ? "SD: mounted" : "SD: not mounted", C_BLACK);
    draw_text(&spiky_font_body, 84, 264,
              storage->usb_core_found ? "USB core: usb:/spiky/core/boot.dol found"
                                      : "USB core: missing", C_MUTED);
    draw_text(&spiky_font_body, 84, 294,
              storage->usb_folders_ready ? "USB folders: ready"
                                         : "USB folders: incomplete", C_MUTED);
    draw_text(&spiky_font_body, 84, 324, "A creates missing USB:/spiky folders only.", C_RED_DARK);
    draw_control_button(34, 410, "A", "Create");
    draw_control_button(220, 410, "B", "Home");
    draw_control_button(404, 410, "HOME", "Exit");
}

void ui_draw(const AppState *app, const StorageState *storage,
             const HomebrewList *apps, const InputState *input)
{
    bool connected = storage->usb_mounted || storage->sd_mounted;
    (void)input;

    draw_background(connected);
    draw_header(app, storage);

    switch (app->screen) {
        case SCREEN_HOME:
            draw_home(app);
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
            draw_message_screen(app);
            break;
    }

    DCFlushRange(xfb, rmode->fbWidth * rmode->xfbHeight * VI_DISPLAY_PIX_SZ);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_Flush();
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
