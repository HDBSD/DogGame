#ifndef DOGGAME_HOST_GAMEPAD_PUBLIC_H
#define DOGGAME_HOST_GAMEPAD_PUBLIC_H

#include <windows.h>
#include <xinput.h>
#include <stdint.h>

/* Host-only adapter. Calls serialize internally; no guest memory is accessed.
 * SDL owns controller handles until disconnect/process shutdown. Ports remain
 * stable while attached. Failure clears output state and reports disconnected.
 * Xbox-compatible state keeps keyboard, overlays and guest conversion separate. */
DWORD host_gamepad_get_state(DWORD port, XINPUT_STATE *state);
DWORD host_gamepad_set_state(DWORD port, const XINPUT_VIBRATION *vibration);
DWORD host_gamepad_get_capabilities(DWORD port, DWORD flags, XINPUT_CAPABILITIES *caps);

/* One zero-initialized cursor per port/consumer, owned by its UI thread.
 * A read returns current held state and the next queued press, in order.
 * Drain until pressed is zero. Other readers and guest polling do not consume
 * this cursor's events. A 128-entry ring bounds history; on overflow the oldest
 * events are discarded with a diagnostic. Reconnection discards old history.
 * Reads use a published snapshot, never wait for SDL/device scanning, and do
 * not stretch guest input. The 4 ms sampler remains subject to OS scheduling. */
typedef struct HostGamepadMenuReader
{
    uint64_t next_press;
    unsigned generation;
} HostGamepadMenuReader;

DWORD host_gamepad_read_menu(DWORD port, HostGamepadMenuReader *reader,
                             XINPUT_STATE *state, WORD *pressed);

#endif
