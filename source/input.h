#pragma once

#include <stdbool.h>
#include <gccore.h>

typedef struct InputState {
    u32 pressed;
    u32 held;
    bool pointer_valid;
    s32 pointer_x;
    s32 pointer_y;
} InputState;

void input_init(GXRModeObj *rmode);
void input_poll(InputState *input);
