#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "host_gamepad.h"
#include "xinput_xbox.h"
#include <stdio.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Failed %d: %s (%s)\n", __LINE__, #condition, SDL_GetError()); return 1; } } while (0)

static WORD rumble_left, rumble_right;
static unsigned keyboard_serial;
static int keyboard_a, captured;
int xbox_FramebufferKeyDown(int key) { return key == VK_SPACE && keyboard_a; }
unsigned xbox_FramebufferKeyboardSerial(void) { return keyboard_serial; }
int xbox_DebugOverlayCaptured(void) { return captured; }

static int SDLCALL rumble(void *unused, Uint16 left, Uint16 right)
{
    (void)unused;
    rumble_left = left; rumble_right = right;
    return 0;
}

static int attach_controller(Uint16 product, const char *name)
{
    SDL_VirtualJoystickDesc desc = {0};
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.vendor_id = 0x054C; desc.product_id = product;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_MAX) - 1;
    desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_MAX) - 1;
    desc.name = name; desc.Rumble = rumble;
    return SDL_JoystickAttachVirtualEx(&desc);
}

int main(void)
{
    /* Virtual devices exercise the actual SDL -> adapter -> guest boundary.
     * They do not validate Sony's physical USB/Bluetooth report transport. */
    SDL_SetMainReady();
    /* Isolate the fixture from physical devices and their vibration motors. */
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
    SDL_SetHint(SDL_HINT_DIRECTINPUT_ENABLED, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "0");
    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_HIDAPI, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_HIDAPI_PS4, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_HIDAPI_PS5, "0", SDL_HINT_OVERRIDE);
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    CHECK(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0);
    CHECK(attach_controller(0x05C4, "DogGame virtual DualShock 4") >= 0);
    CHECK(attach_controller(0x0CE6, "DogGame virtual DualSense") >= 0);
    SDL_Joystick *first = NULL, *second = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        SDL_Joystick *joystick = SDL_JoystickOpen(i);
        if (SDL_JoystickGetVendor(joystick) == 0x054C && SDL_JoystickIsVirtual(i))
        {
            if (!first) first = joystick;
            else second = joystick;
        }
        else SDL_JoystickClose(joystick);
    }
    CHECK(first && second);
    SDL_JoystickSetVirtualAxis(first, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
    SDL_JoystickSetVirtualAxis(first, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    SDL_JoystickSetVirtualAxis(second, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
    SDL_JoystickSetVirtualAxis(second, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    XINPUT_STATE state, previous;
    CHECK(host_gamepad_get_state(0, &state) == ERROR_SUCCESS);
    CHECK(host_gamepad_get_state(1, &state) == ERROR_SUCCESS);
    /* Find the virtual devices' stable ports, allowing real pads to coexist. */
    unsigned first_port = 4, second_port = 4;
    SDL_JoystickSetVirtualButton(first, SDL_CONTROLLER_BUTTON_A, 1);
    SDL_JoystickSetVirtualButton(second, SDL_CONTROLLER_BUTTON_Y, 1);
    Sleep(3);
    for (unsigned i = 0; i < 4; i++)
    {
        if (host_gamepad_get_state(i, &state) != ERROR_SUCCESS) continue;
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) first_port = i;
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) second_port = i;
    }
    CHECK(first_port < 4 && second_port < 4 && first_port != second_port);
    static const struct { SDL_GameControllerButton button; WORD expected; } mapping[] =
    {
        {SDL_CONTROLLER_BUTTON_A, XINPUT_GAMEPAD_A}, {SDL_CONTROLLER_BUTTON_B, XINPUT_GAMEPAD_B},
        {SDL_CONTROLLER_BUTTON_X, XINPUT_GAMEPAD_X}, {SDL_CONTROLLER_BUTTON_Y, XINPUT_GAMEPAD_Y},
        {SDL_CONTROLLER_BUTTON_BACK, XINPUT_GAMEPAD_BACK}, {SDL_CONTROLLER_BUTTON_START, XINPUT_GAMEPAD_START},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, XINPUT_GAMEPAD_LEFT_THUMB}, {SDL_CONTROLLER_BUTTON_RIGHTSTICK, XINPUT_GAMEPAD_RIGHT_THUMB},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER}
    };
    for (unsigned device = 0; device < 2; device++)
    {
        SDL_Joystick *joystick = device ? second : first;
        unsigned port = device ? second_port : first_port;
        for (unsigned b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++) SDL_JoystickSetVirtualButton(joystick, b, 0);
        for (unsigned i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++)
        {
            SDL_JoystickSetVirtualButton(joystick, mapping[i].button, 1); Sleep(2);
            CHECK(host_gamepad_get_state(port, &state) == ERROR_SUCCESS);
            CHECK(state.Gamepad.wButtons == mapping[i].expected);
            previous = state; Sleep(2);
            CHECK(host_gamepad_get_state(port, &state) == ERROR_SUCCESS && state.dwPacketNumber == previous.dwPacketNumber);
            SDL_JoystickSetVirtualButton(joystick, mapping[i].button, 0); Sleep(2);
            CHECK(host_gamepad_get_state(port, &state) == ERROR_SUCCESS && !state.Gamepad.wButtons);
            CHECK(state.dwPacketNumber != previous.dwPacketNumber);
        }
        CHECK(host_gamepad_get_state(port, &state) == ERROR_SUCCESS);
        CHECK(!state.Gamepad.sThumbLX && !state.Gamepad.sThumbLY && !state.Gamepad.sThumbRX && !state.Gamepad.sThumbRY);
        CHECK(!state.Gamepad.bLeftTrigger && !state.Gamepad.bRightTrigger);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0); Sleep(3);
        CHECK(host_gamepad_get_state(port, &state) == ERROR_SUCCESS && state.Gamepad.bLeftTrigger == 127);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, -32768);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTY, -32768);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTX, 32767);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTY, 32767);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 32767);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768); Sleep(3);
        CHECK(host_gamepad_get_state(port, &state) == ERROR_SUCCESS);
        CHECK(state.Gamepad.sThumbLX == -32768 && state.Gamepad.sThumbLY == 32767);
        CHECK(state.Gamepad.sThumbRX == 32767 && state.Gamepad.sThumbRY == -32767);
        CHECK(state.Gamepad.bLeftTrigger == 255 && state.Gamepad.bRightTrigger == 0);
        XINPUT_VIBRATION vibration = {12345, 54321};
        CHECK(host_gamepad_set_state(port, &vibration) == ERROR_SUCCESS);
        CHECK(rumble_left == 12345 && rumble_right == 54321);
        vibration.wLeftMotorSpeed = vibration.wRightMotorSpeed = 0;
        CHECK(host_gamepad_set_state(port, &vibration) == ERROR_SUCCESS && !rumble_left && !rumble_right);
        XINPUT_CAPABILITIES caps;
        CHECK(host_gamepad_get_capabilities(port, 0, &caps) == ERROR_SUCCESS && caps.Type == XINPUT_DEVTYPE_GAMEPAD && (caps.Flags & XINPUT_CAPS_FFB_SUPPORTED));
        XBOX_INPUT_STATE guest;
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1); Sleep(3);
        CHECK(xbox_InputGetState(port, &guest) == ERROR_SUCCESS && guest.Gamepad.bAnalogButtons[XBOX_BUTTON_A] == 255);
        CHECK(guest.Gamepad.bAnalogButtons[XBOX_BUTTON_LTRIGGER] == 255);
    }
    /* Simulate a slow menu frame: neither menu nor guest code polls between
     * press and release. The independent sampler must retain the tap for both
     * consumers without turning it into a held guest button. */
    SDL_JoystickSetVirtualButton(first, SDL_CONTROLLER_BUTTON_A, 0);
    Sleep(30);
    HostGamepadMenuReader debug_menu = {0}, settings_menu = {0};
    WORD pressed;
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_SUCCESS);
    CHECK(host_gamepad_read_menu(first_port, &settings_menu, &state, &pressed) == ERROR_SUCCESS);
    SDL_JoystickSetVirtualButton(first, SDL_CONTROLLER_BUTTON_BACK, 1);
    Sleep(40);
    SDL_JoystickSetVirtualButton(first, SDL_CONTROLLER_BUTTON_BACK, 0);
    Sleep(80);
    CHECK(host_gamepad_get_state(first_port, &state) == ERROR_SUCCESS && !state.Gamepad.wButtons);
    XBOX_INPUT_STATE released_guest;
    CHECK(xbox_InputGetState(first_port, &released_guest) == ERROR_SUCCESS && !released_guest.Gamepad.wButtons);
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_SUCCESS);
    CHECK(pressed == XINPUT_GAMEPAD_BACK && !state.Gamepad.wButtons);
    CHECK(host_gamepad_read_menu(first_port, &settings_menu, &state, &pressed) == ERROR_SUCCESS);
    CHECK(pressed == XINPUT_GAMEPAD_BACK);
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_SUCCESS && !pressed);
    SDL_JoystickSetVirtualButton(first, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1);
    Sleep(30);
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_SUCCESS);
    CHECK(pressed == XINPUT_GAMEPAD_DPAD_DOWN && state.Gamepad.wButtons == pressed);
    Sleep(30);
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_SUCCESS && !pressed);
    SDL_JoystickSetVirtualButton(first, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 0);
    Sleep(30);
    /* Queue repeated navigation and confirm before either menu runs. The old
     * bit history collapsed the two Down taps and lost the sequence. */
    HostGamepadMenuReader burst_first = {0}, burst_second = {0};
    CHECK(host_gamepad_read_menu(first_port, &burst_first, &state, &pressed) == ERROR_SUCCESS);
    CHECK(host_gamepad_read_menu(first_port, &burst_second, &state, &pressed) == ERROR_SUCCESS);
    const SDL_GameControllerButton burst_buttons[] = {
        SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_DPAD_UP};
    const WORD burst_expected[] = {
        XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN,
        XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_DPAD_UP};
    for (unsigned i = 0; i < 4; i++)
    {
        SDL_JoystickSetVirtualButton(first, burst_buttons[i], 1);
        Sleep(25);
        SDL_JoystickSetVirtualButton(first, burst_buttons[i], 0);
        Sleep(25);
    }
    CHECK(host_gamepad_get_state(first_port, &state) == ERROR_SUCCESS && !state.Gamepad.wButtons);
    for (unsigned i = 0; i < 4; i++)
    {
        CHECK(host_gamepad_read_menu(first_port, &burst_first, &state, &pressed) == ERROR_SUCCESS);
        CHECK(pressed == burst_expected[i] && !state.Gamepad.wButtons);
    }
    CHECK(host_gamepad_read_menu(first_port, &burst_first, &state, &pressed) == ERROR_SUCCESS && !pressed);
    for (unsigned i = 0; i < 4; i++)
    {
        CHECK(host_gamepad_read_menu(first_port, &burst_second, &state, &pressed) == ERROR_SUCCESS);
        CHECK(pressed == burst_expected[i]);
    }
    captured = 1;
    XBOX_INPUT_STATE guest;
    CHECK(xbox_InputGetState(first_port, &guest) == ERROR_SUCCESS && !guest.Gamepad.bAnalogButtons[XBOX_BUTTON_A]);
    captured = 0;
    /* Detach by current device index: removing the first changes later indices. */
    SDL_JoystickID removed = SDL_JoystickInstanceID(first);
    SDL_JoystickClose(first);
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_JoystickGetDeviceInstanceID(i) == removed) { CHECK(SDL_JoystickDetachVirtual(i) == 0); break; }
    Sleep(3); CHECK(host_gamepad_get_state(first_port, &state) == ERROR_DEVICE_NOT_CONNECTED);
    CHECK(!state.Gamepad.wButtons && !state.Gamepad.sThumbLX);
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_DEVICE_NOT_CONNECTED && !pressed);
    CHECK(host_gamepad_get_state(second_port, &state) == ERROR_SUCCESS);
    CHECK(attach_controller(0x05C4, "DogGame reconnected virtual DualShock 4") >= 0);
    Sleep(1100); CHECK(host_gamepad_get_state(first_port, &state) == ERROR_SUCCESS);
    CHECK(host_gamepad_read_menu(first_port, &debug_menu, &state, &pressed) == ERROR_SUCCESS && !pressed);
    CHECK(host_gamepad_get_state(4, &state) == ERROR_DEVICE_NOT_CONNECTED);
    CHECK(host_gamepad_get_state(0, NULL) == ERROR_DEVICE_NOT_CONNECTED);
    SDL_JoystickClose(second);
    puts("Passed: virtual PS4/PS5 mapping, axes, triggers, packets, rumble, guest conversion, capture, independent menu tap retention, disconnect/reconnect and stable ports.");
    return 0;
}
