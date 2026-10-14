/* Persistent title overrides: save/menu timing, vblank acknowledgement,
 * pause camera clear, and title debugging/startup shortcuts. Exclude this file from regeneration with
 * --exclude-manual. */
/* These functions retain the Xbox calling convention: arguments live on
 * the guest stack and results in guest registers. loc_ labels identify XBE
 * addresses for disassembly lookup. Flag temporaries preserve x86 branches;
 * the host hooks are placed between the original instruction sequences. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "gen/recomp_funcs.h"
#include "title_guest_routines.h"
#include "save_dialogue.h"
#include "debug_options.h"
#include "title_display.h"
#include <xbox/display_options.h>
#include <math.h>

void title_guest_run_save_dialogue_frame(void)
{
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    double float_compare_left = 0.0, float_compare_right = 0.0;
    (void)float_compare_left;
    (void)float_compare_right;
#define fp_push(v)                                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        double _fp_value = (v);                                                                              \
        g_fp_top = (g_fp_top + 7u) & 7u;                                                                     \
        g_fp_stack[g_fp_top] = _fp_value;                                                                    \
    } while (0)
#define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
#define fp_st1() fp_st(1)

loc_000740D0:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    PUSH32(esp, 0x000740D8u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_000740D8:;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    MEM32(esp) = eax;
    MEM32(esp + 4) = edx;
    fp_push((double)SMEM64(esp));                       /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x180F08)); /* fmul dword ptr [0x180f08] */
    MEMF(0x1C24A0) = (float)fp_top();
    fp_pop();                               /* fstp */
    xmm2 = XMM_SCALAR(MEMF(0x1C24A0));      /* movss */
    xmm0 = xmm2;                            /* movaps */
    xmm0.f[0] = xmm0.f[0] - MEMF(0x1C249C); /* subss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1A5220) = xmm0.f[0];      /* movss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0007411A; /* ja: above (unsigned >) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_00074111:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180DAC); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_00074125; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x180DAC)) */
    }

loc_0007411A:;
    /* Keep the save-screen cooldown advancing when software rendering takes
     * longer than 100 ms. Negative/nonfinite deltas still become zero. */
    xmm0.f[0] = save_dialogue_delta(MEMF(0x1A5220), MEMF(0x180DAC));
    MEMF(0x1A5220) = xmm0.f[0];

loc_00074125:;
    xmm1 = XMM_SCALAR(MEMF(0x1A522C)); /* movss */
    eax = MEM32(0x1C24B0);
    ecx = MEM32(0x1C24B4);
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    MEMF(0x1A522C) = xmm1.f[0];        /* movss */
    MEMF(0x1C249C) = xmm2.f[0];        /* movss */
    MEM32(0x1C24A8) = eax;
    MEM32(0x1C24AC) = ecx;
    PUSH32(esp, 0x0007415Cu);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0007415C:;
    MEM32(0x1C24B0) = eax;
    eax = (uint32_t)(int32_t)SMEM8(0x1C2028);
    MEM32(0x1C24B4) = edx;
    edx = MEM32(0x1A5220);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0007417Bu);
    RECOMP_ABI_CALL(0x00090F50u, sub_00090F50); /* call 0x00090F50 */

loc_0007417B:;
    /* Load/Save run this nested render/input loop while the outer menu and
     * gameplay loops are suspended. Publish and consume mouse targets here. */
    title_display_frame(); /* title_menu_event preserves guest registers */
    PUSH32(esp, 0x00074180u);
    RECOMP_ABI_CALL(0x00085A80u, sub_00085A80); /* call 0x00085A80 */

loc_00074180:;
    ecx = MEM32(0x1A5220);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0007418Cu);
    RECOMP_ABI_CALL(0x00091820u, sub_00091820); /* call 0x00091820 */

loc_0007418C:;
    edx = MEM32(0x1A5220);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00074198u);
    RECOMP_ABI_CALL(0x000791B0u, sub_000791B0); /* call 0x000791B0 */

loc_00074198:;
    PUSH32(esp, 0x41C80000);
    PUSH32(esp, 0x1BEE10);
    PUSH32(esp, 0x1BEE00);
    PUSH32(esp, 0x1BEDF0);
    PUSH32(esp, 0x1BEDE0);
    PUSH32(esp, 0x000741B6u);
    RECOMP_ABI_CALL(0x00095010u, sub_00095010); /* call 0x00095010 */

loc_000741B6:;
    eax = MEM32(0x1BEF4C);
    ecx = MEM32(eax + 0x20);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    ecx = ecx + 0x30;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x000741C7u);
    RECOMP_ABI_CALL(0x000950C0u, sub_000950C0); /* call 0x000950C0 */

loc_000741C7:;
    PUSH32(esp, 0x000741CCu);
    RECOMP_ABI_CALL(0x00094C60u, sub_00094C60); /* call 0x00094C60 */

loc_000741CC:;
    edx = MEM32(0x1A5220);
    PUSH32(esp, edx);
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x000741DDu);
    RECOMP_ABI_CALL(0x000847A0u, sub_000847A0); /* call 0x000847A0 */

loc_000741DD:;
    PUSH32(esp, 1);
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x000741E9u);
    RECOMP_ABI_CALL(0x00082150u, sub_00082150); /* call 0x00082150 */

loc_000741E9:;
    PUSH32(esp, 0x000741EEu);
    RECOMP_ABI_CALL(0x00077380u, sub_00077380); /* call 0x00077380 */

loc_000741EE:;
    PUSH32(esp, 0x000741F3u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_000741F3:;
    PUSH32(esp, 0x000741F8u);
    RECOMP_ABI_CALL(0x00085A80u, sub_00085A80); /* call 0x00085A80 */

loc_000741F8:;
    eax = MEM32(0x1A5220);
    PUSH32(esp, 1);
    PUSH32(esp, eax);
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x0007420Au);
    RECOMP_ABI_CALL(0x00082800u, sub_00082800); /* call 0x00082800 */

loc_0007420A:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x44;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x00074212u);
    RECOMP_ABI_CALL(0x000A2EA0u, sub_000A2EA0); /* call 0x000A2EA0 */

loc_00074212:;
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x0007421Cu);
    RECOMP_ABI_CALL(0x00082830u, sub_00082830); /* call 0x00082830 */

loc_0007421C:;
    MEM32(0x1C3934) = MEM32(0x1C3934) + 1;
    flag_result = (uint32_t)(MEM32(0x1C3934)) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}

/* Refresh interrupt: retain original handler, acknowledge hardware status explicitly. */
#include "kernel/xbox_memory_layout.h"

void title_guest_handle_refresh_interrupt(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00109530:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = ecx;
    PUSH32(esp, edi);
    edi = MEM32(esi);
    PUSH32(esp, 0x00109540u);
    RECOMP_ABI_CALL(0x00109320u, sub_00109320); /* call 0x00109320 */

loc_00109540:;
    ecx = MEM32(esi + 0x1D8);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00109554; /* je: equal / zero */
    }

loc_0010954A:;
    edx = eax;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    edx = edx - ecx;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    MEM32(esi + 0x1D4) = edx;

loc_00109554:;
    ebx = MEM32(esi + 0x1C0);
    ebx++;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esi + 0x1D8) = eax;
    MEM32(esi + 0x1C0) = ebx;
    SET_LO8(ebx, MEM8(edi + 0x6013D4));
    ecx = esi;
    ebp = 0; /* xor self */
    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    PUSH32(esp, 0x00109576u);
    RECOMP_ABI_CALL(0x00109450u, sub_00109450); /* call 0x00109450 */

loc_00109576:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    ecx = 1;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00109583; /* je: equal / zero */
    }

loc_0010957F:;
    ebp = ecx;
    goto loc_0010959D;

loc_00109583:;
    eax = MEM32(esi + 0x1C4);
    flag_result = (uint32_t)(MEM32(esi + 0x1C0)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(esi + 0x1C0), eax (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0010959D; /* jne: not equal / not zero */
    }

loc_00109591:;
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esi + 0x1C4) = eax;
    ebp = 2;

loc_0010959D:;
    flag_result = (uint32_t)(MEM32(esi + 0x1B4)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x1000000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(esi + 0x1B4), 0x1000000 (32-bit) */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_001095D9; /* jne: not equal / not zero */
    }

loc_001095A9:;
    SET_LO16(edx, 0x80C0);
    SET_LO8(eax, xbox_ReadIoPort(LO16(edx), 1));
    edx = MEM32(esi + 8);
    edx = edx & 0xC0000000u;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    eax = eax >> 5;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax = ~eax;
    eax = eax & ecx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x40000000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0x40000000 (32-bit) */
    MEM32(esi + 0x1D0) = eax;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_001095D9; /* jne: not equal / not zero */
    }

loc_001095CC:;
    edx = 0; /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand);    /* cmp eax, 0 (32-bit) */
    SET_LO8(edx, (CMP_EQ(flag_result, flag_operand)) ? 1 : 0); /* sete */
    MEM32(esi + 0x1D0) = edx;

loc_001095D9:;
    eax = 0x1000000;
    edi = edi;

loc_001095E0:;
    xbox_Nv2aAcknowledgeVblank(ecx); /* PCRTC_INTR_0 is write-one-to-clear. */
    flag_result = (uint32_t)(MEM32(edi + 0x100)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(edi + 0x100), eax (32-bit) */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_001095E0; /* jne: not equal / not zero */
    }

loc_001095EE:;
    {
        uint32_t _icall_esp = g_esp;
        PUSH32(esp, 0);
        PUSH32(esp, ecx);
        eax = esi + 0x194;
        PUSH32(esp, eax);
        {
            uint32_t _icall_target = MEM32(0x1807DC);
            PUSH32(esp, 0x001095FEu);
            RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001095F8u);
        } /* indirect call */
    }

loc_001095FE:;
    eax = MEM32(esi + 0x190);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0010962A; /* je: equal / zero */
    }

loc_00109608:;
    ecx = MEM32(esi + 0x1C0);
    edx = MEM32(esi + 0x1BC);
    MEM32(esp + 0x10) = ecx;
    ecx = esp + 0x10;
    {
        uint32_t _icall_esp = g_esp;
        PUSH32(esp, ecx);
        MEM32(esp + 0x18) = edx;
        MEM32(esp + 0x1C) = ebp;
        {
            uint32_t _icall_target = eax;
            PUSH32(esp, 0x00109627u);
            RECOMP_ICALL_SAFE_AT_CC(_icall_target, _icall_esp, 0x00109625u);
        } /* indirect call */
    }

loc_00109627:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0010962A:;
    MEM8(edi + 0x6013D4) = LO8(ebx);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    POP32(esp, ebx);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */
}

/* Original menu frame loop with bounded elapsed time. */
void title_guest_run_main_menu(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* prologue saves caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    double float_compare_left = 0.0, float_compare_right = 0.0;
    (void)float_compare_left;
    (void)float_compare_right;
#define fp_push(v)                                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        double _fp_value = (v);                                                                              \
        g_fp_top = (g_fp_top + 7u) & 7u;                                                                     \
        g_fp_stack[g_fp_top] = _fp_value;                                                                    \
    } while (0)
#define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
#define fp_st1() fp_st(1)

loc_0006CEB0:;
    if (title_debug_boot())
    {
        esp += 4;
        return;
    }
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF8u;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 0x30;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    eax = MEM32(0x1A4F0C);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebx = 0; /* xor self */
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebx (32-bit) */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006CECC; /* je: equal / zero */
    }

loc_0006CEC8:;
    PUSH32(esp, 1);
    goto loc_0006CECE;

loc_0006CECC:;
    PUSH32(esp, 2);

loc_0006CECE:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CED3u);
    RECOMP_ABI_CALL(0x0005F3A0u, sub_0005F3A0); /* call 0x0005F3A0 */

loc_0006CED3:;
    PUSH32(esp, ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CED9u);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0006CED9:;
    PUSH32(esp, ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CEDFu);
    RECOMP_ABI_CALL(0x0006E590u, sub_0006E590); /* call 0x0006E590 */

loc_0006CEDF:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    g_ebp = ebp;                                          /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CEE7u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0006CEE7:;
    MEM32(esp + 0x20) = eax;
    MEM32(esp + 0x24) = edx;
    fp_push((double)SMEM64(esp + 0x20)); /* fild */
    PUSH32(esp, ebx);
    PUSH32(esp, 1);
    PUSH32(esp, ebx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x180F08)); /* fmul dword ptr [0x180f08] */
    PUSH32(esp, ebx);
    eax = esp + 0x28;
    PUSH32(esp, eax);
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x18D0A8)); /* fsub dword ptr [0x18d0a8] */
    ecx = esp + 0x28;
    PUSH32(esp, ecx);
    MEM8(esp + 0x2C) = LO8(ebx);
    MEMF(0x1C204C) = (float)fp_top();
    fp_pop(); /* fstp */
    MEM8(esp + 0x2D) = LO8(ebx);
    MEM8(esp + 0x2E) = LO8(ebx);
    MEM8(esp + 0x2F) = 0xFF;
    MEM8(esp + 0x30) = LO8(ebx);
    MEM8(esp + 0x31) = LO8(ebx);
    MEM8(esp + 0x32) = LO8(ebx);
    MEM8(esp + 0x33) = LO8(ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CF3Au);
    RECOMP_ABI_CALL(0x00093190u, sub_00093190); /* call 0x00093190 */

loc_0006CF3A:;
    flag_operand = (uint32_t)(0x18) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x18;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEM32(esp + 0x1C) = 0xA;
    ebp = 0xB;
    /* nop */

loc_0006CF50:;
    edi = 0; /* xor self */
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    g_ebp = ebp;                                          /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CF57u);
    RECOMP_ABI_CALL(0x0005F2C0u, sub_0005F2C0); /* call 0x0005F2C0 */

loc_0006CF57:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(1) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 1 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D012; /* jne: not equal / not zero */
    }

loc_0006CF60:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CF65u);
    RECOMP_ABI_CALL(0x0005F2B0u, sub_0005F2B0); /* call 0x0005F2B0 */

loc_0006CF65:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebx (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006CFF0; /* je: equal / zero */
    }

loc_0006CF6D:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(3) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 3 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D012; /* jne: not equal / not zero */
    }

loc_0006CF76:;
    PUSH32(esp, ebx);
    PUSH32(esp, 5);
    edi = 1;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CF83u);
    RECOMP_ABI_CALL(0x0005B700u, sub_0005B700); /* call 0x0005B700 */

loc_0006CF83:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CF8Cu);
    RECOMP_ABI_CALL(0x0005B6E0u, sub_0005B6E0); /* call 0x0005B6E0 */

loc_0006CF8C:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ebx);
    PUSH32(esp, 4);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CF97u);
    RECOMP_ABI_CALL(0x0005B700u, sub_0005B700); /* call 0x0005B700 */

loc_0006CF97:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFA0u);
    RECOMP_ABI_CALL(0x0005B6E0u, sub_0005B6E0); /* call 0x0005B6E0 */

loc_0006CFA0:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFAAu);
    RECOMP_ABI_CALL(0x0005B700u, sub_0005B700); /* call 0x0005B700 */

loc_0006CFAA:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFB3u);
    RECOMP_ABI_CALL(0x0005B6E0u, sub_0005B6E0); /* call 0x0005B6E0 */

loc_0006CFB3:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ebx);
    PUSH32(esp, 0xD);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFBEu);
    RECOMP_ABI_CALL(0x0005B700u, sub_0005B700); /* call 0x0005B700 */

loc_0006CFBE:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFC7u);
    RECOMP_ABI_CALL(0x0005B6E0u, sub_0005B6E0); /* call 0x0005B6E0 */

loc_0006CFC7:;
    PUSH32(esp, 2);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFCEu);
    RECOMP_ABI_CALL(0x0005F3A0u, sub_0005F3A0); /* call 0x0005F3A0 */

loc_0006CFCE:;
    PUSH32(esp, edi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFD4u);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0006CFD4:;
    PUSH32(esp, ebx);
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, 0x3DCCCCCD);
    edx = esp + 0x38;
    PUSH32(esp, edx);
    eax = esp + 0x38;
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFEBu);
    RECOMP_ABI_CALL(0x00093190u, sub_00093190); /* call 0x00093190 */

loc_0006CFEB:;
    flag_operand = (uint32_t)(0x28) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x28;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    goto loc_0006D012;

loc_0006CFF0:;
    PUSH32(esp, 3);
    edi = 1;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006CFFCu);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0006CFFC:;
    PUSH32(esp, ebx);
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebx);
    ecx = esp + 0x28;
    PUSH32(esp, ecx);
    edx = esp + 0x30;
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D00Fu);
    RECOMP_ABI_CALL(0x00093190u, sub_00093190); /* call 0x00093190 */

loc_0006D00F:;
    flag_operand = (uint32_t)(0x1C) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x1C;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0006D012:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D017u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0006D017:;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    MEM32(esp + 0x28) = eax;
    MEM32(esp + 0x2C) = edx;
    fp_push((double)SMEM64(esp + 0x28));                /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x180F08)); /* fmul dword ptr [0x180f08] */
    MEMF(0x1C2050) = (float)fp_top();
    fp_pop();                               /* fstp */
    xmm0 = XMM_SCALAR(MEMF(0x1C2050));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(0x1C204C); /* subss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1A4F14) = xmm0.f[0];      /* movss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0006D058; /* ja: above (unsigned >) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_0006D04F:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180DAC); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0006D060; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x180DAC)) */
    }

loc_0006D058:;
    /* Slow software-rendered frames must still advance menu timers. */
    MEMF(0x1A4F14) = save_dialogue_delta(MEMF(0x1A4F14), MEMF(0x180DAC));

loc_0006D060:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D065u);
    RECOMP_ABI_CALL(0x0005F2C0u, sub_0005F2C0); /* call 0x0005F2C0 */

loc_0006D065:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 2 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D1A9; /* jne: not equal / not zero */
    }

loc_0006D06E:;
    xmm0 = XMM_SCALAR(MEMF(0x1C2068));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(0x1A4F14); /* subss */
    xmm1 = XMM_ZERO();                      /* xorps self = zero */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1C2068) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0006D0D2; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_0006D08E:;
    PUSH32(esp, 0x18E050);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D098u);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0006D098:;
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D09Eu);
    RECOMP_ABI_CALL(0x00077EA0u, sub_00077EA0); /* call 0x00077EA0 */

loc_0006D09E:;
    PUSH32(esp, 0x18E03C);
    esi = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D0AAu);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0006D0AA:;
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D0B0u);
    RECOMP_ABI_CALL(0x00077EA0u, sub_00077EA0); /* call 0x00077EA0 */

loc_0006D0B0:;
    SET_LO8(ecx, MEM8(esi + 6));
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(ecx), 1 (8-bit) */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0006D0C8; /* jne: not equal / not zero */
    }

loc_0006D0BB:;
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D0C1u);
    RECOMP_ABI_CALL(0x00087CB0u, sub_00087CB0); /* call 0x00087CB0 */

loc_0006D0C1:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006D0D2; /* je: equal / zero */
    }

loc_0006D0C8:;
    PUSH32(esp, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D0CFu);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0006D0CF:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0006D0D2:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D0D7u);
    RECOMP_ABI_CALL(0x0005F2B0u, sub_0005F2B0); /* call 0x0005F2B0 */

loc_0006D0D7:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(1) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 1 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D176; /* jne: not equal / not zero */
    }

loc_0006D0E0:;
    flag_result = (uint32_t)(MEM32(0x1A4F0C)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1A4F0C), ebx (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006D10A; /* je: equal / zero */
    }

loc_0006D0E8:;
    PUSH32(esp, ebx);
    PUSH32(esp, 1);
    PUSH32(esp, 0x3DCCCCCD);
    PUSH32(esp, 0xFFFFFFFFu);
    PUSH32(esp, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D0F9u);
    RECOMP_ABI_CALL(0x0005B700u, sub_0005B700); /* call 0x0005B700 */

loc_0006D0F9:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D102u);
    RECOMP_ABI_CALL(0x0006CE20u, sub_0006CE20); /* call 0x0006CE20 */

loc_0006D102:;
    MEM32(0x1A4F0C) = ebx;
    goto loc_0006D15B;

loc_0006D10A:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D10Fu);
    RECOMP_ABI_CALL(0x000A5500u, sub_000A5500); /* call 0x000A5500 */

loc_0006D10F:;
    ecx = MEM32(0x1C2070);
    eax = eax & 1;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ecx (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D132; /* jne: not equal / not zero */
    }

loc_0006D11C:;
    /* nop */

loc_0006D120:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D125u);
    RECOMP_ABI_CALL(0x000A5500u, sub_000A5500); /* call 0x000A5500 */

loc_0006D125:;
    ecx = MEM32(0x1C2070);
    eax = eax & 1;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ecx (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006D120; /* je: equal / zero */
    }

loc_0006D132:;
    ecx = 0; /* xor self */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    SET_LO8(ecx, MEM8(0x1C2029));
    MEM32(0x1C2070) = eax;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    eax = eax + 2;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ecx);
    PUSH32(esp, 1);
    PUSH32(esp, 0x3DCCCCCD);
    PUSH32(esp, 0xFFFFFFFFu);
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D152u);
    RECOMP_ABI_CALL(0x0005B700u, sub_0005B700); /* call 0x0005B700 */

loc_0006D152:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D15Bu);
    RECOMP_ABI_CALL(0x0006CE20u, sub_0006CE20); /* call 0x0006CE20 */

loc_0006D15B:;
    flag_operand = (uint32_t)(0x14) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x14;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D164u);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0006D164:;
    xmm0 = XMM_SCALAR(MEMF(0x18BAD4)); /* movss */
    MEMF(0x1C2068) = xmm0.f[0];        /* movss */
    goto loc_0006D182;

loc_0006D176:;
    edx = MEM32(0x1A4F14);
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D182u);
    RECOMP_ABI_CALL(0x0006E680u, sub_0006E680); /* call 0x0006E680 */

loc_0006D182:;
    eax = (uint32_t)(int32_t)SMEM8(0x1C2028);
    eax = (uint32_t)((int32_t)eax * (int32_t)0x68);
    ecx = MEM32(eax + 0x1C5548);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, ebx (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006D1A9; /* je: equal / zero */
    }

loc_0006D199:;
    xmm0 = XMM_SCALAR(MEMF(0x18BAD4)); /* movss */
    MEMF(0x1C2068) = xmm0.f[0];        /* movss */

loc_0006D1A9:;
    ecx = MEM32(0x1A4F14);
    edx = (uint32_t)(int32_t)SMEM8(0x1C2028);
    xmm0 = XMM_SCALAR(MEMF(0x1C2050)); /* movss */
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    MEMF(0x1C204C) = xmm0.f[0]; /* movss */
    g_ebp = ebp;                /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D1CDu);
    RECOMP_ABI_CALL(0x00090F50u, sub_00090F50); /* call 0x00090F50 */

loc_0006D1CD:;
    title_debug_menu_frame();
    SET_LO8(eax, MEM8(0x1C2029));
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(LO8(ebx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D205; /* jne: not equal / not zero */
    }

loc_0006D1D9:;
    eax = MEM32(0x1A4F14);
    PUSH32(esp, eax);
    PUSH32(esp, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D1E6u);
    RECOMP_ABI_CALL(0x00090F50u, sub_00090F50); /* call 0x00090F50 */

loc_0006D1E6:;
    ecx = MEM32(0x1A4F14);
    PUSH32(esp, ecx);
    PUSH32(esp, 2);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D1F4u);
    RECOMP_ABI_CALL(0x00090F50u, sub_00090F50); /* call 0x00090F50 */

loc_0006D1F4:;
    edx = MEM32(0x1A4F14);
    PUSH32(esp, edx);
    PUSH32(esp, 3);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D202u);
    RECOMP_ABI_CALL(0x00090F50u, sub_00090F50); /* call 0x00090F50 */

loc_0006D202:;
    flag_operand = (uint32_t)(0x18) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x18;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0006D205:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D20Au);
    RECOMP_ABI_CALL(0x00085A80u, sub_00085A80); /* call 0x00085A80 */

loc_0006D20A:;
    eax = MEM32(0x1A4F14);
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D215u);
    RECOMP_ABI_CALL(0x000791B0u, sub_000791B0); /* call 0x000791B0 */

loc_0006D215:;
    ecx = MEM32(0x1A4F14);
    edx = MEM32(0x1BF88C);
    {
        uint32_t _icall_esp = g_esp;
        PUSH32(esp, ecx);
        PUSH32(esp, edx);
        PUSH32(esp, 0x1BEF20);
        {
            uint32_t _icall_target = MEM32(0x1BEF50);
            PUSH32(esp, 0x0006D22Eu);
            RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0006D228u);
        } /* indirect call */
    }

loc_0006D22E:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D233u);
    RECOMP_ABI_CALL(0x00094C60u, sub_00094C60); /* call 0x00094C60 */

loc_0006D233:;
    eax = MEM32(0x1A4F14);
    PUSH32(esp, eax);
    PUSH32(esp, 0x1BED20);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D243u);
    RECOMP_ABI_CALL(0x000847A0u, sub_000847A0); /* call 0x000847A0 */

loc_0006D243:;
    PUSH32(esp, 1);
    PUSH32(esp, 0x1BED20);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D24Fu);
    RECOMP_ABI_CALL(0x00082150u, sub_00082150); /* call 0x00082150 */

loc_0006D24F:;
    flag_operand = (uint32_t)(0x20) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x20;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    g_ebp = ebp;                                          /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D257u);
    RECOMP_ABI_CALL(0x00077380u, sub_00077380); /* call 0x00077380 */

loc_0006D257:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D25Cu);
    RECOMP_ABI_CALL(0x0008D090u, sub_0008D090); /* call 0x0008D090 */

loc_0006D25C:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D261u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0006D261:;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, ebx (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006D28E; /* je: equal / zero */
    }

loc_0006D265:;
    PUSH32(esp, ebx);
    PUSH32(esp, 6);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D26Du);
    RECOMP_ABI_CALL(0x000B8160u, sub_000B8160); /* call 0x000B8160 */

loc_0006D26D:;
    PUSH32(esp, ebx);
    PUSH32(esp, 8);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D275u);
    RECOMP_ABI_CALL(0x000B8160u, sub_000B8160); /* call 0x000B8160 */

loc_0006D275:;
    PUSH32(esp, 1);
    PUSH32(esp, 0xA);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D27Eu);
    RECOMP_ABI_CALL(0x000B8160u, sub_000B8160); /* call 0x000B8160 */

loc_0006D27E:;
    PUSH32(esp, 1);
    PUSH32(esp, ebp);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D286u);
    RECOMP_ABI_CALL(0x000B8160u, sub_000B8160); /* call 0x000B8160 */

loc_0006D286:;
    flag_operand = (uint32_t)(0x20) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x20;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    g_ebp = ebp;                                          /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D28Eu);
    RECOMP_ABI_CALL(0x00092C00u, sub_00092C00); /* call 0x00092C00 */

loc_0006D28E:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D293u);
    RECOMP_ABI_CALL(0x00085A80u, sub_00085A80); /* call 0x00085A80 */

loc_0006D293:;
    ecx = MEM32(0x1A4F14);
    PUSH32(esp, 1);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x1BED20);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D2A6u);
    RECOMP_ABI_CALL(0x00082800u, sub_00082800); /* call 0x00082800 */

loc_0006D2A6:;
    PUSH32(esp, 0x1BED20);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D2B0u);
    RECOMP_ABI_CALL(0x00082830u, sub_00082830); /* call 0x00082830 */

loc_0006D2B0:;
    eax = MEM32(0x1B8D9C);
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esi = 0;                                              /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 4 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D305; /* jne: not equal / not zero */
    }

loc_0006D2BF:;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D2C4u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0006D2C4:;
    MEM32(esp + 0x30) = eax;
    MEM32(esp + 0x34) = edx;
    fp_push((double)SMEM64(esp + 0x30));                /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x1C204C)); /* fsub dword ptr [0x1c204c] */
    MEMF(esp + 0x20) = (float)fp_top();
    fp_pop();    /* fstp */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D2DFu);
    RECOMP_ABI_CALL(0x00075D90u, sub_00075D90); /* call 0x00075D90 */

loc_0006D2DF:;
    esi = eax;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x30303030) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0x30303030 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        esi = ebx; /* cmove */
    }
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D2EFu);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0006D2EF:;
    MEM32(esp + 0x38) = eax;
    MEM32(esp + 0x3C) = edx;
    fp_push((double)SMEM64(esp + 0x38));                  /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(esp + 0x20)); /* fadd dword ptr [esp + 0x20] */
    MEMF(0x1C204C) = (float)fp_top();
    fp_pop(); /* fstp */

loc_0006D305:;
    ecx = MEM32(0x1C3934);
    eax = MEM32(esp + 0x1C);
    ecx++;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebx (32-bit) */
    MEM32(0x1C3934) = ecx;
    if (CMP_LE(signed_flag_result, signed_flag_operand))
    {
        goto loc_0006D321; /* jle: less or equal (signed <=) */
    }

loc_0006D31A:;
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x1C) = eax;
    goto loc_0006D354;

loc_0006D321:;
    PUSH32(esp, 2);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D328u);
    RECOMP_ABI_CALL(0x0005F2E0u, sub_0005F2E0); /* call 0x0005F2E0 */

loc_0006D328:;
    eax = MEM32(0x1EDEFC);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 2 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0006D354; /* je: equal / zero */
    }

loc_0006D335:;
    flag_result = (uint32_t)(MEM8(0x1C202B)) & 0xFFu;
    flag_operand = (uint32_t)(LO8(ebx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(0x1C202B), LO8(ebx) (8-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006D354; /* jne: not equal / not zero */
    }

loc_0006D33D:;
    edx = MEM32(0x1EDEF8);
    PUSH32(esp, 1);
    PUSH32(esp, edx);
    MEM8(0x1C2030) = LO8(ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D351u);
    RECOMP_ABI_CALL(0x00097510u, sub_00097510); /* call 0x00097510 */

loc_0006D351:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0006D354:;
    flag_result = (uint32_t)(MEM32(0x1B8D9C)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1B8D9C), ebp (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006CF50; /* jne: not equal / not zero */
    }

loc_0006D360:;
    PUSH32(esp, ebx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0006D366u);
    RECOMP_ABI_CALL(0x0005F2E0u, sub_0005F2E0); /* call 0x0005F2E0 */

loc_0006D366:;
    eax = MEM32(0x1B8D9C);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebp (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0006CF50; /* jne: not equal / not zero */
    }

loc_0006D376:;
    POP32(esp, edi);
    eax = esi;
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4;
    return; /* ret */

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}

/* Title camera clear: preserve the pause background. */
void title_guest_clear_camera_background(void)
{
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    int carry_flag = 0; /* carry flag */

loc_000BDD70:;
    ecx = MEM32(esp + 0xC);
    /* Pause UI follows the world render on the same camera. Keep its color
     * image while resetting depth for text and blended overlay draws. */
    if (MEM32(esp) == 0x00077BCEu && MEM32(0x001B8D9Cu) == 7u)
    {
        ecx &= ~1u; /* rwCAMERACLEARIMAGE; retain rwCAMERACLEARZ */
    }
    edx = MEM32(esp + 8);
    eax = MEM32(0x59C73C);
    {
        uint32_t _icall_esp = g_esp;
        PUSH32(esp, esi);
        esi = MEM32(esp + 8);
        PUSH32(esp, ecx);
        PUSH32(esp, edx);
        PUSH32(esp, esi);
        {
            uint32_t _icall_target = eax;
            PUSH32(esp, 0x000BDD87u);
            RECOMP_ICALL_SAFE_AT_CC(_icall_target, _icall_esp, 0x000BDD85u);
        } /* indirect call */
    }

loc_000BDD87:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    carry_flag = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* neg result */
    eax = carry_flag ? 0xFFFFFFFF : 0;                    /* sbb self (CF extend) */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sbb result */
    carry_flag = 0;                                       /* logical op clears CF */
    eax = eax & esi;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    POP32(esp, esi);
    esp += 4;
    return; /* ret */
}

/* Opt-in high-FPS pacing: preserve the original loop except its minimum
 * refresh wait. The refresh clock itself remains 60 Hz. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void title_guest_run_gameplay_frame(void)
{
    static unsigned refresh_ticks;
    if (!refresh_ticks)
    {
        const char *flag = getenv("RECOMP_TITLE_60FPS");
        refresh_ticks = flag && strcmp(flag, "1") == 0 ? 1u : 2u;
        fprintf(stderr, "[TITLE-PACING] waiting %u refresh tick(s) per frame\n", refresh_ticks);
    }
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    double float_compare_left = 0.0, float_compare_right = 0.0;
    (void)float_compare_left;
    (void)float_compare_right;
    int carry_flag = 0; /* carry flag */
#define fp_push(v)                                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        double _fp_value = (v);                                                                              \
        g_fp_top = (g_fp_top + 7u) & 7u;                                                                     \
        g_fp_stack[g_fp_top] = _fp_value;                                                                    \
    } while (0)
#define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
#define fp_st1() fp_st(1)
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_0005DA70:;
    PUSH32(esp, ecx);
    PUSH32(esp, 1);
    MEM32(esp + 4) = 0xA;
    PUSH32(esp, 0x0005DA80u);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0005DA80:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x0005DA88u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0005DA88:;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    carry_flag = (int)((uint32_t)(eax) < (uint32_t)(0x10));
    eax = eax - 0x10;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    MEM32(0x1B8B68) = eax;
    eax = MEM32(0x1BF588);
    {
        uint64_t _t = (uint64_t)(edx) - (uint64_t)(0) - (uint64_t)carry_flag;
        carry_flag = (int)((_t >> 32) & 1);
        edx = (uint32_t)_t;
    } /* sbb */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sbb result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    MEM32(0x1B8B6C) = edx;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DAAB; /* je: equal / zero */
    }

loc_0005DAA2:;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0005DAA8u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DAA8:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DAAB:;
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = edi;

loc_0005DAB0:;
    title_debug_frame();
    PUSH32(esp, 0x0005DAB5u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0005DAB5:;
    ecx = MEM32(0x1B8B68);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    MEM32(0x1B8B70) = eax;
    eax = MEM32(0x1B8B6C);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    MEM32(0x1B8B74) = edx;
    PUSH32(esp, 0x0005DAD4u);
    RECOMP_ABI_CALL(0x000ADFD0u, sub_000ADFD0); /* call 0x000ADFD0 */

loc_0005DAD4:;
    MEMF(0x1B8B64) = (float)fp_top(); /* fst */
    fp_push(MEMF(0x180C70));          /* fld float */
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x10)) >> 32) & 1);
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1());
    fp_pop(); /* fcompi */
    fp_pop(); /* fstp st(0) */
    if ((g_fp_cmp != 1) /* fcompi */)
    {
        goto loc_0005DAF3; /* jbe: below or equal (unsigned <=) */
    }

loc_0005DAE9:;
    xmm0 = XMM_SCALAR(MEMF(0x18D0A8)); /* movss */
    goto loc_0005DB08;

loc_0005DAF3:;
    xmm1 = XMM_SCALAR(MEMF(0x1B8B64)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x180DAC)); /* movss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0005DB10; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_0005DB08:;
    MEMF(0x1B8B64) = xmm0.f[0]; /* movss */

loc_0005DB10:;
    edx = MEM32(0x1B8B70);
    eax = MEM32(0x1B8B74);
    ecx = MEM32(0x1B8B88);
    MEM32(0x1B8B68) = edx;
    edx = MEM32(0x1B8B8C);
    MEM32(0x1B8B6C) = eax;
    MEM32(0x1B8B80) = ecx;
    MEM32(0x1B8B84) = edx;
    PUSH32(esp, 0x0005DB43u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0005DB43:;
    ecx = (uint32_t)(int32_t)SMEM8(0x1C2028);
    MEM32(0x1B8B88) = eax;
    eax = MEM32(0x1B8B64);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    MEM32(0x1B8B8C) = edx;
    PUSH32(esp, 0x0005DB61u);
    RECOMP_ABI_CALL(0x00090F50u, sub_00090F50); /* call 0x00090F50 */

loc_0005DB61:;
    title_display_input_frame(); /* remap freshly polled Map/warp Start edge */
    edx = MEM32(0x1B8B64);
    xmm0 = XMM_SCALAR(MEMF(0x1B8B64)); /* movss */
    PUSH32(esp, edx);
    MEMF(0x1C2024) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0005DB7Du);
    RECOMP_ABI_CALL(0x0005D180u, sub_0005D180); /* call 0x0005D180 */

loc_0005DB7D:;
    eax = MEM32(0x1B8B64);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0005DB88u);
    RECOMP_ABI_CALL(0x0005F270u, sub_0005F270); /* call 0x0005F270 */

loc_0005DB88:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x10)) >> 32) & 1);
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DCC2; /* je: equal / zero */
    }

loc_0005DB98:;
    ecx = MEM32(0x1BF584);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DCC2; /* je: equal / zero */
    }

loc_0005DBA6:;
    flag_result = (uint32_t)(MEM32(eax + 0x30)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x10000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(eax + 0x30), 0x10000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DBD8; /* je: equal / zero */
    }

loc_0005DBAF:;
    PUSH32(esp, 0);
    PUSH32(esp, 0xFA);
    PUSH32(esp, 0x0005DBBBu);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DBBB:;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEMF(0x1B8BE0) = xmm0.f[0];                           /* movss */
    MEMF(0x1B8BDC) = xmm0.f[0];                           /* movss */
    goto loc_0005DBDB;

loc_0005DBD8:;
    xmm0 = XMM_ZERO(); /* xorps self = zero */

loc_0005DBDB:;
    flag_result = (uint32_t)(MEM32(eax + 0x30)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x20000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(eax + 0x30), 0x20000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DC0B; /* je: equal / zero */
    }

loc_0005DBE4:;
    PUSH32(esp, 1);
    PUSH32(esp, 0xFA);
    PUSH32(esp, 0x0005DBF0u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DBF0:;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEMF(0x1B8BE0) = xmm0.f[0];                           /* movss */
    MEMF(0x1B8BDC) = xmm0.f[0];                           /* movss */

loc_0005DC0B:;
    flag_result = (uint32_t)(MEM8(eax + 0x30)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(eax + 0x30), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DC30; /* je: equal / zero */
    }

loc_0005DC11:;
    PUSH32(esp, 0);
    MEMF(0x1B8BE0) = xmm0.f[0]; /* movss */
    MEMF(0x1B8BDC) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0005DC28u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DC28:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DC30:;
    flag_result = (uint32_t)(MEM32(eax + 0x30)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x40000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(eax + 0x30), 0x40000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DC4B; /* je: equal / zero */
    }

loc_0005DC39:;
    PUSH32(esp, 0x1BF58C);
    PUSH32(esp, 0x0005DC43u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DC43:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DC4B:;
    flag_result = (uint32_t)(MEM32(eax + 0x30)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x80000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(eax + 0x30), 0x80000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DC66; /* je: equal / zero */
    }

loc_0005DC54:;
    PUSH32(esp, 0x1BF60C);
    PUSH32(esp, 0x0005DC5Eu);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DC5E:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DC66:;
    SET_LO8(ecx, MEM8(eax + 0x30));
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((((uint8_t)((uint8_t)(flag_result) - (uint8_t)(flag_operand)) >> 7) == 0))
    {
        goto loc_0005DC7F; /* jns: not sign (positive) */
    }

loc_0005DC6D:;
    PUSH32(esp, 0x1BF68C);
    PUSH32(esp, 0x0005DC77u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DC77:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DC7F:;
    flag_result = (uint32_t)(MEM8(eax + 0x30)) & 0xFFu;
    flag_operand = (uint32_t)(0x20) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(eax + 0x30), 0x20 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DC97; /* je: equal / zero */
    }

loc_0005DC85:;
    PUSH32(esp, 0x1BF70C);
    PUSH32(esp, 0x0005DC8Fu);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DC8F:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DC97:;
    flag_result = (uint32_t)(MEM8(eax + 0x30)) & 0xFFu;
    flag_operand = (uint32_t)(0x10) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(eax + 0x30), 0x10 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DCAF; /* je: equal / zero */
    }

loc_0005DC9D:;
    PUSH32(esp, 0x1BF78C);
    PUSH32(esp, 0x0005DCA7u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DCA7:;
    eax = MEM32(0x1BEF14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DCAF:;
    flag_result = (uint32_t)(MEM8(eax + 0x30)) & 0xFFu;
    flag_operand = (uint32_t)(0x40) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(eax + 0x30), 0x40 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0005DCC2; /* je: equal / zero */
    }

loc_0005DCB5:;
    PUSH32(esp, 0x1BF80C);
    PUSH32(esp, 0x0005DCBFu);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DCBF:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DCC2:;
    PUSH32(esp, 0x0005DCC7u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0005DCC7:;
    edi = eax;
    ebp = edx;
    PUSH32(esp, 0x0005DCD0u);
    RECOMP_ABI_CALL(0x00085A80u, sub_00085A80); /* call 0x00085A80 */

loc_0005DCD0:;
    eax = MEM32(0x1B8D9C);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(7) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 7 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DCE1; /* jne: not equal / not zero */
    }

loc_0005DCDA:;
    esi = 1;
    goto loc_0005DD11;

loc_0005DCE1:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(6) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 6 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DCED; /* jne: not equal / not zero */
    }

loc_0005DCE6:;
    esi = 1;
    goto loc_0005DD11;

loc_0005DCED:;
    PUSH32(esp, 0x0005DCF2u);
    RECOMP_ABI_CALL(0x0006CB90u, sub_0006CB90); /* call 0x0006CB90 */

loc_0005DCF2:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DD0F; /* jne: not equal / not zero */
    }

loc_0005DCF6:;
    flag_result = (uint32_t)(MEM32(0x1EDEFC)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1EDEFC), 2 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DD0F; /* je: equal / zero */
    }

loc_0005DCFF:;
    eax = MEM32(0x1B8B90);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DD0F; /* jne: not equal / not zero */
    }

loc_0005DD08:;
    esi = 1;
    goto loc_0005DD11;

loc_0005DD0F:;
    carry_flag = 0; /* xor clears CF */
    esi = 0;        /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */

loc_0005DD11:;
    ecx = MEM32(0x1B8B64);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0005DD1Du);
    RECOMP_ABI_CALL(0x000791B0u, sub_000791B0); /* call 0x000791B0 */

loc_0005DD1D:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x0005DD25u);
    RECOMP_ABI_CALL(0x00037E90u, sub_00037E90); /* call 0x00037E90 */

loc_0005DD25:;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DD44; /* jne: not equal / not zero */
    }

loc_0005DD29:;
    edx = MEM32(0x1B8B64);
    eax = MEM32(0x1BF88C);
    {
        uint32_t _icall_esp = g_esp;
        PUSH32(esp, edx);
        PUSH32(esp, eax);
        PUSH32(esp, 0x1BEF20);
        {
            uint32_t _icall_target = MEM32(0x1BEF50);
            PUSH32(esp, 0x0005DD41u);
            RECOMP_ICALL_SAFE_AT_CC(_icall_target, _icall_esp, 0x0005DD3Bu);
        } /* indirect call */
    }

loc_0005DD41:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DD44:;
    ecx = MEM32(0x1A3B08);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x1BEE10);
    PUSH32(esp, 0x1BEE00);
    PUSH32(esp, 0x1BEDF0);
    PUSH32(esp, 0x1BEDE0);
    PUSH32(esp, 0x0005DD64u);
    RECOMP_ABI_CALL(0x00095010u, sub_00095010); /* call 0x00095010 */

loc_0005DD64:;
    edx = MEM32(0x1BEF4C);
    eax = MEM32(edx + 0x20);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(eax) + (uint64_t)(0x30)) >> 32) & 1);
    eax = eax + 0x30;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, eax);
    PUSH32(esp, 0x0005DD76u);
    RECOMP_ABI_CALL(0x000950C0u, sub_000950C0); /* call 0x000950C0 */

loc_0005DD76:;
    PUSH32(esp, 0x0005DD7Bu);
    RECOMP_ABI_CALL(0x00094C60u, sub_00094C60); /* call 0x00094C60 */

loc_0005DD7B:;
    ecx = MEM32(0x1B8B64);
    edx = MEM32(0x1BF88C);
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    PUSH32(esp, 0x0005DD8Eu);
    RECOMP_ABI_CALL(0x000622C0u, sub_000622C0); /* call 0x000622C0 */

loc_0005DD8E:;
    eax = MEM32(0x1B8B64);
    PUSH32(esp, eax);
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x0005DD9Eu);
    RECOMP_ABI_CALL(0x000847A0u, sub_000847A0); /* call 0x000847A0 */

loc_0005DD9E:;
    ecx = MEM32(0x1B8B64);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0005DDAAu);
    RECOMP_ABI_CALL(0x00078300u, sub_00078300); /* call 0x00078300 */

loc_0005DDAA:;
    PUSH32(esp, 1);
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x0005DDB6u);
    RECOMP_ABI_CALL(0x00082150u, sub_00082150); /* call 0x00082150 */

loc_0005DDB6:;
    PUSH32(esp, 0x1BEE10);
    PUSH32(esp, 0x0005DDC0u);
    RECOMP_ABI_CALL(0x0007DDA0u, sub_0007DDA0); /* call 0x0007DDA0 */

loc_0005DDC0:;
    PUSH32(esp, 0x0005DDC5u);
    RECOMP_ABI_CALL(0x00077380u, sub_00077380); /* call 0x00077380 */

loc_0005DDC5:;
    PUSH32(esp, 0x0005DDCAu);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0005DDCA:;
    PUSH32(esp, 0x0005DDCFu);
    RECOMP_ABI_CALL(0x00085A80u, sub_00085A80); /* call 0x00085A80 */

loc_0005DDCF:;
    PUSH32(esp, 0x0005DDD4u);
    RECOMP_ABI_CALL(0x000ADFA0u, sub_000ADFA0); /* call 0x000ADFA0 */

loc_0005DDD4:;
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    PUSH32(esp, 0x0005DDDDu);
    RECOMP_ABI_CALL(0x000ADFD0u, sub_000ADFD0); /* call 0x000ADFD0 */

loc_0005DDDD:;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x1B8BE0)); /* fadd dword ptr [0x1b8be0] */
    edx = MEM32(0x1B8B64);
    xmm0 = XMM_SCALAR(MEMF(0x1B8BDC));      /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x180B48); /* addss */
    MEMF(0x1B8BE0) = (float)fp_top();
    fp_pop(); /* fstp */
    flag_operand = (uint32_t)(0x48) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x48)) >> 32) & 1);
    esp = esp + 0x48;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 1);
    PUSH32(esp, edx);
    PUSH32(esp, 0x1BED20);
    MEMF(0x1B8BDC) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0005DE17u);
    RECOMP_ABI_CALL(0x00082800u, sub_00082800); /* call 0x00082800 */

loc_0005DE17:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x0005DE1Fu);
    RECOMP_ABI_CALL(0x000A2EA0u, sub_000A2EA0); /* call 0x000A2EA0 */

loc_0005DE1F:;
    ecx = MEM32(0x1B8AD8);

loc_0005DE25:;
    eax = MEM32(0x214B2C);
    edx = eax;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    carry_flag = (int)((uint32_t)(edx) < (uint32_t)(ecx));
    edx = edx - ecx;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = refresh_ticks;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 2 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0005DE25; /* jb: below (unsigned <) */
    }

loc_0005DE33:;
    MEM32(0x1B8AD8) = eax;
    eax = MEM32(0x1B8B90);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DE4E; /* jne: not equal / not zero */
    }

loc_0005DE41:;
    PUSH32(esp, 0x1BED20);
    PUSH32(esp, 0x0005DE4Bu);
    RECOMP_ABI_CALL(0x00082830u, sub_00082830); /* call 0x00082830 */

loc_0005DE4B:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DE4E:;
    eax = MEM32(0x1B8B64);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0005DE59u);
    RECOMP_ABI_CALL(0x00067B40u, sub_00067B40); /* call 0x00067B40 */

loc_0005DE59:;
    ecx = MEM32(0x1B8B64);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0005DE65u);
    RECOMP_ABI_CALL(0x0006E680u, sub_0006E680); /* call 0x0006E680 */

loc_0005DE65:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x0005DE6Du);
    RECOMP_ABI_CALL(0x0005D440u, sub_0005D440); /* call 0x0005D440 */

loc_0005DE6D:;
    ecx = MEM32(0x1C3934);
    eax = MEM32(esp + 0xC);
    ecx++;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    MEM32(0x1C3934) = ecx;
    if (CMP_LE(signed_flag_result, signed_flag_operand))
    {
        goto loc_0005DE8C; /* jle: less or equal (signed <=) */
    }

loc_0005DE82:;
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0xC) = eax;
    goto loc_0005DF14;

loc_0005DE8C:;
    PUSH32(esp, 2);
    PUSH32(esp, 0x0005DE93u);
    RECOMP_ABI_CALL(0x0005F2E0u, sub_0005F2E0); /* call 0x0005F2E0 */

loc_0005DE93:;
    eax = MEM32(0x1EDEFC);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 2 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DF14; /* je: equal / zero */
    }

loc_0005DEA0:;
    SET_LO8(eax, MEM8(0x1C2030));
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    eax = MEM32(0x1B8B90);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DEBC; /* je: equal / zero */
    }

loc_0005DEAE:;
    ecx = MEM32(0x1BF588);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DEBC; /* jne: not equal / not zero */
    }

loc_0005DEB8:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DECA; /* je: equal / zero */
    }

loc_0005DEBC:;
    SET_LO8(ecx, MEM8(0x1C202B));
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DF14; /* jne: not equal / not zero */
    }

loc_0005DEC6:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DF14; /* jne: not equal / not zero */
    }

loc_0005DECA:;
    edx = MEM32(0x1EDEF8);
    PUSH32(esp, 1);
    PUSH32(esp, edx);
    MEM8(0x1C2030) = 0;
    PUSH32(esp, 0x0005DEDFu);
    RECOMP_ABI_CALL(0x00097510u, sub_00097510); /* call 0x00097510 */

loc_0005DEDF:;
    eax = MEM32(0x1B8D9C);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(7) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 7 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0005DF14; /* je: equal / zero */
    }

loc_0005DEEC:;
    PUSH32(esp, 7);
    PUSH32(esp, 0x0005DEF3u);
    RECOMP_ABI_CALL(0x0005F3A0u, sub_0005F3A0); /* call 0x0005F3A0 */

loc_0005DEF3:;
    PUSH32(esp, 0);
    PUSH32(esp, 0x0005DEFAu);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0005DEFA:;
    eax = MEM32(0x1BEF10);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0005DF05u);
    RECOMP_ABI_CALL(0x000A86A0u, sub_000A86A0); /* call 0x000A86A0 */

loc_0005DF05:;
    PUSH32(esp, 0x18);
    PUSH32(esp, 0x18CFD8);
    PUSH32(esp, 0x0005DF11u);
    RECOMP_ABI_CALL(0x000142F0u, sub_000142F0); /* call 0x000142F0 */

loc_0005DF11:;
    flag_operand = (uint32_t)(0x14) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x14)) >> 32) & 1);
    esp = esp + 0x14;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0005DF14:;
    PUSH32(esp, 0x0005DF19u);
    RECOMP_ABI_CALL(0x0005CB30u, sub_0005CB30); /* call 0x0005CB30 */

loc_0005DF19:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005DAB0; /* jne: not equal / not zero */
    }

loc_0005DF21:;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4;
    return; /* ret */

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}

/* Intro movie skip preserves all gameplay cutscenes. */
void title_guest_play_menu_movie(void)
{
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;

loc_0005B620:;
    if (title_skip_movie(MEM32(esp), MEM32(esp + 4)))
    {
        eax = 1;
        esp += 4;
        return;
    }
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 0x44;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    eax = MEM32(0x1AC744);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(esp + 0x50);
    PUSH32(esp, edi);
    PUSH32(esp, 0x18CEA4);
    PUSH32(esp, esi);
    PUSH32(esp, 0x18CEA0);
    MEM32(esp + 0x58) = eax;
    eax = esp + 0x18;
    PUSH32(esp, 0x18CE98);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0005B64Du);
    RECOMP_ABI_CALL(0x000F1630u, sub_000F1630); /* call 0x000F1630 */

loc_0005B64D:;
    PUSH32(esp, 0x0005B652u);
    RECOMP_ABI_CALL(0x000951A0u, sub_000951A0); /* call 0x000951A0 */

loc_0005B652:;
    PUSH32(esp, 0x0005B657u);
    RECOMP_ABI_CALL(0x0005F2D0u, sub_0005F2D0); /* call 0x0005F2D0 */

loc_0005B657:;
    PUSH32(esp, 1);
    edi = eax;
    PUSH32(esp, 0x0005B660u);
    RECOMP_ABI_CALL(0x0005F2E0u, sub_0005F2E0); /* call 0x0005F2E0 */

loc_0005B660:;
    ecx = MEM32(esp + 0x7C);
    edx = MEM32(esp + 0x78);
    eax = MEM32(esp + 0x74);
    PUSH32(esp, ecx);
    ecx = MEM32(esp + 0x74);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    edx = esp + 0x34;
    PUSH32(esp, edx);
    PUSH32(esp, 0x0005B67Eu);
    RECOMP_ABI_CALL(0x000A3330u, sub_000A3330); /* call 0x000A3330 */

loc_0005B67E:;
    PUSH32(esp, edi);
    ebx = eax;
    PUSH32(esp, 0x0005B686u);
    RECOMP_ABI_CALL(0x0005F2E0u, sub_0005F2E0); /* call 0x0005F2E0 */

loc_0005B686:;
    PUSH32(esp, 0x0005B68Bu);
    RECOMP_ABI_CALL(0x000951C0u, sub_000951C0); /* call 0x000951C0 */

loc_0005B68B:;
    PUSH32(esp, 0x18CE8C);
    PUSH32(esp, esi);
    PUSH32(esp, 0x0005B696u);
    RECOMP_ABI_CALL(0x000971A0u, sub_000971A0); /* call 0x000971A0 */

loc_0005B696:;
    flag_operand = (uint32_t)(0x38) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x38;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005B6C7; /* jne: not equal / not zero */
    }

loc_0005B69D:;
    eax = MEM32(0x1BF88C);
    flag_result = (uint32_t)(MEM32(eax + 0x6C)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x53303035) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(eax + 0x6C), 0x53303035 (32-bit) */
    eax = ebx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005B6C9; /* jne: not equal / not zero */
    }

loc_0005B6AD:;
    POP32(esp, edi);
    POP32(esp, esi);
    MEM32(0x1B6664) = 1;
    POP32(esp, ebx);
    ecx = MEM32(esp + 0x40);
    PUSH32(esp, 0x0005B6C3u);
    RECOMP_ABI_CALL(0x000F16B8u, sub_000F16B8); /* call 0x000F16B8 */

loc_0005B6C3:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x44;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0005B6C7:;
    eax = ebx;

loc_0005B6C9:;
    ecx = MEM32(esp + 0x4C);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    PUSH32(esp, 0x0005B6D5u);
    RECOMP_ABI_CALL(0x000F16B8u, sub_000F16B8); /* call 0x000F16B8 */

loc_0005B6D5:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x44;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */
}

/* Original helmet-charge contact callback, recovered from XBE bytes.
 * The caller selects this address through EBX; it was absent from dispatch. */
/**
 * sub_00024BF0
 * Original: 0x00024BF0 - 0x00024C84 (148 bytes, 41 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void title_guest_handle_helmet_contact(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00024BF0:;
    ecx = MEM32(esp + 8);
    eax = MEM32(ecx + 8);
    flag_result = (uint32_t)(MEM8(eax + 0x15)) & 0xFFu;
    flag_operand = (uint32_t)(0x24) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(eax + 0x15), 0x24 (8-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00024C06; /* jne: not equal / not zero */
    }

loc_00024BFD:;
    MEM32(esp + 8) = ecx;
    g_seh_ebp = ebp;
    sub_00037500();
    return; /* tail jmp 0x00037500 */

loc_00024C06:;
    flag_operand = (uint32_t)(0x34) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    ecx = ecx + 0x34;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, ecx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x00024C10u);
    RECOMP_ABI_CALL(0x0004CC20u, sub_0004CC20); /* call 0x0004CC20 */

loc_00024C10:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00024C81; /* je: equal / zero */
    }

loc_00024C17:;
    eax = MEM32(esp + 4);
    ecx = MEM32(eax + 0x2C);
    edx = MEM32(ecx + 0x20);
    edx = MEM32(edx + 0x30);
    ecx = MEM32(eax + 0x40);
    MEM32(ecx + 0x30) = edx;
    ecx = MEM32(eax + 0x2C);
    edx = MEM32(ecx + 0x20);
    ecx = MEM32(eax + 0x40);
    edx = MEM32(edx + 0x34);
    MEM32(ecx + 0x34) = edx;
    ecx = MEM32(eax + 0x2C);
    edx = MEM32(ecx + 0x20);
    eax = MEM32(eax + 0x40);
    ecx = MEM32(edx + 0x38);
    MEM32(eax + 0x38) = ecx;
    xmm0 = XMM_SCALAR(MEMF(0x1BF050));      /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x180DFC); /* mulss */
    MEM32(0x1BF05C) = 1;
    MEMF(0x1BF050) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x00024C6Fu);
    RECOMP_ABI_CALL(0x000224E0u, sub_000224E0); /* call 0x000224E0 */

loc_00024C6F:;
    edx = (uint32_t)(int32_t)SMEM8(0x1C2028);
    PUSH32(esp, 1);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00024C7Eu);
    RECOMP_ABI_CALL(0x00073BA0u, sub_00073BA0); /* call 0x00073BA0 */

loc_00024C7E:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_00024C81:;
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    esp += 4;
    return; /* ret */
}

/* Main camera FOV: widen before RenderWare updates projection/frustum.
 * Shadow/offscreen cameras use other view-window paths and stay unchanged. */
void title_guest_update_camera_projection(void)
{
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    double float_compare_left = 0.0, float_compare_right = 0.0;
    (void)float_compare_left;
    (void)float_compare_right;
#define fp_push(v)                                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        double _fp_value = (v);                                                                              \
        g_fp_top = (g_fp_top + 7u) & 7u;                                                                     \
        g_fp_stack[g_fp_top] = _fp_value;                                                                    \
    } while (0)
#define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
#define fp_st1() fp_st(1)

loc_000A0260:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    fp_push(MEMF(esp + 0x10));                            /* fld float */
    xmm0 = XMM_SCALAR(MEMF(0x1A7004));                    /* movss */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x180B44));   /* fmul dword ptr [0x180b44] */
    xmm1 = XMM_SCALAR(MEMF(0x1A7008));                    /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x1906EC);               /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x18D5D8);               /* mulss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm1.f[0];                    /* comiss */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x180DF4)); /* fmul dword ptr [0x180df4] */
    {
        fp_top() = tan(fp_top());
        fp_push(1.0);
    } /* fptan */
    fp_pop(); /* fstp st(0) */
    MEMF(esp + 0x10) = (float)fp_top();
    fp_pop();                            /* fstp */
    xmm2 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_000A02CC; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm1.f[0]) */
    }

loc_000A02A6:;
    ecx = MEM32(esp + 0xC);
    eax = esp;
    PUSH32(esp, eax);
    xmm1.f[0] = xmm1.f[0] * xmm2.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    PUSH32(esp, ecx);
    MEMF(esp + 8) = xmm2.f[0];   /* movss */
    MEMF(esp + 0xC) = xmm1.f[0]; /* movss */
    if (xbox_DisplayWidescreen())
    {
        MEMF(esp + 8) *= 4.0f / 3.0f; /* Hor+: retain vertical FOV. */
    }
    PUSH32(esp, 0x000A02C8u);
    RECOMP_ABI_CALL(0x000BDE40u, sub_000BDE40); /* call 0x000BDE40 */

loc_000A02C8:;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_000A02CC:;
    ecx = MEM32(esp + 0xC);
    eax = esp;
    PUSH32(esp, eax);
    xmm0.f[0] = xmm0.f[0] * xmm2.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    PUSH32(esp, ecx);
    MEMF(esp + 8) = xmm0.f[0];   /* movss */
    MEMF(esp + 0xC) = xmm2.f[0]; /* movss */
    if (xbox_DisplayWidescreen())
    {
        MEMF(esp + 8) *= 4.0f / 3.0f; /* Hor+: retain vertical FOV. */
    }
    PUSH32(esp, 0x000A02EEu);
    RECOMP_ABI_CALL(0x000BDE40u, sub_000BDE40); /* call 0x000BDE40 */

loc_000A02EE:;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}

/* Runtime-recovered S005 scene routine. */
/**

 * sub_000135D0

 * Original: 0x000135D0 - 0x000136C0 (240 bytes, 71 insns)

 * CC: cdecl, 0 params, returns int_or_void

 * Frame: fpo_leaf

 */

void title_guest_recovered_s005_135D0(void)

{

    int zero_flag = 0; /* fallback flag var */

    uint32_t flag_result = 0, flag_operand = 0;

    int32_t signed_flag_result = 0, signed_flag_operand = 0;

    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;

loc_000135D0:;

    PUSH32(esp, esi);

    esi = MEM32(esp + 8);

    PUSH32(esp, edi);

    edi = MEM32(esp + 0x10);

    PUSH32(esp, 0x4547454E);

    PUSH32(esp, edi);

    PUSH32(esp, esi);

    PUSH32(esp, 0x000135E6u);
    RECOMP_ABI_CALL(0x00013E60u, sub_00013E60); /* call 0x00013E60 */

loc_000135E6:;

    SET_LO8(eax, MEM8(esi + 5));

    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    esp = esp + 0xC;

    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;

    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */

    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_000135F8; /* jbe: below or equal (unsigned <=) */
    }

loc_000135F0:;

    eax = edi + 0x68;

    MEM32(esi + 8) = eax;

    goto loc_000135FF;

loc_000135F8:;

    MEM32(esi + 8) = 0;

loc_000135FF:;

    MEM32(esi + 0x30) = 0x133D0;

    MEM32(esi + 0x38) = 0;

    MEM32(esi + 0xC) = 0x13490;

    MEM32(esi + 0x3C) = 0x12FD0;

    MEM32(esi + 0xF4) = 0;

    eax = MEM32(edi + 0x64);

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */

    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_000136BD; /* je: equal / zero */
    }

loc_00013630:;

    ecx = esp + 0x10;

    PUSH32(esp, ecx);

    PUSH32(esp, eax);

    PUSH32(esp, 0x0001363Bu);
    RECOMP_ABI_CALL(0x0009CAE0u, sub_0009CAE0); /* call 0x0009CAE0 */

loc_0001363B:;

    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    esp = esp + 8;

    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */

    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_000136BD; /* je: equal / zero */
    }

loc_00013642:;

    PUSH32(esp, 0);

    PUSH32(esp, 0);

    PUSH32(esp, 0x180BF1);

    PUSH32(esp, eax);

    PUSH32(esp, 0x00013651u);
    RECOMP_ABI_CALL(0x0007EBE0u, sub_0007EBE0); /* call 0x0007EBE0 */

loc_00013651:;

    PUSH32(esp, 0);

    PUSH32(esp, 0x180BF1);

    MEM32(esi + 0xF4) = eax;

    PUSH32(esp, 0x00013663u);
    RECOMP_ABI_CALL(0x0007F650u, sub_0007F650); /* call 0x0007F650 */

loc_00013663:;

    PUSH32(esp, 0);

    PUSH32(esp, 0);

    PUSH32(esp, 0);

    PUSH32(esp, 0);

    PUSH32(esp, 0x3F800000);

    PUSH32(esp, 0);

    PUSH32(esp, 0x10);

    PUSH32(esp, 0x180DD4);

    PUSH32(esp, eax);

    MEM32(esi + 0xE8) = eax;

    PUSH32(esp, 0x00013685u);
    RECOMP_ABI_CALL(0x0007F6A0u, sub_0007F6A0); /* call 0x0007F6A0 */

loc_00013685:;

    edx = MEM32(esi + 0xF4);

    eax = MEM32(esi + 0xE8);

    PUSH32(esp, 0x180DD4);

    PUSH32(esp, edx);

    PUSH32(esp, eax);

    PUSH32(esp, 0x0001369Du);
    RECOMP_ABI_CALL(0x000819B0u, sub_000819B0); /* call 0x000819B0 */

loc_0001369D:;

    ecx = MEM32(esi + 0x2C);

    edx = MEM32(esi + 0xE8);

    eax = MEM32(0x1BF88C);

    flag_operand = (uint32_t)(0x48) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    esp = esp + 0x48;

    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    PUSH32(esp, ecx);

    PUSH32(esp, edx);

    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    eax = eax + 0x44;

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    PUSH32(esp, esi);

    PUSH32(esp, eax);

    PUSH32(esp, 0x000136BAu);
    RECOMP_ABI_CALL(0x00081620u, sub_00081620); /* call 0x00081620 */

loc_000136BA:;

    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    esp = esp + 0x10;

    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_000136BD:;

    POP32(esp, edi);

    POP32(esp, esi);

    esp += 4;
    return; /* ret */
}

/* Runtime-recovered S005 scene routine. */
/**

 * sub_00053960

 * Original: 0x00053960 - 0x00053A58 (248 bytes, 73 insns)

 * Category: game_vtable

 * CC: cdecl, 0 params, returns int_or_void

 * Frame: fpo_leaf

 */

void title_guest_recovered_s005_53960(void)

{

    uint32_t ebp = 0;

    ebp = g_ebp; /* frameless: caller's frame */

    int zero_flag = 0; /* fallback flag var */

    uint32_t flag_result = 0, flag_operand = 0;

    int32_t signed_flag_result = 0, signed_flag_operand = 0;

    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;

    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00053960:;

    eax = MEM32(esp + 8);

    PUSH32(esp, ebx);

    PUSH32(esp, esi);

    PUSH32(esp, edi);

    edi = MEM32(esp + 0x10);

    esi = MEM32(edi + 0x2C);

    ebx = 0; /* xor self */

    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */

    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00053A1E; /* jne: not equal / not zero */
    }

loc_00053978:;

    edx = MEM32(edi + 0x380);

    eax = MEM32(edi + 0x6A0);

    edx = edx | 8;

    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* or result */

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(1) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 1 (32-bit) */

    MEM32(0x1B5C68) = 4;

    MEM32(edi + 0x380) = edx;

    if (CMP_GE(signed_flag_result, signed_flag_operand))
    {
        goto loc_000539A1; /* jge: greater or equal (signed >=) */
    }

loc_0005399C:;

    eax = 1;

loc_000539A1:;

    xmm0 = XMM_SCALAR(MEMF(0x181324)); /* movss */

    MEMF(edi + 0x184) = xmm0.f[0]; /* movss */

    xmm0 = XMM_SCALAR(MEMF(0x181280)); /* movss */

    PUSH32(esp, ebp);

    MEM32(edi + 0x390) = eax;

    MEMF(edi + 0x670) = xmm0.f[0]; /* movss */

    ebp = 0; /* xor self */

    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */

    /* nop */

loc_000539D0:;

    eax = MEM32(ebp + 0x19EDF0);

    PUSH32(esp, eax);

    PUSH32(esp, 0x000539DCu);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_000539DC:;

    PUSH32(esp, eax);

    PUSH32(esp, 0x000539E2u);
    RECOMP_ABI_CALL(0x0006D3E0u, sub_0006D3E0); /* call 0x0006D3E0 */

loc_000539E2:;

    MEM32(ebp + 0x1B5F78) = eax;

    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    ebp = ebp + 4;

    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    esp = esp + 8;

    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebp, 0x30 (32-bit) */

    if (CMP_L(signed_flag_result, signed_flag_operand))
    {
        goto loc_000539D0; /* jl: less (signed <) */
    }

loc_000539F3:;

    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */

    POP32(esp, ebp);

    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00053A1E; /* je: equal / zero */
    }

loc_000539F8:;

    ecx = 7;

    eax = 0xFFF8;

loc_00053A02:;

    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, ecx (32-bit) */

    if (CMP_A(flag_result, flag_operand))
    {
        goto loc_00053A17; /* ja: above (unsigned >) */
    }

loc_00053A06:;

    {
        uint32_t _jt = MEM32(ebx * 4 + 0x53A58); /* switch: 8 entries, 2 targets */

        if (_jt == 0x00053A0Du)
        {
            goto loc_00053A0D;
        }

        if (_jt == 0x00053A13u)
        {
            goto loc_00053A13;
        }

        g_seh_ebp = ebp;
        RECOMP_ITAIL(_jt);
        return;
    }

loc_00053A0D:;

    MEM16(esi + 0x18) = MEM16(esi + 0x18) | LO16(ecx);

    flag_result = (uint32_t)(MEM16(esi + 0x18)) & 0xFFFFu;
    signed_flag_result = (int32_t)(int16_t)(flag_result); /* or result */

    goto loc_00053A17;

loc_00053A13:;

    MEM16(esi + 0x18) = MEM16(esi + 0x18) & LO16(eax);

    flag_result = (uint32_t)(MEM16(esi + 0x18)) & 0xFFFFu;
    signed_flag_result = (int32_t)(int16_t)(flag_result); /* and result */

loc_00053A17:;

    esi = MEM32(esi);

    ebx++;

    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */

    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */

    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00053A02; /* jne: not equal / not zero */
    }

loc_00053A1E:;

    eax = MEM32(esp + 0x14);

    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */

    eax = eax - 0;

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */

    if ((flag_result == 0))
    {
        goto loc_00053A48; /* je: equal / zero */
    }

loc_00053A27:;

    eax--;

    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */

    if ((flag_result != 0))
    {
        goto loc_00053A4D; /* jne: not equal / not zero */
    }

loc_00053A2A:;

    flag_result = (uint32_t)(MEM32(edi + 0x110)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(3) & 0xFFFFFFFFu;

    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(edi + 0x110), 3 (32-bit) */

    if (CMP_L(signed_flag_result, signed_flag_operand))
    {
        goto loc_00053A4D; /* jl: less (signed <) */
    }

loc_00053A33:;

    PUSH32(esp, 0xF);

    ebx = edi;

    PUSH32(esp, 0x00053A3Cu);
    RECOMP_ABI_CALL(0x0004E460u, sub_0004E460); /* call 0x0004E460 */

loc_00053A3C:;

    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */

    esp = esp + 4;

    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

    POP32(esp, edi);

    POP32(esp, esi);

    eax = 1;

    POP32(esp, ebx);

    esp += 4;
    return; /* ret */

loc_00053A48:;

    PUSH32(esp, 0x00053A4Du);
    RECOMP_ABI_CALL(0x0003B1E0u, sub_0003B1E0); /* call 0x0003B1E0 */

loc_00053A4D:;

    POP32(esp, edi);

    POP32(esp, esi);

    eax = 1;

    POP32(esp, ebx);

    esp += 4;
    return; /* ret */
}

/* Runtime-recovered enemy setup used by W028. */
/**
 * sub_0004DFD0
 * Original: 0x0004DFD0 - 0x0004E08D (189 bytes, 42 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void title_guest_recovered_w028_enemy_setup(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_0004DFD0:;
    PUSH32(esp, esi);
    esi = MEM32(esp + 8);
    eax = MEM32(esi + 0x114);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x20) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0x20 (32-bit) */
    if (CMP_A(flag_result, flag_operand))
    {
        goto loc_0004E086; /* ja: above (unsigned >) */
    }

loc_0004DFE4:;
    eax = ZX8(MEM8(eax + 0x4E0A8));
    PUSH32(esp, edi);
    {
        uint32_t _jt = MEM32(eax * 4 + 0x4E090); /* switch: 6 entries, 5 targets */
        if (_jt == 0x0004DFF3u)
        {
            goto loc_0004DFF3;
        }
        if (_jt == 0x0004E024u)
        {
            goto loc_0004E024;
        }
        if (_jt == 0x0004E065u)
        {
            goto loc_0004E065;
        }
        if (_jt == 0x0004E079u)
        {
            goto loc_0004E079;
        }
        if (_jt == 0x0004E085u)
        {
            goto loc_0004E085;
        }
        g_seh_ebp = ebp;
        RECOMP_ITAIL(_jt);
        return;
    }

loc_0004DFF3:;
    xmm0 = XMM_SCALAR(MEMF(0x181B60)); /* movss */
    MEMF(esi + 0x3B4) = xmm0.f[0];     /* movss */
    xmm0 = XMM_ZERO();                 /* xorps self = zero */
    MEMF(esi + 0x3B8) = xmm0.f[0];     /* movss */
    MEMF(esi + 0x3BC) = xmm0.f[0];     /* movss */
    ecx = MEM32(esi + 0x4FC);
    MEM32(esi + 0x3D8) = ecx;
    goto loc_0004E053;

loc_0004E024:;
    xmm0 = XMM_SCALAR(MEMF(0x181B60)); /* movss */
    MEMF(esi + 0x3B4) = xmm0.f[0];     /* movss */
    xmm0 = XMM_ZERO();                 /* xorps self = zero */
    MEMF(esi + 0x3B8) = xmm0.f[0];     /* movss */
    MEMF(esi + 0x3BC) = xmm0.f[0];     /* movss */
    edx = MEM32(esi + 0x4FC);
    MEM32(esi + 0x3D8) = edx;

loc_0004E053:;
    MEM32(esi + 0x3B0) = MEM32(esi + 0x3B0) | 0xC000084Bu;
    flag_result = (uint32_t)(MEM32(esi + 0x3B0)) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* or result */
    POP32(esp, edi);
    eax = 1;
    POP32(esp, esi);
    esp += 4;
    return; /* ret */

loc_0004E065:;
    edi = esi;
    PUSH32(esp, 0x0004E06Cu);
    RECOMP_ABI_CALL(0x00046950u, sub_00046950); /* call 0x00046950 */

loc_0004E06C:;
    PUSH32(esp, 0x0004E071u);
    RECOMP_ABI_CALL(0x00043230u, sub_00043230); /* call 0x00043230 */

loc_0004E071:;
    POP32(esp, edi);
    eax = 1;
    POP32(esp, esi);
    esp += 4;
    return; /* ret */

loc_0004E079:;
    edi = esi;
    PUSH32(esp, 0x0004E080u);
    RECOMP_ABI_CALL(0x00046800u, sub_00046800); /* call 0x00046800 */

loc_0004E080:;
    PUSH32(esp, 0x0004E085u);
    RECOMP_ABI_CALL(0x00043050u, sub_00043050); /* call 0x00043050 */

loc_0004E085:;
    POP32(esp, edi);

loc_0004E086:;
    eax = 1;
    POP32(esp, esi);
    esp += 4;
    return; /* ret */
}

/* Runtime-recovered O008 callback. */
/**
 * sub_000E9E00
 * Original: 0x000E9E00 - 0x000EA2B8 (1208 bytes, 293 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void title_guest_recovered_o008_E9E00(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
#define fp_push(v)                                                                                           \
    do                                                                                                       \
    {                                                                                                        \
        double _fp_value = (v);                                                                              \
        g_fp_top = (g_fp_top + 7u) & 7u;                                                                     \
        g_fp_stack[g_fp_top] = _fp_value;                                                                    \
    } while (0)
#define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
#define fp_st1() fp_st(1)
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_000E9E00:;
    eax = MEM32(0x595BDC);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = MEM32(esp + 0x14);
    PUSH32(esp, edi);
    edi = MEM32(eax + esi);
    ecx = MEM32(edi);
    PUSH32(esp, ecx);
    MEM32(esp + 0x1C) = edi;
    PUSH32(esp, 0x000E9E1Bu);
    RECOMP_ABI_CALL(0x000E9C90u, sub_000E9C90); /* call 0x000E9C90 */

loc_000E9E1B:;
    fp_push(MEMF(edi + 8));                             /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x181AF0)); /* fmul dword ptr [0x181af0] */
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x000E9E2Cu);
    RECOMP_ABI_CALL(0x000F16C8u, sub_000F16C8); /* call 0x000F16C8 */

loc_000E9E2C:;
    ecx = eax;
    eax = eax << 8;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    ecx = ecx & 0xFF;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    eax = eax | ecx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* or result */
    eax = eax << 8;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax = eax | ecx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* or result */
    eax = eax << 8;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax = eax | ecx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* or result */
    ecx = MEM32(0x59C3B0);
    ebp = 0; /* xor self */
    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, ebp (32-bit) */
    edi = 1;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9E72; /* jne: not equal / not zero */
    }

loc_000E9E54:;
    ecx = MEM32(0x59C140);
    MEM32(ecx * 4 + 0x59C400) = 0x94;
    ecx++;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C3B0) = edi;
    MEM32(0x59C140) = ecx;

loc_000E9E72:;
    MEM32(0x59BCD0) = eax;
    eax = MEM32(esi);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebp (32-bit) */
    PUSH32(esp, ebx);
    ebx = 4;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_000E9FA3; /* je: equal / zero */
    }

loc_000E9E87:;
    PUSH32(esp, ebp);
    PUSH32(esp, eax);
    PUSH32(esp, 0x000E9E8Eu);
    RECOMP_ABI_CALL(0x000C13C0u, sub_000C13C0); /* call 0x000C13C0 */

loc_000E9E8E:;
    eax = MEM32(0x59C000);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebp (32-bit) */
    eax = MEM32(0x59BF20);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9EBA; /* jne: not equal / not zero */
    }

loc_000E9E9F:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C000) = edi;
    MEM32(edx + 0x59B880) = 0xC;
    MEM32(0x59BF20) = eax;

loc_000E9EBA:;
    flag_result = (uint32_t)(MEM32(0x59C020)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x59C020), ebp (32-bit) */
    MEM32(0x59BDE0) = ebx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9EE3; /* jne: not equal / not zero */
    }

loc_000E9EC8:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C020) = edi;
    MEM32(ecx + 0x59B880) = 0xE;
    MEM32(0x59BF20) = eax;

loc_000E9EE3:;
    flag_result = (uint32_t)(MEM32(0x59C030)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x59C030), ebp (32-bit) */
    MEM32(0x59BE00) = ebp;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9F0C; /* jne: not equal / not zero */
    }

loc_000E9EF1:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C030) = edi;
    MEM32(edx + 0x59B880) = 0xF;
    MEM32(0x59BF20) = eax;

loc_000E9F0C:;
    edx = MEM32(0x59C040);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    ecx = 2;
    MEM32(0x59BE10) = ecx;
    ebp = 0x10;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9F3D; /* jne: not equal / not zero */
    }

loc_000E9F26:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C040) = edi;
    MEM32(edx + 0x59B880) = ebp;
    MEM32(0x59BF20) = eax;

loc_000E9F3D:;
    edx = MEM32(0x59C060);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    MEM32(0x59BE20) = ebx;
    esi = 0x12;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9F69; /* jne: not equal / not zero */
    }

loc_000E9F52:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C060) = edi;
    MEM32(edx + 0x59B880) = esi;
    MEM32(0x59BF20) = eax;

loc_000E9F69:;
    edx = MEM32(0x59C070);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    MEM32(0x59BE40) = 0;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9F98; /* jne: not equal / not zero */
    }

loc_000E9F7D:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C070) = edi;
    MEM32(edx + 0x59B880) = 0x13;
    MEM32(0x59BF20) = eax;

loc_000E9F98:;
    MEM32(0x59BE50) = ecx;
    goto loc_000EA05B;

loc_000E9FA3:;
    flag_result = (uint32_t)(MEM32(0x59C000)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x59C000), ebp (32-bit) */
    eax = MEM32(0x59BF20);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9FCB; /* jne: not equal / not zero */
    }

loc_000E9FB0:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C000) = edi;
    MEM32(ecx + 0x59B880) = 0xC;
    MEM32(0x59BF20) = eax;

loc_000E9FCB:;
    flag_result = (uint32_t)(MEM32(0x59C020)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x59C020), ebp (32-bit) */
    ecx = 2;
    MEM32(0x59BDE0) = ecx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000E9FF9; /* jne: not equal / not zero */
    }

loc_000E9FDE:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C020) = edi;
    MEM32(edx + 0x59B880) = 0xE;
    MEM32(0x59BF20) = eax;

loc_000E9FF9:;
    edx = MEM32(0x59C040);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    MEM32(0x59BE00) = ebp;
    ebp = 0x10;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA025; /* jne: not equal / not zero */
    }

loc_000EA00E:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C040) = edi;
    MEM32(edx + 0x59B880) = ebp;
    MEM32(0x59BF20) = eax;

loc_000EA025:;
    MEM32(0x59BE20) = ecx;
    ecx = MEM32(0x59C060);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    esi = 0x12;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA051; /* jne: not equal / not zero */
    }

loc_000EA03A:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C060) = edi;
    MEM32(ecx + 0x59B880) = esi;
    MEM32(0x59BF20) = eax;

loc_000EA051:;
    MEM32(0x59BE40) = 0;

loc_000EA05B:;
    ecx = MEM32(0x59C080);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA080; /* jne: not equal / not zero */
    }

loc_000EA065:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C080) = edi;
    MEM32(edx + 0x59B880) = 0x14;
    MEM32(0x59BF20) = eax;

loc_000EA080:;
    eax = MEM32(0x59C004);
    edx = 5;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    eax = MEM32(0x59BF24);
    MEM32(0x59BE60) = edx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA0B4; /* jne: not equal / not zero */
    }

loc_000EA099:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C004) = edi;
    MEM32(ecx + 0x59B884) = 0xC;
    MEM32(0x59BF24) = eax;

loc_000EA0B4:;
    ecx = MEM32(0x59C024);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BDE4) = ebx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA0DF; /* jne: not equal / not zero */
    }

loc_000EA0C4:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C024) = edi;
    MEM32(ecx + 0x59B884) = 0xE;
    MEM32(0x59BF24) = eax;

loc_000EA0DF:;
    ecx = MEM32(0x59C034);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BE04) = 2;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA10E; /* jne: not equal / not zero */
    }

loc_000EA0F3:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C034) = edi;
    MEM32(ecx + 0x59B884) = 0xF;
    MEM32(0x59BF24) = eax;

loc_000EA10E:;
    ebx = MEM32(0x59C044);
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, 0 (32-bit) */
    ecx = 3;
    MEM32(0x59BE14) = ecx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA13A; /* jne: not equal / not zero */
    }

loc_000EA123:;
    ebx = eax;
    ebx = ebx << 4;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C044) = edi;
    MEM32(ebx + 0x59B884) = ebp;
    MEM32(0x59BF24) = eax;

loc_000EA13A:;
    ebx = MEM32(0x59C064);
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, 0 (32-bit) */
    MEM32(0x59BE24) = 2;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA165; /* jne: not equal / not zero */
    }

loc_000EA14E:;
    ebx = eax;
    ebx = ebx << 4;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C064) = edi;
    MEM32(ebx + 0x59B884) = esi;
    MEM32(0x59BF24) = eax;

loc_000EA165:;
    ebx = MEM32(0x59C074);
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, 0 (32-bit) */
    MEM32(0x59BE44) = 2;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA194; /* jne: not equal / not zero */
    }

loc_000EA179:;
    ebx = eax;
    ebx = ebx << 4;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C074) = edi;
    MEM32(ebx + 0x59B884) = 0x13;
    MEM32(0x59BF24) = eax;

loc_000EA194:;
    eax = MEM32(0x59C008);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    eax = MEM32(0x59BF28);
    MEM32(0x59BE54) = ecx;
    POP32(esp, ebx);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA1C4; /* jne: not equal / not zero */
    }

loc_000EA1A9:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C008) = edi;
    MEM32(ecx + 0x59B888) = 0xC;
    MEM32(0x59BF28) = eax;

loc_000EA1C4:;
    ecx = MEM32(0x59C028);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BDE8) = 7;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA1F3; /* jne: not equal / not zero */
    }

loc_000EA1D8:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C028) = edi;
    MEM32(ecx + 0x59B888) = 0xE;
    MEM32(0x59BF28) = eax;

loc_000EA1F3:;
    ecx = MEM32(0x59C038);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BE08) = edx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA21E; /* jne: not equal / not zero */
    }

loc_000EA203:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C038) = edi;
    MEM32(ecx + 0x59B888) = 0xF;
    MEM32(0x59BF28) = eax;

loc_000EA21E:;
    ecx = MEM32(0x59C048);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BE18) = edi;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA245; /* jne: not equal / not zero */
    }

loc_000EA22E:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C048) = edi;
    MEM32(ecx + 0x59B888) = ebp;
    MEM32(0x59BF28) = eax;

loc_000EA245:;
    ecx = MEM32(0x59C068);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BE28) = 2;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA270; /* jne: not equal / not zero */
    }

loc_000EA259:;
    ecx = eax;
    ecx = ecx << 4;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C068) = edi;
    MEM32(ecx + 0x59B888) = esi;
    MEM32(0x59BF28) = eax;

loc_000EA270:;
    ecx = MEM32(0x59C078);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    MEM32(0x59BE48) = edx;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA29B; /* jne: not equal / not zero */
    }

loc_000EA280:;
    edx = eax;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C078) = edi;
    MEM32(edx + 0x59B888) = 0x13;
    MEM32(0x59BF28) = eax;

loc_000EA29B:;
    eax = MEM32(esp + 0x18);
    MEM32(0x59BE58) = edi;
    ecx = MEM32(eax + 4);
    PUSH32(esp, edi);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x000EA2AFu);
    RECOMP_ABI_CALL(0x000C13C0u, sub_000C13C0); /* call 0x000C13C0 */

loc_000EA2AF:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    POP32(esp, edi);
    POP32(esp, esi);
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    POP32(esp, ebp);
    esp += 4;
    return; /* ret */

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}

/* Runtime-recovered O008 callback. */
/**
 * sub_000EA2C0
 * Original: 0x000EA2C0 - 0x000EA3BD (253 bytes, 65 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void title_guest_recovered_o008_EA2C0(void)
{
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;

loc_000EA2C0:;
    ecx = MEM32(0x59C080);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    eax = 1;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA2F0; /* jne: not equal / not zero */
    }

loc_000EA2CF:;
    ecx = MEM32(0x59BF20);
    edx = ecx;
    edx = edx << 4;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    ecx++;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C080) = eax;
    MEM32(edx + 0x59B880) = 0x14;
    MEM32(0x59BF20) = ecx;

loc_000EA2F0:;
    edx = MEM32(0x59C004);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    PUSH32(esp, esi);
    MEM32(0x59BE60) = eax;
    ecx = 0xC;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA322; /* jne: not equal / not zero */
    }

loc_000EA305:;
    edx = MEM32(0x59BF24);
    esi = edx;
    esi = esi << 4;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    edx++;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C004) = eax;
    MEM32(esi + 0x59B884) = ecx;
    MEM32(0x59BF24) = edx;

loc_000EA322:;
    edx = MEM32(0x59C008);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    MEM32(0x59BDE4) = eax;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA34E; /* jne: not equal / not zero */
    }

loc_000EA331:;
    edx = MEM32(0x59BF28);
    esi = edx;
    esi = esi << 4;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    edx++;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C008) = eax;
    MEM32(esi + 0x59B888) = ecx;
    MEM32(0x59BF28) = edx;

loc_000EA34E:;
    edx = MEM32(0x59C044);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    MEM32(0x59BDE8) = eax;
    ecx = 0x10;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA37F; /* jne: not equal / not zero */
    }

loc_000EA362:;
    edx = MEM32(0x59BF24);
    esi = edx;
    esi = esi << 4;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    edx++;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C044) = eax;
    MEM32(esi + 0x59B884) = ecx;
    MEM32(0x59BF24) = edx;

loc_000EA37F:;
    edx = MEM32(0x59C048);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    MEM32(0x59BE24) = eax;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000EA3AB; /* jne: not equal / not zero */
    }

loc_000EA38E:;
    edx = MEM32(0x59BF28);
    esi = edx;
    esi = esi << 4;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    edx++;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x59C048) = eax;
    MEM32(esi + 0x59B888) = ecx;
    MEM32(0x59BF28) = edx;

loc_000EA3AB:;
    PUSH32(esp, eax);
    PUSH32(esp, 0);
    MEM32(0x59BE28) = eax;
    PUSH32(esp, 0x000EA3B8u);
    RECOMP_ABI_CALL(0x000C13C0u, sub_000C13C0); /* call 0x000C13C0 */

loc_000EA3B8:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    POP32(esp, esi);
    esp += 4;
    return; /* ret */
}

/* Runtime-recovered O008 callback. */
/**
 * sub_000525B0
 * Original: 0x000525B0 - 0x000527BA (522 bytes, 150 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void title_guest_recovered_o008_525B0(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_000525B0:;
    eax = MEM32(esp + 8);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x1F) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0x1F (32-bit) */
    if (CMP_A(flag_result, flag_operand))
    {
        goto loc_000527B1; /* ja: above (unsigned >) */
    }

loc_000525C1:;
    eax = ZX8(MEM8(eax + 0x527E4));
    {
        uint32_t _jt = MEM32(eax * 4 + 0x527BC); /* switch: 10 entries, 10 targets */
        if (_jt == 0x000525CFu)
        {
            goto loc_000525CF;
        }
        if (_jt == 0x000525D9u)
        {
            goto loc_000525D9;
        }
        if (_jt == 0x000525F4u)
        {
            goto loc_000525F4;
        }
        if (_jt == 0x0005264Au)
        {
            goto loc_0005264A;
        }
        if (_jt == 0x00052665u)
        {
            goto loc_00052665;
        }
        if (_jt == 0x000526E0u)
        {
            goto loc_000526E0;
        }
        if (_jt == 0x0005272Du)
        {
            goto loc_0005272D;
        }
        if (_jt == 0x00052750u)
        {
            goto loc_00052750;
        }
        if (_jt == 0x0005279Fu)
        {
            goto loc_0005279F;
        }
        if (_jt == 0x000527B1u)
        {
            goto loc_000527B1;
        }
        g_seh_ebp = ebp;
        RECOMP_ITAIL(_jt);
        return;
    }

loc_000525CF:;
    eax = 1;
    goto loc_000527A4;

loc_000525D9:;
    edx = MEM32(esp + 0x10);
    PUSH32(esp, edx);
    eax = 1;
    PUSH32(esp, 0x000525E8u);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_000525E8:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_000525F4:;
    eax = MEM32(esp + 0x18);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00052617; /* jne: not equal / not zero */
    }

loc_000525FC:;
    eax = MEM32(esp + 0x10);
    PUSH32(esp, eax);
    eax = 1;
    PUSH32(esp, 0x0005260Bu);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_0005260B:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_00052617:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 2 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00052626; /* jne: not equal / not zero */
    }

loc_0005261C:;
    eax = 0x3B;
    goto loc_000527A4;

loc_00052626:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xC (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000527B1; /* jne: not equal / not zero */
    }

loc_0005262F:;
    edx = MEM32(esp + 0x10);
    PUSH32(esp, edx);
    eax = 0x3D;
    PUSH32(esp, 0x0005263Eu);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_0005263E:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0005264A:;
    eax = MEM32(esp + 0x10);
    PUSH32(esp, eax);
    eax = 0x3C;
    PUSH32(esp, 0x00052659u);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_00052659:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_00052665:;
    eax = MEM32(esp + 0x18);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00052692; /* jne: not equal / not zero */
    }

loc_0005266D:;
    ecx = MEM32(esp + 0x10);
    PUSH32(esp, ecx);
    edx = 0x3F;
    ecx = 0x3E;
    eax = 0x3C;
    PUSH32(esp, 0x00052686u);
    RECOMP_ABI_CALL(0x0004C040u, sub_0004C040); /* call 0x0004C040 */

loc_00052686:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_00052692:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xB) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xB (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000526B7; /* jne: not equal / not zero */
    }

loc_00052697:;
    edx = MEM32(esp + 0x10);
    PUSH32(esp, edx);
    ecx = 0x3C;
    eax = 0x3F;
    PUSH32(esp, 0x000526ABu);
    RECOMP_ABI_CALL(0x0004C010u, sub_0004C010); /* call 0x0004C010 */

loc_000526AB:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_000526B7:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xC (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000527B1; /* jne: not equal / not zero */
    }

loc_000526C0:;
    eax = MEM32(esp + 0x10);
    PUSH32(esp, eax);
    ecx = 0x3C;
    eax = 0x3E;
    PUSH32(esp, 0x000526D4u);
    RECOMP_ABI_CALL(0x0004C010u, sub_0004C010); /* call 0x0004C010 */

loc_000526D4:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_000526E0:;
    eax = MEM32(esp + 0x18);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_000526F6; /* je: equal / zero */
    }

loc_000526E8:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xD) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xD (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_000526F6; /* je: equal / zero */
    }

loc_000526ED:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xE) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xE (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000527B1; /* jne: not equal / not zero */
    }

loc_000526F6:;
    edx = MEM32(esp + 0x10);
    PUSH32(esp, 3);
    ecx = esp + 4;
    PUSH32(esp, ecx);
    PUSH32(esp, 0);
    PUSH32(esp, edx);
    MEM32(esp + 0x10) = 0x43;
    MEM32(esp + 0x14) = 0x44;
    MEM32(esp + 0x18) = 0x45;
    PUSH32(esp, 0x00052721u);
    RECOMP_ABI_CALL(0x0004C0B0u, sub_0004C0B0); /* call 0x0004C0B0 */

loc_00052721:;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0005272D:;
    eax = MEM32(esp + 0x18);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000527B1; /* jne: not equal / not zero */
    }

loc_00052735:;
    eax = MEM32(esp + 0x10);
    PUSH32(esp, eax);
    eax = 0x40;
    PUSH32(esp, 0x00052744u);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_00052744:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_00052750:;
    eax = MEM32(esp + 0x18);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005275F; /* jne: not equal / not zero */
    }

loc_00052758:;
    eax = 0x41;
    goto loc_000527A4;

loc_0005275F:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xF) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xF (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0005277F; /* jne: not equal / not zero */
    }

loc_00052764:;
    edx = MEM32(esp + 0x10);
    PUSH32(esp, edx);
    eax = 0x42;
    PUSH32(esp, 0x00052773u);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_00052773:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0005277F:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0x10 (32-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_000527B1; /* jne: not equal / not zero */
    }

loc_00052784:;
    eax = MEM32(esp + 0x10);
    PUSH32(esp, eax);
    eax = 0x41;
    PUSH32(esp, 0x00052793u);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_00052793:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0005279F:;
    eax = 0x46;

loc_000527A4:;
    ecx = MEM32(esp + 0x10);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x000527AEu);
    RECOMP_ABI_CALL(0x0004BFE0u, sub_0004BFE0); /* call 0x0004BFE0 */

loc_000527AE:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_000527B1:;
    eax = 1;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */
}

/* Lightning stun delivery diagnostics. */
void title_guest_trace_lightning_stun_1B340(void)
{
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;

loc_0001B340:;
    fprintf(stderr, "[STUN] attack power_flags=%08X\n", MEM32(0x1BF2E0));

    eax = MEM32(0x1BF2E0);
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x40) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x40 (8-bit) */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0001B4B6; /* je: equal / zero */
    }

loc_0001B34E:;
    eax = MEM32(0x1AF6E4);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0001B48E; /* je: equal / zero */
    }

loc_0001B35B:;
    eax = MEM32(0x1BEF64);
    ecx = MEM32(0x1BEF4C);
    xmm1 = XMM_SCALAR(MEMF(0x180B44)); /* movss */
    MEM32(0x1AF3F0) = 0x205E1;
    xmm0 = XMM_SCALAR(MEMF(eax + 0x28));      /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(eax + 0x18); /* mulss */
    edx = MEM32(ecx + 0x20);
    xmm0.f[0] = xmm0.f[0] + MEMF(edx + 0x30); /* addss */
    MEMF(0x1AF400) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x2C));      /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(eax + 0x18); /* mulss */
    edx = MEM32(ecx + 0x20);
    xmm0.f[0] = xmm0.f[0] + MEMF(edx + 0x34); /* addss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0];        /* addss */
    MEMF(0x1AF404) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x30));      /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(eax + 0x18); /* mulss */
    eax = MEM32(ecx + 0x20);
    xmm0.f[0] = xmm0.f[0] + MEMF(eax + 0x38); /* addss */
    MEMF(0x1AF408) = xmm0.f[0];               /* movss */
    xmm0 = XMM_ZERO();                        /* xorps self = zero */
    MEMF(0x1AF42C) = xmm1.f[0];               /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x180DFC));        /* movss */
    PUSH32(esp, ebx);
    MEMF(0x1AF424) = xmm0.f[0];        /* movss */
    MEMF(0x1AF430) = xmm1.f[0];        /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x181318)); /* movss */
    MEMF(0x1AF40C) = xmm0.f[0];        /* movss */
    MEMF(0x1AF414) = xmm0.f[0];        /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x180E00)); /* movss */
    PUSH32(esp, esi);
    MEM8(0x1AF420) = 0x40;
    MEM8(0x1AF421) = 0x40;
    MEM8(0x1AF422) = 0x40;
    MEM8(0x1AF423) = 0;
    MEMF(0x1AF410) = xmm1.f[0]; /* movss */
    MEMF(0x1AF43C) = xmm0.f[0]; /* movss */
    esi = 0;                    /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    SET_LO8(ebx, 0x80);
    /* nop */

loc_0001B440:;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */
    MEM8(0x1AF41D) = LO8(ebx);
    MEM8(0x1AF41C) = LO8(ebx);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0001B45F; /* jne: not equal / not zero */
    }

loc_0001B450:;
    MEM8(0x1AF41E) = LO8(ebx);
    MEM8(0x1AF41F) = 0x40;
    goto loc_0001B46D;

loc_0001B45F:;
    MEM8(0x1AF41E) = 0x40;
    MEM8(0x1AF41F) = 0x60;

loc_0001B46D:;
    ecx = MEM32(0x1AF6E4);
    PUSH32(esp, 0x1AF3F0);
    PUSH32(esp, 0x3F800000);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0001B483u);
    RECOMP_ABI_CALL(0x000714F0u, sub_000714F0); /* call 0x000714F0 */

loc_0001B483:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esi++;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 2 (32-bit) */
    if (CMP_L(signed_flag_result, signed_flag_operand))
    {
        goto loc_0001B440; /* jl: less (signed <) */
    }

loc_0001B48C:;
    POP32(esp, esi);
    POP32(esp, ebx);

loc_0001B48E:;
    xmm0 = XMM_SCALAR(MEMF(0x181314)); /* movss */
    MEMF(0x1BF09C) = xmm0.f[0];        /* movss */
    xmm0 = XMM_ZERO();                 /* xorps self = zero */
    PUSH32(esp, 3);
    MEMF(0x1BF0A0) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0001B4B0u);
    RECOMP_ABI_CALL(0x00067AF0u, sub_00067AF0); /* call 0x00067AF0 */

loc_0001B4B0:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 0;                                              /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    esp += 4;
    return; /* ret */

loc_0001B4B6:;
    PUSH32(esp, 2);
    PUSH32(esp, 0x0001B4BDu);
    RECOMP_ABI_CALL(0x00067AF0u, sub_00067AF0); /* call 0x00067AF0 */

loc_0001B4BD:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = 0;                                              /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    esp += 4;
    return; /* ret */
}

/* Lightning stun delivery diagnostics. */
void title_guest_trace_lightning_stun_4CF30(void)
{
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    double float_compare_left = 0.0, float_compare_right = 0.0;
    (void)float_compare_left;
    (void)float_compare_right;

loc_0004CF30:;
    fprintf(stderr, "[STUN] hit target=%08X duration=%.3f\n", MEM32(esp + 4), MEMF(esp + 8));

    flag_operand = (uint32_t)(0x28) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 0x28;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x30));                  /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x180DAC));                    /* movss */
    eax = 0;                                              /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm1.f[0]; /* comiss */
    MEM32(esp + 4) = eax;
    MEM32(esp + 8) = eax;
    MEM32(esp + 0xC) = eax;
    MEM32(esp + 0x10) = eax;
    MEM32(esp + 0x14) = eax;
    MEM32(esp + 0x18) = eax;
    MEM32(esp + 0x1C) = eax;
    MEM32(esp + 0x20) = eax;
    MEM32(esp) = 0x1E;
    MEM32(esp + 0x24) = eax;
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0004CF7F; /* ja: above (unsigned >) (xmm0.f[0] vs xmm1.f[0]) */
    }

loc_0004CF79:;
    MEMF(esp + 0x14) = xmm1.f[0]; /* movss */

loc_0004CF7F:;
    PUSH32(esp, ebx);
    ebx = MEM32(esp + 0x30);
    eax = esp + 4;
    PUSH32(esp, 0x0004CF8Du);
    RECOMP_ABI_CALL(0x00046790u, sub_00046790); /* call 0x00046790 */

loc_0004CF8D:;
    PUSH32(esp, 0xC);
    PUSH32(esp, 0x0004CF94u);
    RECOMP_ABI_CALL(0x00067AF0u, sub_00067AF0); /* call 0x00067AF0 */

loc_0004CF94:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    POP32(esp, ebx);
    flag_operand = (uint32_t)(0x28) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x28;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */
}

/* Particle atlas initialization: byte IMUL preserves the column count in DL. */
/**
 * sub_00071E80
 * Original: 0x00071E80 - 0x00072039 (441 bytes, 154 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void title_guest_initialize_particle_atlas(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;       /* frameless: caller's frame */
    int zero_flag = 0; /* fallback flag var */
    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00071E80:;
    PUSH32(esp, esi);
    esi = MEM32(esp + 8);
    PUSH32(esp, edi);
    edi = MEM32(esp + 0x10);
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, 0x00071E91u);
    RECOMP_ABI_CALL(0x00081BA0u, sub_00081BA0); /* call 0x00081BA0 */

loc_00071E91:;
    SET_LO8(eax, MEM8(esi + 5));
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    MEM32(esi + 0xC) = 0x71C50;
    MEM32(esi + 0x10) = edi;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00071EB1; /* je: equal / zero */
    }

loc_00071EA5:;
    eax = MEM32(edi + 0x1C);
    ecx = eax + edi + 0x20;
    MEM32(esi + 8) = ecx;
    goto loc_00071EB8;

loc_00071EB1:;
    MEM32(esi + 8) = 0;

loc_00071EB8:;
    SET_LO8(edx, MEM8(edi + 0x14));
    SET_LO8(edx, LO8(edx) & 1);
    flag_result = (uint32_t)(LO8(edx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result); /* and result */
    PUSH32(esp, ebx);
    MEM8(esi + 0x24) = LO8(edx);
    eax = ZX8(MEM8(edi + 0x1B));
    PUSH32(esp, ebp);
    ebp = 0; /* xor self */
    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebp (32-bit) */
    MEM32(esi + 0x14) = eax;
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_00071EE8; /* jbe: below or equal (unsigned <=) */
    }

loc_00071ED0:;
    ecx = MEM32(0x1C5120);
    eax = eax << 3;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00071EE0u);
    RECOMP_ABI_CALL(0x0008F970u, sub_0008F970); /* call 0x0008F970 */

loc_00071EE0:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEM32(esi + 0x18) = eax;
    goto loc_00071EEB;

loc_00071EE8:;
    MEM32(esi + 0x18) = ebp;

loc_00071EEB:;
    eax = MEM32(esi + 0x14);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    ebx = edi + 0x20;
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_00071F26; /* jbe: below or equal (unsigned <=) */
    }

loc_00071EF5:;
    goto loc_00071F00;

    /* nop */
    edi = edi;

loc_00071F00:;
    edx = MEM32(esi + 0x18);
    MEM32(edx + ebp * 8) = 1;
    eax = MEM32(esi + 0x18);
    MEM32(eax + ebp * 8 + 4) = ebx;
    ecx = MEM32(ebx);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00071F19u);
    RECOMP_ABI_CALL(0x0006E960u, sub_0006E960); /* call 0x0006E960 */

loc_00071F19:;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    ebx = ebx + eax;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = MEM32(esi + 0x14);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    ebp++;
    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(ebp) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebp, eax (32-bit) */
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_00071F00; /* jb: below (unsigned <) */
    }

loc_00071F26:;
    edx = MEM32(0x1C5120);
    PUSH32(esp, 0x24);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00071F34u);
    RECOMP_ABI_CALL(0x0008F970u, sub_0008F970); /* call 0x0008F970 */

loc_00071F34:;
    PUSH32(esp, eax);
    MEM32(esi + 0x20) = eax;
    PUSH32(esp, 0x00071F3Du);
    RECOMP_ABI_CALL(0x000913C0u, sub_000913C0); /* call 0x000913C0 */

loc_00071F3D:;
    ecx = MEM32(esi + 0x20);
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    SET_LO8(eax, MEM8(edi + 0x15));
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00071F4Cu);
    RECOMP_ABI_CALL(0x00091490u, sub_00091490); /* call 0x00091490 */

loc_00071F4C:;
    edx = MEM32(esi + 0x20);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00071F55u);
    RECOMP_ABI_CALL(0x000914A0u, sub_000914A0); /* call 0x000914A0 */

loc_00071F55:;
    ecx = MEM32(esi + 0x20);
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    SET_LO8(eax, MEM8(edi + 0x14));
    eax = eax >> 1;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    eax = ~eax;
    eax = eax & 1;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00071F6Bu);
    RECOMP_ABI_CALL(0x000913F0u, sub_000913F0); /* call 0x000913F0 */

loc_00071F6B:;
    edx = ZX8(MEM8(esi + 0x24));
    eax = MEM32(esi + 0x20);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x00071F79u);
    RECOMP_ABI_CALL(0x00091480u, sub_00091480); /* call 0x00091480 */

loc_00071F79:;
    edx = MEM32(esi + 0x20);
    ecx = 0; /* xor self */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    SET_LO8(ecx, MEM8(edi + 0x14));
    ecx = ecx >> 2;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    ecx = ~ecx;
    ecx = ecx & 1;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00071F90u);
    RECOMP_ABI_CALL(0x00091420u, sub_00091420); /* call 0x00091420 */

loc_00071F90:;
    ecx = MEM32(esi + 0x14);
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x30;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    MEM32(esi + 0x1C) = eax;
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0007200A; /* jbe: below or equal (unsigned <=) */
    }

loc_00071F9F:;
    edx = MEM32(esi + 0x18);
    ecx = edx + 4;

loc_00071FA5:;
    edi = MEM32(ecx);
    flag_result = (uint32_t)(MEM32(edi)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(edi), 0xC (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00071FB9; /* je: equal / zero */
    }

loc_00071FAC:;
    edi = MEM32(esi + 0x14);
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    ecx = ecx + 8;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, edi (32-bit) */
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_00071FA5; /* jb: below (unsigned <) */
    }

loc_00071FB7:;
    goto loc_0007200A;

loc_00071FB9:;
    edx = MEM32(edx + eax * 8 + 4);
    ecx = MEM32(esi + 0x20);
    MEM32(ecx + 0x20) = edx;
    eax = MEM32(esi + 0x20);
    ecx = MEM32(eax + 0x20);
    SET_LO8(ebx, MEM8(ecx + 0x19));
    SET_LO8(edx, MEM8(ecx + 0x1A));
    xmm0 = XMM_SCALAR(MEMF(ecx + 0x10));   /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ecx + 8); /* subss */
    SET_LO8(eax, LO8(ebx));
    {
        int16_t _r = (int16_t)(int8_t)LO8(eax) * (int16_t)(int8_t)LO8(edx);
        SET_LO16(eax, (uint16_t)_r);
    }
    edx = ZX8(LO8(edx));
    MEM8(ecx + 0x1B) = LO8(eax);
    xmm1.f[0] = (float)(int32_t)edx;         /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0];       /* divss */
    MEMF(ecx + 0x1C) = xmm0.f[0];            /* movss */
    xmm0 = XMM_SCALAR(MEMF(ecx + 0x14));     /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ecx + 0xC); /* subss */
    eax = ZX8(LO8(ebx));
    xmm1.f[0] = (float)(int32_t)eax;   /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    MEMF(ecx + 0x20) = xmm0.f[0];      /* movss */

loc_0007200A:;
    ecx = MEM32(esi + 0x10);
    edx = ZX8(MEM8(ecx + 0x18));
    eax = MEM32(esi + 0x20);
    ecx = MEM32(edx * 8 + 0x1A51C0);
    MEM32(eax + 0x1C) = ecx;
    esi = MEM32(esi + 0x10);
    SET_LO8(eax, MEM8(esi + 0x1A));
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(3) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 3 (8-bit) */
    POP32(esp, ebp);
    POP32(esp, ebx);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00072032; /* je: equal / zero */
    }

loc_0007202A:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(7) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 7 (8-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00072032; /* je: equal / zero */
    }

loc_0007202E:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(8) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 8 (8-bit) */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_00072036; /* jne: not equal / not zero */
    }

loc_00072032:;
    MEM8(esi + 0x1A) = 5;

loc_00072036:;
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 4;
    return; /* ret */
}
