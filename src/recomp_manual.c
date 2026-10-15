#include "api/title_guest_diagnostics.h"
/* Manual dispatch runs before generated dispatch. Keep guest addresses here
 * in sync with the persistent overrides and the supported XBE revision.
 * Unresolved indirect calls are logged with the runtime's recent call history. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <windows.h>
#include "xbox_memory_layout.h"
extern void sub_00018320(void);
extern void sub_0001DCC0(void);
extern void sub_0001D970(void);
extern void sub_00028CB0(void);
extern void sub_0002AD70(void);

/* ICALL trace ring buffer. */

/*
 * These globals are written by the RECOMP_ICALL macro (defined in
 * recomp_types.h) every time an indirect call is dispatched. When a
 * crash occurs, the VEH handler or recomp_icall_fail_log() can dump
 * the last 16 call targets to help you trace what happened.
 *
 * The runtime owns them: xbox_kernel defines all three in
 * src/kernel/xbox_memory_layout.c, and recomp_types.h declares them extern.
 * Declare, do not define -- a definition here as well is a duplicate symbol,
 * and a project copied from this template failed to link on all three:
 *
 *   xbox_memory_layout.obj : error LNK2005: g_icall_count already defined
 *                            in recomp_manual.obj
 */
extern volatile uint32_t g_icall_trace[16];
extern volatile uint32_t g_icall_trace_idx;
extern volatile uint64_t g_icall_count;

typedef void (*recomp_func_t)(void);

/* Register state (defined in xbox_memory_layout.c). */

extern RECOMP_TLS uint32_t g_eax;
extern RECOMP_TLS uint32_t g_esp;
extern ptrdiff_t g_xbox_mem_offset;

extern void sub_000AD9F0(void);
extern void sub_000991C0(void);
extern void sub_00098330(void);
extern void sub_00011740(void);
extern void sub_00041E20(void);
extern void sub_00024BF0(void);

/* Manual function overrides. */

/*
 * Return a function pointer to override the given Xbox VA, or NULL
 * to fall through to the auto-generated dispatch table.
 *
 * This is called on every indirect call (RECOMP_ICALL) and every
 * direct call through the dispatch table, so keep it fast. A chain
 * of if-statements on uint32_t compiles to a simple comparison
 * sequence; for large override tables, consider a sorted array
 * with binary search.
 *
 */
#include "title_display.h"
extern void sub_000E9E00(void);
extern void sub_000EA2C0(void);
extern void sub_000525B0(void);
extern void sub_0007C490(void);
extern void sub_0008CE00(void);
extern void sub_00053960(void);
extern void sub_0004DFD0(void);

static void native_display_event(void)
{
    const uint32_t *stack = (const uint32_t *)((uintptr_t)g_esp + g_xbox_mem_offset);
    if (title_display_native_event(stack[2], stack[3]))
    {
        g_eax = 1;
        g_esp += 4;
        return;
    }
    sub_0007C490();
}

recomp_func_t recomp_lookup_manual(uint32_t xbox_va)
{
    if (xbox_va == 0x00018320)
    {
        return sub_00018320;
    }
    if (xbox_va == 0x0001DCC0)
    {
        return sub_0001DCC0;
    }
    if (xbox_va == 0x0001D970)
    {
        return sub_0001D970;
    }
    if (xbox_va == 0x00028CB0)
    {
        return sub_00028CB0;
    }
    if (xbox_va == 0x0002AD70)
    {
        return sub_0002AD70;
    }

    if (xbox_va == 0x0008CE00)
    {
        return sub_0008CE00;
    }
    if (xbox_va == 0x0004DFD0)
    {
        return sub_0004DFD0;
    }
    if (xbox_va == 0x00053960)
    {
        return sub_00053960;
    }
    if (xbox_va == 0x000E9E00)
    {
        return sub_000E9E00;
    }
    if (xbox_va == 0x000EA2C0)
    {
        return sub_000EA2C0;
    }
    if (xbox_va == 0x000525B0)
    {
        return sub_000525B0;
    }
    if (xbox_va == 0x0007C490)
    {
        return native_display_event;
    }
    /* Helmet-charge collision callback is selected through EBX, not a direct call. */
    if (xbox_va == 0x00024BF0)
    {
        return getenv("RECOMP_COMBAT_TRACE") ? traced_helmet_contact : sub_00024BF0;
    }

    if (xbox_va == 0x000AD9F0 && getenv("RECOMP_RESOURCE_TRACE"))
    {
        return traced_resource_read;
    }
    if (xbox_va == 0x00041E20 && getenv("RECOMP_EVENT_TRACE"))
    {
        return traced_game_event;
    }
    if (xbox_va == 0x000991C0 && getenv("RECOMP_RESOURCE_TRACE"))
    {
        return traced_asset_read;
    }
    if (xbox_va == 0x00098330 && getenv("RECOMP_RESOURCE_TRACE"))
    {
        return traced_io_poll;
    }
    if (xbox_va == 0x00011740 && getenv("RECOMP_RESOURCE_TRACE"))
    {
        return traced_texture_convert;
    }
    return (recomp_func_t)0;
}

/* ICALL failure logging. */

/*
 * Called when RECOMP_ICALL cannot resolve a target address.
 * This usually means one of:
 *   - A vtable dispatch to an address not in the dispatch table
 *   - A function pointer loaded from uninitialized or corrupt memory
 *   - A kernel thunk address that the bridge doesn't handle
 *
 * During early bring-up you will see many of these. Most are harmless
 * (the ICALL macro pops the dummy return address and continues).
 * Focus on the ones that cause crashes or incorrect behavior.
 */
void recomp_icall_fail_log(uint32_t va)
{
    fprintf(stderr, "[ICALL] Failed to resolve VA 0x%08X (total calls: %llu)\n", va,
            (unsigned long long)g_icall_count);

    /* Dump last 16 call targets from the ring buffer */
    fprintf(stderr, "  Recent ICALL targets:\n");
    for (int i = 0; i < 16; i++)
    {
        int idx = (g_icall_trace_idx - 16 + i) & 15;
        if (g_icall_trace[idx])
        {
            fprintf(stderr, "    [%2d] 0x%08X\n", i, g_icall_trace[idx]);
        }
    }
    fflush(stderr);
}

/* An indirect call whose target is not code: a null or wild function pointer.
 *
 * Skipping these is right -- calling a data address is worse -- but skipping
 * them *silently* is not. They almost always arrive inside a loop, so the
 * symptom is a hang with no output rather than a diagnosable null vtable call.
 *
 * Rate-limited per address: a spin can produce millions of these, and the
 * useful information is which addresses occur, not how often.
 */
void recomp_icall_not_code_log(uint32_t va)
{
    enum
    {
        SLOTS = 16
    };

    static uint32_t seen[SLOTS];
    static uint64_t hits[SLOTS];
    static int count;
    int i;

    for (i = 0; i < count; i++)
    {
        if (seen[i] == va)
        {
            break;
        }
    }
    if (i == count)
    {
        if (count == SLOTS)
        {
            return;
        }
        seen[count] = va;
        hits[count] = 0;
        count++;
    }
    hits[i]++;
    /* Report at 1, 10, 100, 1000 ... rather than once. A single line says a
     * wild pointer was skipped; the progression says it is being skipped in a
     * loop, which is the difference between a curiosity and the reason the
     * title is hung. */
    {
        uint64_t n = hits[i];
        while (n >= 10 && n % 10 == 0)
        {
            n /= 10;
        }
        if (n != 1)
        {
            return;
        }
    }
    fprintf(stderr,
            "[ICALL] target 0x%08X is not code -- skipped %llu time(s) "
            "(null or wild function pointer, at call #%llu)\n",
            va, (unsigned long long)hits[i], (unsigned long long)g_icall_count);
    fflush(stderr);
}

/* ── Untranslated instructions ───────────────────────────────────────────
 *
 * The lifter emits RECOMP_UNIMPL(text, va) at every instruction it has no
 * translation for, in place of the bare comment it used to leave. The
 * instruction is still a no-op; this only stops the omission being silent.
 * RECOMP_UNIMPL_TRAP=1 aborts at the first hit, at the guest address of the
 * cause rather than wherever the damage surfaces. */
#include <stdlib.h>

void recomp_unimpl(const char *text, uint32_t va)
{
    static int printed;
    const char *trap = getenv("RECOMP_UNIMPL_TRAP");
    int stop = trap && *trap && *trap != '0';

    if (printed < 50 || stop)
    {
        printed++;
        fprintf(stderr,
                "[UNIMPL] untranslated instruction REACHED: `%s` at 0x%08X"
                " (a no-op; set RECOMP_UNIMPL_TRAP=1 to stop here)\n",
                text, va);
        fflush(stderr);
    }
    if (stop)
    {
        abort();
    }
}
