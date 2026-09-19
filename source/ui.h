#pragma once

#include "app_state.h"
#include "input.h"

bool ui_init(void);
void ui_draw(const AppState *app, const StorageState *storage,
             const HomebrewList *apps, const InputState *input);
void ui_shutdown(void);
int ui_home_hit_test(s32 x, s32 y);
