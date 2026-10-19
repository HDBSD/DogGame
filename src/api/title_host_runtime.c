/* Runtime defaults and the recovered D3D device layout. Explicit user
 * environment settings retain precedence. Recovery references live in
 * REVERSE_ENGINEERING.md. */
#include "title_host_runtime.h"
#include <xbox/xboxrecomp.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct MCPXAPUState MCPXAPUState;
extern MCPXAPUState *g_apu_state;
extern MCPXAPUState *mcpx_apu_init_standalone(uint8_t *ram);

typedef struct RuntimeEnvironmentDefault
{
    const char *name;
    const char *value;
} RuntimeEnvironmentDefault;

void title_host_configure_runtime(void)
{
    static const RuntimeEnvironmentDefault defaults[] = {
        /* This title runs its linked XDK device code against the runtime models. */
        {"RECOMP_AC97_READY", "1"},
        {"RECOMP_GP_COMMAND_ACK", "0x810"},
        {"RECOMP_CONTIG_AFTER_IMAGE", "1"},
        {"RECOMP_CONTIG_RECLAIM", "1"},
        /* Level transitions require split/coalesced reuse and releasing virtual
         * reservations. Keeping old whole-block reuse exhausts retail RAM. */
        {"RECOMP_HEAP_RECLAIM", "1"},
        {"RECOMP_PB_EXEC", "1"},
        /* Hardware rasterization is verified in the title's gameplay scenes.
         * Explicit RECOMP_GPU=0 retains the software renderer for diagnostics. */
        {"RECOMP_GPU", "1"},
        {"RECOMP_TITLE_KEVENTS", "1"},
        {"RECOMP_VBLANK", "1"},
        {"RECOMP_FB_WINDOW", "1"},
        {"RECOMP_PVIDEO_OVERLAY", "1"},
        {"RECOMP_USB", "1"},
        /* This XDK maps root port indexes to Xbox slots with index XOR 2.
         * Root port 3 therefore exposes the virtual pad as player one. */
        {"RECOMP_USB_NDP", "4"},
        {"RECOMP_USB_PORT", "2"},
    };
    for (unsigned index = 0; index < sizeof defaults / sizeof defaults[0]; index++)
    {
        const RuntimeEnvironmentDefault *setting = &defaults[index];
        if (!getenv(setting->name))
        {
            _putenv_s(setting->name, setting->value);
        }
    }
}

void title_host_initialize_devices(void)
{
    /* This title's D3D fence waiter at 0x001045A0 reads the completion
     * word through device+0x30 and compares it with device+0x2C. Register
     * that layout with the runtime's GPU completion acknowledgement. */
    xbox_Nv2aMirrorFence(0x0010E648u, 0x2Cu, 0x30u);
    /* This D3D build keeps the ring's initial write base at device+0x24.
     * The first PUT is an end pointer; starting there loses GPU setup. */
    xbox_Nv2aRingBase(0x0010E648u, 0x24u);
    xbox_Nv2aCompletionEvent(0x0010E648u, 0x1DBCu);
    /* SD100F's linked refresh path advances device+1DD8 at 000FD227-000FD23A.
     * The runtime's synthetic timer/flip writer advanced the same word again.
     * FMVs then waited and caught up in bursts (up to 359 ms in cin1). With
     * the native writer alone, the probe presented 30 images/s without skips.
     * Retain the old fallback solely as an explicit diagnostic opt-in. */
    if (getenv("RECOMP_SYNTHETIC_FRAMECOUNTER") && !getenv("RECOMP_TITLE_FRAMECOUNTER"))
    {
        xbox_Nv2aFrameCounter(0x0010E648u, 0x1DD8u);
    }
    xbox_Nv2aExplicitVblankAck();

    /* Bring up the emulated APU.
     *
     * Gated on the same variable that unmaps its registers, because the two
     * halves are useless apart: a trap with no model declines every access,
     * and a model nothing traps into never sees a register. The APU walks
     * Xbox physical RAM to find voice buffers, so it gets the guest RAM
     * base. */
    if (getenv("RECOMP_AC97_READY"))
    {
        g_apu_state = mcpx_apu_init_standalone((uint8_t *)xbox_GetMemoryBase());
        fprintf(stderr, "[BOOT] emulated APU %s\n", g_apu_state ? "up" : "FAILED to initialise");
    }
}
