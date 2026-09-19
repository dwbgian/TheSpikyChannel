#include "app_state.h"
#include "input.h"
#include "storage.h"
#include "ui.h"

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <wiiuse/wpad.h>

static void set_status(AppState *app, const char *message)
{
    snprintf(app->status_line, sizeof(app->status_line), "%s", message);
}

static void clamp_app_selection(AppState *app, const HomebrewList *apps)
{
    if (apps->count == 0) {
        app->app_selected = 0;
        app->app_scroll = 0;
        return;
    }

    if (app->app_selected >= apps->count) {
        app->app_selected = apps->count - 1;
    }

    if (app->app_scroll > app->app_selected) {
        app->app_scroll = app->app_selected;
    }

    while (app->app_selected >= app->app_scroll + SPIKY_APP_PAGE_SIZE) {
        app->app_scroll++;
    }
}

static void refresh_storage(AppState *app, StorageState *storage, HomebrewList *apps)
{
    storage_refresh(storage, apps);
    clamp_app_selection(app, apps);

    if (storage->usb_mounted || storage->sd_mounted) {
        set_status(app, "Storage refreshed. USB/SD scan complete.");
    } else {
        set_status(app, "No USB or SD storage mounted.");
    }
}

static void move_home(AppState *app, s32 delta)
{
    s32 selected = (s32)app->home_selected + delta;

    if (selected < 0) {
        selected = SCREEN_COUNT - 1;
    }

    if (selected >= SCREEN_COUNT) {
        selected = 0;
    }

    app->home_selected = (u32)selected;
}

static void handle_home(AppState *app, const InputState *input)
{
    int pointer_hit;

    if (input->pressed & WPAD_BUTTON_RIGHT) {
        move_home(app, 1);
    }

    if (input->pressed & WPAD_BUTTON_LEFT) {
        move_home(app, -1);
    }

    if (input->pressed & WPAD_BUTTON_DOWN) {
        move_home(app, 4);
    }

    if (input->pressed & WPAD_BUTTON_UP) {
        move_home(app, -4);
    }

    if (input->pointer_valid) {
        pointer_hit = ui_home_hit_test(input->pointer_x, input->pointer_y);
        if (pointer_hit >= 0) {
            app->home_selected = (u32)pointer_hit;
        }
    }

    if (input->pressed & WPAD_BUTTON_A) {
        app->screen = (SpikyScreen)app->home_selected;
        if (app->screen == SCREEN_HOME) {
            set_status(app, "Already on the Spiky dashboard.");
        } else if (app->screen == SCREEN_APPS) {
            set_status(app, "Apps are listed only. Launching is disabled in this build.");
        } else if (app->screen == SCREEN_STORAGE) {
            set_status(app, "Storage tools are USB-only and never touch NAND.");
        } else {
            set_status(app, "Feature area prepared. Network actions are coming soon.");
        }
    }
}

static void handle_apps(AppState *app, const InputState *input, const HomebrewList *apps)
{
    if (apps->count > 0) {
        if ((input->pressed & WPAD_BUTTON_DOWN) && app->app_selected + 1 < apps->count) {
            app->app_selected++;
        }

        if ((input->pressed & WPAD_BUTTON_UP) && app->app_selected > 0) {
            app->app_selected--;
        }

        if (input->pressed & WPAD_BUTTON_PLUS) {
            if (app->app_selected + SPIKY_APP_PAGE_SIZE < apps->count) {
                app->app_selected += SPIKY_APP_PAGE_SIZE;
            } else {
                app->app_selected = apps->count - 1;
            }
        }

        if (input->pressed & WPAD_BUTTON_MINUS) {
            if (app->app_selected > SPIKY_APP_PAGE_SIZE) {
                app->app_selected -= SPIKY_APP_PAGE_SIZE;
            } else {
                app->app_selected = 0;
            }
        }
    }

    if (input->pressed & WPAD_BUTTON_A) {
        set_status(app, "App launch is intentionally disabled until the loader is stable.");
    }

    clamp_app_selection(app, apps);
}

static void handle_storage(AppState *app, const InputState *input,
                           StorageState *storage, HomebrewList *apps)
{
    if (input->pressed & WPAD_BUTTON_A) {
        if (storage_create_usb_spiky_folders(storage)) {
            storage_refresh(storage, apps);
            set_status(app, "Created/verified USB:/spiky folders.");
        } else {
            storage_refresh(storage, apps);
            set_status(app, "Could not create USB:/spiky folders. Is USB mounted?");
        }
    }
}

static void handle_screen(AppState *app, const InputState *input,
                          StorageState *storage, HomebrewList *apps)
{
    if (input->pressed & WPAD_BUTTON_HOME) {
        app->running = false;
        return;
    }

    if (input->pressed & WPAD_BUTTON_B) {
        app->screen = SCREEN_HOME;
        set_status(app, "Back to dashboard.");
        return;
    }

    if (input->pressed & WPAD_BUTTON_1) {
        refresh_storage(app, storage, apps);
    }

    switch (app->screen) {
        case SCREEN_HOME:
            handle_home(app, input);
            break;
        case SCREEN_APPS:
            handle_apps(app, input, apps);
            break;
        case SCREEN_STORAGE:
            handle_storage(app, input, storage, apps);
            break;
        case SCREEN_DOWNLOADS:
        case SCREEN_UPDATES:
        case SCREEN_ACCOUNT:
        case SCREEN_PAIRING:
        case SCREEN_SETTINGS:
        default:
            if (input->pressed & WPAD_BUTTON_A) {
                set_status(app, "Prepared area only. No network or system changes run.");
            }
            break;
    }
}

int main(int argc, char **argv)
{
    AppState app;
    StorageState storage;
    HomebrewList apps;
    InputState input;

    (void)argc;
    (void)argv;

    memset(&app, 0, sizeof(app));
    memset(&storage, 0, sizeof(storage));
    memset(&apps, 0, sizeof(apps));

    app.screen = SCREEN_HOME;
    app.home_selected = SCREEN_APPS;
    app.running = true;

    if (!ui_init()) {
        return 1;
    }

    input_init(VIDEO_GetPreferredMode(NULL));
    refresh_storage(&app, &storage, &apps);

    while (app.running) {
        input_poll(&input);
        handle_screen(&app, &input, &storage, &apps);
        ui_draw(&app, &storage, &apps, &input);
        VIDEO_WaitVSync();
    }

    storage_shutdown(&storage);
    WPAD_Shutdown();
    ui_shutdown();

    return 0;
}
