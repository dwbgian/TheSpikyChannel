#include "input.h"

#include <string.h>
#include <wiiuse/wpad.h>

void input_init(GXRModeObj *rmode)
{
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

    if (rmode != NULL) {
        WPAD_SetVRes(WPAD_CHAN_0, rmode->fbWidth, rmode->xfbHeight);
    }
}

void input_poll(InputState *input)
{
    WPADData *pad;

    memset(input, 0, sizeof(*input));

    WPAD_ScanPads();
    input->pressed = WPAD_ButtonsDown(WPAD_CHAN_0);
    input->held = WPAD_ButtonsHeld(WPAD_CHAN_0);

    pad = WPAD_Data(WPAD_CHAN_0);
    if (pad != NULL && pad->ir.valid) {
        input->pointer_valid = true;
        input->pointer_x = (s32)pad->ir.x;
        input->pointer_y = (s32)pad->ir.y;
    }
}
