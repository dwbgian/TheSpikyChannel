#pragma once

#include "app_state.h"

void storage_refresh(StorageState *storage, HomebrewList *apps);
void storage_shutdown(const StorageState *storage);
bool storage_create_usb_spiky_folders(StorageState *storage);
