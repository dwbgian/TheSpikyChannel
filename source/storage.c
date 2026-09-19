#include "storage.h"

#include <dirent.h>
#include <fat.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static bool file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return false;
    }

    fclose(file);
    return true;
}

static bool dir_exists(const char *path)
{
    DIR *dir = opendir(path);

    if (dir == NULL) {
        return false;
    }

    closedir(dir);
    return true;
}

static void copy_string(char *dest, size_t dest_size, const char *src)
{
    if (dest_size == 0) {
        return;
    }

    snprintf(dest, dest_size, "%s", src);
}

static bool extract_meta_name(const char *path, char *name, size_t name_size)
{
    FILE *file;
    char buffer[4097];
    size_t read;
    char *start;
    char *end;
    size_t len;

    file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    read = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[read] = '\0';

    start = strstr(buffer, "<name>");
    end = strstr(buffer, "</name>");
    if (start == NULL || end == NULL || end <= start) {
        return false;
    }

    start += 6;
    len = (size_t)(end - start);
    if (len >= name_size) {
        len = name_size - 1;
    }

    memcpy(name, start, len);
    name[len] = '\0';
    return len > 0;
}

static void add_app(HomebrewList *apps,
                    const char *device,
                    const char *base_path,
                    const char *folder_name,
                    bool spiky_space)
{
    HomebrewApp *app;
    char app_path[SPIKY_PATH_LEN];
    char boot_path[SPIKY_PATH_LEN];
    char meta_path[SPIKY_PATH_LEN];

    if (apps->count >= SPIKY_MAX_APPS) {
        apps->truncated = true;
        return;
    }

    snprintf(app_path, sizeof(app_path), "%s/%s", base_path, folder_name);
    if (!dir_exists(app_path)) {
        return;
    }

    snprintf(boot_path, sizeof(boot_path), "%s/boot.dol", app_path);
    snprintf(meta_path, sizeof(meta_path), "%s/meta.xml", app_path);

    app = &apps->items[apps->count++];
    memset(app, 0, sizeof(*app));
    copy_string(app->device, sizeof(app->device), device);
    copy_string(app->path, sizeof(app->path), app_path);
    copy_string(app->name, sizeof(app->name), folder_name);
    app->has_boot_dol = file_exists(boot_path);
    app->has_meta_xml = file_exists(meta_path);
    app->spiky_space = spiky_space;

    if (app->has_meta_xml) {
        extract_meta_name(meta_path, app->name, sizeof(app->name));
    }
}

static void scan_apps_in(HomebrewList *apps,
                         const char *device,
                         const char *base_path,
                         bool spiky_space)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(base_path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        add_app(apps, device, base_path, entry->d_name, spiky_space);
    }

    closedir(dir);
}

static void scan_all_apps(const StorageState *storage, HomebrewList *apps)
{
    memset(apps, 0, sizeof(*apps));

    if (storage->usb_mounted) {
        scan_apps_in(apps, "USB", "usb:/apps", false);
        scan_apps_in(apps, "USB", "usb:/spiky/apps", true);
    }

    if (storage->sd_mounted) {
        scan_apps_in(apps, "SD", "sd:/apps", false);
        scan_apps_in(apps, "SD", "sd:/spiky/apps", true);
    }
}

void storage_refresh(StorageState *storage, HomebrewList *apps)
{
    bool created = storage->created_usb_folders;

    if (storage->usb_mounted && !dir_exists("usb:/")) {
        fatUnmount("usb:/");
        storage->usb_mounted = false;
    }

    if (storage->sd_mounted && !dir_exists("sd:/")) {
        fatUnmount("sd:/");
        storage->sd_mounted = false;
    }

    storage->usb_mounted = storage->usb_mounted ||
                           fatMountSimple("usb", &__io_usbstorage);
    storage->sd_mounted = storage->sd_mounted ||
                          fatMountSimple("sd", &__io_wiisd);

    storage->created_usb_folders = created;
    storage->usb_core_found = false;
    storage->sd_core_found = false;
    storage->usb_folders_ready = false;

    storage->usb_core_found = storage->usb_mounted &&
                              file_exists("usb:/spiky/core/boot.dol");
    storage->sd_core_found = storage->sd_mounted &&
                             file_exists("sd:/spiky/core/boot.dol");
    storage->usb_folders_ready = storage->usb_mounted &&
                                 dir_exists("usb:/spiky") &&
                                 dir_exists("usb:/spiky/core") &&
                                 dir_exists("usb:/spiky/apps") &&
                                 dir_exists("usb:/spiky/downloads") &&
                                 dir_exists("usb:/spiky/cache") &&
                                 dir_exists("usb:/spiky/config") &&
                                 dir_exists("usb:/spiky/account");

    scan_all_apps(storage, apps);
}

void storage_shutdown(const StorageState *storage)
{
    if (storage->usb_mounted) {
        fatUnmount("usb:/");
    }

    if (storage->sd_mounted) {
        fatUnmount("sd:/");
    }
}

bool storage_create_usb_spiky_folders(StorageState *storage)
{
    if (!storage->usb_mounted) {
        return false;
    }

    mkdir("usb:/spiky", 0777);
    mkdir("usb:/spiky/core", 0777);
    mkdir("usb:/spiky/apps", 0777);
    mkdir("usb:/spiky/channels", 0777);
    mkdir("usb:/spiky/downloads", 0777);
    mkdir("usb:/spiky/cache", 0777);
    mkdir("usb:/spiky/config", 0777);
    mkdir("usb:/spiky/account", 0777);

    storage->created_usb_folders = true;
    storage->usb_folders_ready = dir_exists("usb:/spiky") &&
                                 dir_exists("usb:/spiky/core") &&
                                 dir_exists("usb:/spiky/apps") &&
                                 dir_exists("usb:/spiky/downloads") &&
                                 dir_exists("usb:/spiky/cache") &&
                                 dir_exists("usb:/spiky/config") &&
                                 dir_exists("usb:/spiky/account");

    return storage->usb_folders_ready;
}
