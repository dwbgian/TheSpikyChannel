#pragma once

#include <stdbool.h>
#include <gccore.h>

#define SPIKY_MAX_APPS 96
#define SPIKY_NAME_LEN 64
#define SPIKY_PATH_LEN 160
#define SPIKY_APP_PAGE_SIZE 6

typedef enum SpikyScreen {
    SCREEN_HOME = 0,
    SCREEN_APPS,
    SCREEN_DOWNLOADS,
    SCREEN_UPDATES,
    SCREEN_ACCOUNT,
    SCREEN_PAIRING,
    SCREEN_SETTINGS,
    SCREEN_STORAGE,
    SCREEN_COUNT
} SpikyScreen;

typedef struct HomebrewApp {
    char device[4];
    char name[SPIKY_NAME_LEN];
    char path[SPIKY_PATH_LEN];
    bool has_boot_dol;
    bool has_meta_xml;
    bool spiky_space;
} HomebrewApp;

typedef struct HomebrewList {
    HomebrewApp items[SPIKY_MAX_APPS];
    u32 count;
    bool truncated;
} HomebrewList;

typedef struct StorageState {
    bool usb_mounted;
    bool sd_mounted;
    bool usb_core_found;
    bool sd_core_found;
    bool usb_folders_ready;
    bool created_usb_folders;
} StorageState;

typedef struct AppState {
    SpikyScreen screen;
    u32 home_selected;
    u32 app_selected;
    u32 app_scroll;
    char status_line[96];
    bool running;
} AppState;
