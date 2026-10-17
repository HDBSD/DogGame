#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "host_gamepad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST_PAD_COUNT 4

typedef struct
{
    SDL_GameController *controller;
    XINPUT_STATE state;
    XINPUT_VIBRATION vibration;
    ULONGLONG rumble_time;
} HostPad;

static HostPad pads[HOST_PAD_COUNT];
static SRWLOCK pad_lock = SRWLOCK_INIT;
static INIT_ONCE initialization = INIT_ONCE_STATIC_INIT;
static BOOL ready;
static ULONGLONG last_scan, last_update;

static void close_controller(HostPad *pad)
{
    if (pad->controller)
    {
        SDL_GameControllerRumble(pad->controller, 0, 0, 0);
        SDL_GameControllerClose(pad->controller);
    }
    pad->controller = NULL;
    memset(&pad->state.Gamepad, 0, sizeof(pad->state.Gamepad));
    memset(&pad->vibration, 0, sizeof(pad->vibration));
    pad->state.dwPacketNumber++;
}

static void shutdown_controllers(void)
{
    AcquireSRWLockExclusive(&pad_lock);
    for (unsigned i = 0; i < HOST_PAD_COUNT; i++) close_controller(&pads[i]);
    if (ready) SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    ready = FALSE;
    ReleaseSRWLockExclusive(&pad_lock);
}

static BOOL CALLBACK initialize_controllers(PINIT_ONCE once, PVOID parameter, PVOID *context)
{
    (void)once; (void)parameter; (void)context;
    /* This application owns a Win32 window, not an SDL event loop. SDL's
     * joystick thread handles Windows device notifications independently. */
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0)
    {
        SDL_GameControllerEventState(SDL_IGNORE);
        SDL_JoystickEventState(SDL_IGNORE);
        ready = TRUE;
        atexit(shutdown_controllers);
    }
    else
    {
        fprintf(stderr, "[INPUT] SDL controller initialization failed: %s; using XInput.\n", SDL_GetError());
    }
    return TRUE;
}

static void scan_controllers(ULONGLONG now)
{
    for (unsigned slot = 0; slot < HOST_PAD_COUNT; slot++)
    {
        if (pads[slot].controller && !SDL_GameControllerGetAttached(pads[slot].controller))
            close_controller(&pads[slot]);
    }
    if (last_scan && now - last_scan < 1000) return;
    last_scan = now;
    for (int device = 0; device < SDL_NumJoysticks(); device++)
    {
        if (!SDL_IsGameController(device)) continue;
        SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device);
        int free_slot = -1;
        BOOL opened = FALSE;
        for (unsigned slot = 0; slot < HOST_PAD_COUNT; slot++)
        {
            if (!pads[slot].controller)
            {
                if (free_slot < 0) free_slot = (int)slot;
            }
            else if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[slot].controller)) == id)
                opened = TRUE;
        }
        if (opened || free_slot < 0) continue;
        SDL_GameController *controller = SDL_GameControllerOpen(device);
        if (!controller) continue;
        pads[free_slot].controller = controller;
        pads[free_slot].state.dwPacketNumber++;
        fprintf(stderr, "[INPUT] Port %d: %s (SDL, %04X:%04X)\n", free_slot + 1,
            SDL_GameControllerName(controller), SDL_GameControllerGetVendor(controller),
            SDL_GameControllerGetProduct(controller));
    }
}

static BYTE trigger_value(Sint16 value)
{
    return value > 0 ? (BYTE)((unsigned)value * 255 / 32767) : 0;
}

/* SDL's built-in Sony HID drivers normalize USB and Bluetooth reports. SDL
 * gamepad names use physical Xbox positions: Cross=A, Circle=B, Square=X,
 * Triangle=Y; Share/Create=Back and Options=Start. Keep the console's original
 * black/white shoulder conversion in xinput_device.c, shared with Xbox pads. */
static XINPUT_GAMEPAD read_controller(SDL_GameController *controller)
{
    static const struct { SDL_GameControllerButton button; WORD xbox; } buttons[] =
    {
        {SDL_CONTROLLER_BUTTON_A, XINPUT_GAMEPAD_A},
        {SDL_CONTROLLER_BUTTON_B, XINPUT_GAMEPAD_B},
        {SDL_CONTROLLER_BUTTON_X, XINPUT_GAMEPAD_X},
        {SDL_CONTROLLER_BUTTON_Y, XINPUT_GAMEPAD_Y},
        {SDL_CONTROLLER_BUTTON_BACK, XINPUT_GAMEPAD_BACK},
        {SDL_CONTROLLER_BUTTON_START, XINPUT_GAMEPAD_START},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, XINPUT_GAMEPAD_LEFT_THUMB},
        {SDL_CONTROLLER_BUTTON_RIGHTSTICK, XINPUT_GAMEPAD_RIGHT_THUMB},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT}
    };
    XINPUT_GAMEPAD state = {0};
    for (unsigned i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++)
        if (SDL_GameControllerGetButton(controller, buttons[i].button)) state.wButtons |= buttons[i].xbox;
    state.bLeftTrigger = trigger_value(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    state.bRightTrigger = trigger_value(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    state.sThumbLX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    state.sThumbRX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
    /* Saturate the asymmetric negative endpoint and retain exactly neutral zero. */
    int left_y = -((int)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY));
    int right_y = -((int)SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY));
    state.sThumbLY = (SHORT)(left_y > 32767 ? 32767 : left_y);
    state.sThumbRY = (SHORT)(right_y > 32767 ? 32767 : right_y);
    return state;
}

static void update_controllers(void)
{
    ULONGLONG now = SDL_GetTicks64();
    if (last_update && now == last_update) return;
    last_update = now;
    SDL_GameControllerUpdate();
    scan_controllers(now);
    for (unsigned slot = 0; slot < HOST_PAD_COUNT; slot++)
    {
        HostPad *pad = &pads[slot];
        if (!pad->controller) continue;
        XINPUT_GAMEPAD state = read_controller(pad->controller);
        if (memcmp(&state, &pad->state.Gamepad, sizeof(state)))
        {
            pad->state.Gamepad = state;
            pad->state.dwPacketNumber++;
        }
        if ((pad->vibration.wLeftMotorSpeed || pad->vibration.wRightMotorSpeed) && now - pad->rumble_time >= 1000)
        {
            SDL_GameControllerRumble(pad->controller, pad->vibration.wLeftMotorSpeed,
                pad->vibration.wRightMotorSpeed, 2000);
            pad->rumble_time = now;
        }
    }
}

DWORD host_gamepad_get_state(DWORD port, XINPUT_STATE *state)
{
    if (state) memset(state, 0, sizeof(*state));
    if (!state || port >= HOST_PAD_COUNT) return ERROR_DEVICE_NOT_CONNECTED;
    InitOnceExecuteOnce(&initialization, initialize_controllers, NULL, NULL);
    AcquireSRWLockExclusive(&pad_lock);
    if (!ready)
    {
        ReleaseSRWLockExclusive(&pad_lock);
        DWORD result = XInputGetState(port, state);
        if (result != ERROR_SUCCESS) memset(state, 0, sizeof(*state));
        return result;
    }
    update_controllers();
    DWORD result = pads[port].controller ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
    if (result == ERROR_SUCCESS) *state = pads[port].state;
    ReleaseSRWLockExclusive(&pad_lock);
    return result;
}

DWORD host_gamepad_set_state(DWORD port, const XINPUT_VIBRATION *vibration)
{
    XINPUT_STATE state;
    if (!vibration || host_gamepad_get_state(port, &state) != ERROR_SUCCESS) return ERROR_DEVICE_NOT_CONNECTED;
    if (!ready) return XInputSetState(port, (XINPUT_VIBRATION *)vibration);
    AcquireSRWLockExclusive(&pad_lock);
    if (!pads[port].controller)
    {
        ReleaseSRWLockExclusive(&pad_lock);
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    int result = SDL_GameControllerRumble(pads[port].controller, vibration->wLeftMotorSpeed,
        vibration->wRightMotorSpeed, 2000);
    if (result == 0)
    {
        pads[port].vibration = *vibration;
        pads[port].rumble_time = SDL_GetTicks64();
    }
    ReleaseSRWLockExclusive(&pad_lock);
    return result == 0 ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
}

DWORD host_gamepad_get_capabilities(DWORD port, DWORD flags, XINPUT_CAPABILITIES *caps)
{
    XINPUT_STATE state;
    if (!caps) return ERROR_DEVICE_NOT_CONNECTED;
    memset(caps, 0, sizeof(*caps));
    if (host_gamepad_get_state(port, &state) != ERROR_SUCCESS) return ERROR_DEVICE_NOT_CONNECTED;
    if (!ready) return XInputGetCapabilities(port, flags, caps);
    AcquireSRWLockExclusive(&pad_lock);
    if (!pads[port].controller)
    {
        ReleaseSRWLockExclusive(&pad_lock);
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    caps->Type = XINPUT_DEVTYPE_GAMEPAD;
    caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    if (SDL_GameControllerHasRumble(pads[port].controller))
    {
        caps->Flags = XINPUT_CAPS_FFB_SUPPORTED;
        caps->Vibration.wLeftMotorSpeed = caps->Vibration.wRightMotorSpeed = 65535;
    }
    ReleaseSRWLockExclusive(&pad_lock);
    return ERROR_SUCCESS;
}
