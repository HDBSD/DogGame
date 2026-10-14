/* Host input policy for debug flight. The player adapter owns guest writes
 * and the native animation/model update sequence. */
#include "debug_options.h"
#include "api/title_guest_player.h"
#include "api/title_guest_state.h"
#include <windows.h>
#include <Xinput.h>
#include <xbox/debug_overlay.h>
#include <math.h>
#include <stdlib.h>
extern volatile LONG title_debug_noclip;
extern int xbox_FramebufferKeyDown(int key);
extern int xbox_FramebufferFocused(void);

int title_debug_player_flight(void)
{
    if (!title_options.debug || !title_debug_noclip || title_state_menu_mode() != 11)
    {
        return 0;
    }
    TitlePlayerUpdate update = title_guest_current_player_update();
    uint32_t player = update.player, scene = update.scene, dt_bits = update.elapsed_bits;
    if (!title_guest_is_main_player(player))
    {
        return 0;
    }
    uint32_t frame = title_guest_player_frame(player), model = title_guest_player_model(player);
    if (!frame || !model)
    {
        return 0;
    }
    float dt = update.elapsed;
    if (!isfinite(dt) || dt < 0)
    {
        dt = 0;
    }
    if (dt > 0.1f)
    {
        dt = 0.1f;
    }
    float x = 0, z = 0, y = 0;
    XINPUT_STATE pad;
    if (xbox_FramebufferFocused() && !xbox_DebugOverlayCaptured())
    {
        if (XInputGetState(0, &pad) == ERROR_SUCCESS)
        {
            int lx = pad.Gamepad.sThumbLX, ly = pad.Gamepad.sThumbLY;
            if (abs(lx) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            {
                x = lx / 32768.0f;
            }
            if (abs(ly) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            {
                z = ly / 32768.0f;
            }
            y = (pad.Gamepad.bRightTrigger > 30 ? pad.Gamepad.bRightTrigger / 255.0f : 0) -
                (pad.Gamepad.bLeftTrigger > 30 ? pad.Gamepad.bLeftTrigger / 255.0f : 0);
        }
        const char *keyboard = getenv("RECOMP_KEYBOARD");
        if (!keyboard || *keyboard != '0')
        {
            x += xbox_FramebufferKeyDown('D') - xbox_FramebufferKeyDown('A');
            z += xbox_FramebufferKeyDown('W') - xbox_FramebufferKeyDown('S');
            y += xbox_FramebufferKeyDown('E') - xbox_FramebufferKeyDown('Q');
        }
    }
    float length = sqrtf(x * x + z * z + y * y);
    if (length > 1)
    {
        x /= length;
        z /= length;
        y /= length;
    }
    /* RenderWare's horizontal camera basis has the opposite sign to input
  * right/forward. Normalize away pitch, then reverse both horizontal axes. */
    float rx, rz;
    title_guest_horizontal_camera_basis(&rx, &rz);
    float basis = sqrtf(rx * rx + rz * rz);
    if (!isfinite(basis) || basis < 0.01f)
    {
        rx = 1;
        rz = 0;
    }
    else
    {
        rx /= basis;
        rz /= basis;
    }
    float dx = -(rx * x + rz * z) * 12.0f * dt, dz = -(rz * x - rx * z) * 12.0f * dt, dy = y * 12.0f * dt;
    title_guest_apply_player_flight(player, scene, frame, dt_bits, dx, dy, dz);
    return 1;
}
