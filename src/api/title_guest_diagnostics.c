/* Manual dispatch runs before generated dispatch. Keep guest addresses here
 * in sync with the persistent overrides and the supported XBE revision.
 * Unresolved indirect calls are logged with the runtime's recent call history. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <windows.h>
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

extern __declspec(thread) uint32_t g_eax;
extern __declspec(thread) uint32_t g_esp;
extern ptrdiff_t g_xbox_mem_offset;

extern void sub_000AD9F0(void);
extern void sub_000991C0(void);
extern void sub_00098330(void);
extern void sub_00011740(void);
extern void sub_00041E20(void);
extern void sub_00024BF0(void);

#include "title_guest_diagnostics.h"

void traced_helmet_contact(void)
{
    const uint8_t *mem = (const uint8_t *)g_xbox_mem_offset;
    const uint32_t *stack = (const uint32_t *)(mem + g_esp);
    uint32_t contact = stack[2], target = *(const uint32_t *)(mem + contact + 8);
    uint32_t before = *(const uint32_t *)(mem + 0x1BF05C);
    sub_00024BF0();
    fprintf(stderr, "[COMBAT] helmet contact=%08X target=%08X type=%u hit=%u->%u\n", contact, target,
            mem[target + 0x15], before, *(const uint32_t *)(mem + 0x1BF05C));
}

void traced_game_event(void)
{
    const uint32_t *stack = (const uint32_t *)(g_xbox_mem_offset + g_esp);
    uint32_t saved_esp = g_esp, actor = stack[1], event = stack[2];
    uint32_t command = 0xFFFFFFFF;
    if (event >= 0x10000 && event < 0x04000000)
    {
        command = *(const uint32_t *)(g_xbox_mem_offset + event);
    }
    sub_00041E20();
    fprintf(stderr, "[EVENT] actor=%08X event=%08X command=%u result=%u stack=%08X->%08X\n", actor, event,
            command, g_eax, saved_esp, g_esp);
}

void traced_io_poll(void)
{
    sub_00098330();
    if (g_eax)
    {
        uint32_t layer = *(const uint32_t *)(g_xbox_mem_offset + 0x1F386C);
        const uint32_t *p = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)layer);
        fprintf(stderr,
                "[ASSET] I/O completion status=%u layer=%08X type=%u count=%u flags=%08X buffer=%08X\n",
                g_eax, layer, p[0], p[2], p[5], p[9]);
    }
}

void traced_texture_convert(void)
{
    const uint32_t *args = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)g_esp);
    uint32_t id = args[2], raw = args[3], size = args[4], outsize = args[5];
    if (id == 0x16B8B5C9 || id == 0xFE7EB229)
    {
        fprintf(stderr, "[ASSET] convert begin id=%08X raw=%08X size=%u esp=%08X\n", id, raw, size, g_esp);
    }
    sub_00011740();
    if (id == 0x16B8B5C9 || id == 0xFE7EB229)
    {
        fprintf(stderr, "[ASSET] convert end id=%08X raw=%08X size=%u result=%08X outsize=%u esp=%08X\n", id,
                raw, size, g_eax, *(const uint32_t *)(g_xbox_mem_offset + (uintptr_t)outsize), g_esp);
    }
}

void traced_asset_read(void)
{
    const uint32_t *args = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)g_esp);
    uint32_t pkg = args[1], id = args[2];
    sub_000991C0();
    if (g_eax && id == 0xFE7EB229)
    {
        const uint32_t *list = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)pkg + 0xB0);
        const uint32_t *nodes = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)list[0]);
        unsigned i, n = list[1];
        fprintf(stderr, "[ASSET] pkg=%08X id=%08X result=%08X count=%u\n", pkg, id, g_eax, n);
        for (i = 0; i < n && i < 20000; i++)
        {
            const uint32_t *node = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)nodes[i]);
            if (node[0] == id)
            {
                unsigned j;
                fprintf(stderr, "[ASSET] node=%08X", nodes[i]);
                for (j = 0; j < 16; j++)
                {
                    fprintf(stderr, " %08X", node[j]);
                }
                fprintf(stderr, "\n");
                break;
            }
        }
    }
}

void traced_resource_read(void)
{
    sub_000AD9F0();
    if (g_eax && getenv("RECOMP_RESOURCE_TRACE"))
    {
        const uint32_t *entry = (const uint32_t *)(g_xbox_mem_offset + (uintptr_t)g_eax);
        fprintf(stderr, "[RESOURCE] read result=%08X header=%08X %08X %08X %08X name=%.32s\n", g_eax,
                entry[0], entry[1], entry[2], entry[3], (const char *)(entry + 4));
    }
}

void title_guest_log_observed_state(void)
{
    const volatile uint8_t *mem = (const volatile uint8_t *)g_xbox_mem_offset;
    float dt = *(const volatile float *)(mem + 0x1A4F14);
    fprintf(stderr, "[STATE] t=%lu mode=%u sub=%u dt=%.6f menu=%u delay=%.6f pad=%08X/%08X/%08X rawA=%u\n",
            GetTickCount(), *(const volatile uint32_t *)(mem + 0x1B8D9C),
            *(const volatile uint32_t *)(mem + 0x1B8DA0), dt, *(const volatile uint32_t *)(mem + 0x1C206C),
            *(const volatile float *)(mem + 0x1C2068), *(const volatile uint32_t *)(mem + 0x1C5544),
            *(const volatile uint32_t *)(mem + 0x1C5548), *(const volatile uint32_t *)(mem + 0x1C554C),
            mem[0x20A31E]);
    if (*(const volatile uint32_t *)(mem + 0x1B8D9C) == 6)
    {
        fprintf(stderr, "[SAVE] dt=%.6f cooldown=%.6f state=%08X selection=%08X\n",
                *(const volatile float *)(mem + 0x1A5220), *(const volatile float *)(mem + 0x1A522C),
                *(const volatile uint32_t *)(mem + 0x1C2494), *(const volatile uint32_t *)(mem + 0x1A5228));
    }
    fflush(stderr);
}
