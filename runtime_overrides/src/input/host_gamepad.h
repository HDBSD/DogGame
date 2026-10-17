#ifndef DOGGAME_HOST_GAMEPAD_H
#define DOGGAME_HOST_GAMEPAD_H

#include <windows.h>
#include <xinput.h>

/* Host-only adapter. Calls serialize internally; no guest memory is accessed.
 * SDL owns controller handles until disconnect/process shutdown. Ports remain
 * stable while attached. Failure clears output state and reports disconnected.
 * Xbox-compatible state keeps keyboard, overlays and guest conversion separate. */
DWORD host_gamepad_get_state(DWORD port, XINPUT_STATE *state);
DWORD host_gamepad_set_state(DWORD port, const XINPUT_VIBRATION *vibration);
DWORD host_gamepad_get_capabilities(DWORD port, DWORD flags, XINPUT_CAPABILITIES *caps);

#endif
