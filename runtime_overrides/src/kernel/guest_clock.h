#ifndef DOGGAME_GUEST_CLOCK_H
#define DOGGAME_GUEST_CLOCK_H

#include <stdint.h>

/* Shared, monotonic host-uptime milliseconds for guest clock exports. The
 * initialization baseline retains the old boot-relative epoch. Windows uses
 * QPC elapsed time so the guest's millisecond tick is not quantized into the
 * host GetTickCount clock's larger steps. Thread-safe; no guest memory writes.
 * Publishers explicitly truncate to 32 bits for KeTickCount's wrap contract. */
uint64_t xbox_GuestMilliseconds(void);

#endif
