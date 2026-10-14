/* Persistent native debug hooks. Exclude from generated translation. */
/* These functions retain the Xbox calling convention: arguments live on
 * the guest stack and results in guest registers. loc_ labels identify XBE
 * addresses for disassembly lookup. Flag temporaries preserve x86 branches;
 * the host hooks are placed between the original instruction sequences. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "gen/recomp_funcs.h"
#include "title_guest_routines.h"
#include "debug_options.h"

void title_guest_check_warp_requirement(void)
{
    if (title_debug_warp_unlocked())
    {
        eax = 1;
        esp += 4;
        return;
    }

loc_00018320:;
    eax = MEM32(esp + 4);
    eax = eax + eax * 2;
    eax = MEM32(eax * 4 + 0x1982F4);
    esp += 4;
    return; /* ret */
}

void title_guest_check_snack_requirement(void)
{
    if (title_debug_snacks_unlocked())
    {
        eax = 1;
        esp += 4;
        return;
    }

    uint32_t flag_result = 0, flag_operand = 0;
    int32_t signed_flag_result = 0, signed_flag_operand = 0;
    (void)flag_result;
    (void)flag_operand;
    (void)signed_flag_result;
    (void)signed_flag_operand;
    int carry_flag = 0; /* carry flag */

loc_0001DCC0:;
    eax = MEM32(0x1BF2DC);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(MEM32(esp + 4)) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, MEM32(esp + 4) (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    eax = carry_flag ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sbb result */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    esp += 4;
    return; /* ret */
}

void title_guest_apply_player_damage(void)
{
    int debug_blocked = title_debug_monster_damage(MEM32(esp + 4), MEM32(esp));
    title_debug_damage_trace(MEM32(esp + 4), MEM32(esp), MEM32(esp + 8), debug_blocked);
    if (debug_blocked)
    {
        eax = 0;
        esp += 4;
        return;
    }

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

loc_0001D970:;
    eax = MEM32(0x1BF104);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0001D9E2; /* je: equal / zero */
    }

loc_0001D97B:;
    eax = MEM32(eax + 0x2C);
    ecx = MEM32(eax + 0xC);
    edx = MEM32(ecx + 8);
    eax = MEM32(edx + 4);
    eax = MEM32(eax + 4);
    edi = 0x181B40;
    esi = eax;
    ecx = 0xA;
    edx = 0; /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0001DB94; /* je: equal / zero */
    }

loc_0001D9A0:;
    edi = 0x181B34;
    esi = eax;
    ecx = 0xB;
    edx = 0; /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0001DB94; /* je: equal / zero */
    }

loc_0001D9B6:;
    edi = 0x181B2C;
    esi = eax;
    ecx = 8;
    edx = 0; /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0001DB94; /* je: equal / zero */
    }

loc_0001D9CC:;
    esi = eax;
    edi = 0x181B14;
    ecx = 0xC;
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0001DB94; /* je: equal / zero */
    }

loc_0001D9E2:;
    xmm1 = XMM_SCALAR(MEMF(0x1BF070)); /* movss */
    xmm0 = XMM_ZERO();                 /* xorps self = zero */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0001DB94; /* ja: above (unsigned >) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_0001D9F6:;
    xmm1 = XMM_SCALAR(MEMF(0x1BF074)); /* movss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0001DB94; /* ja: above (unsigned >) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_0001DA07:;
    ecx = MEM32(esp + 0xC);
    eax = MEM32(0x1BF010);
    edx = MEM32(0x1BF54C);
    MEM32(0x1BF064) = ecx;
    ecx = MEM32(esp + 0x10);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    if (CMP_A(flag_result, flag_operand))
    {
        ecx = eax; /* cmova */
    }
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0001DA30; /* je: equal / zero */
    }

loc_0001DA29:;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    eax = eax - ecx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    MEM32(0x1BF010) = eax;

loc_0001DA30:;
    PUSH32(esp, 0x0001DA35u);
    RECOMP_ABI_CALL(0x00068680u, sub_00068680); /* call 0x00068680 */

loc_0001DA35:;
    eax = MEM32(0x1BF010);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x180FF8));                      /* movss */
    MEMF(0x1BF060) = xmm0.f[0];                             /* movss */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0001DA75; /* je: equal / zero */
    }

loc_0001DA4E:;
    xmm0 = XMM_SCALAR(MEMF(0x180DB4)); /* movss */
    MEMF(0x1BF0A4) = xmm0.f[0];        /* movss */
    PUSH32(esp, 0x0001DA63u);
    RECOMP_ABI_CALL(0x000A5500u, sub_000A5500); /* call 0x000A5500 */

loc_0001DA63:;
    eax = eax >> 0x10;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* shift result */
    edx = 0;                                              /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    ecx = 6;
    {
        uint64_t _dividend = ((uint64_t)edx << 32) | eax;
        eax = (uint32_t)(_dividend / (uint32_t)ecx);
        edx = (uint32_t)(_dividend % (uint32_t)ecx);
    }
    MEM32(0x1BF0A8) = edx;

loc_0001DA75:;
    eax = MEM32(0x1BF010);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    MEM32(0x1BF068) = 2;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0001DB2A; /* jne: not equal / not zero */
    }

loc_0001DA8C:;
    eax = MEM32(0x1BF000);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x180DC8));                      /* movss */
    MEMF(0x1BF070) = xmm0.f[0];                             /* movss */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0001DB13; /* je: equal / zero */
    }

loc_0001DAA5:;
    eax = MEM32(0x1BEF60);
    xmm0 = XMM_SCALAR(MEMF(0x1907D8)); /* movss */
    MEM32(0x1BF000) = 0;
    MEMF(eax + 0xE0) = xmm0.f[0];      /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1907DC)); /* movss */
    flag_operand = (uint32_t)(0xE0) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    eax = eax + 0xE0;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEMF(eax + 4) = xmm0.f[0];                            /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1907E0));                    /* movss */
    MEMF(eax + 8) = xmm0.f[0];                            /* movss */
    eax = MEM32(0x1BEF60);
    xmm0 = XMM_SCALAR(MEMF(0x1907D8)); /* movss */
    flag_operand = (uint32_t)(0x8C) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    eax = eax + 0x8C;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEMF(eax) = xmm0.f[0];                                /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1907DC));                    /* movss */
    MEMF(eax + 4) = xmm0.f[0];                            /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1907E0));                    /* movss */
    MEMF(eax + 8) = xmm0.f[0];                            /* movss */

loc_0001DB13:;
    PUSH32(esp, 0);
    PUSH32(esp, 0x33);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 0x0001DB20u);
    RECOMP_ABI_CALL(0x000140D0u, sub_000140D0); /* call 0x000140D0 */

loc_0001DB20:;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x0001DB28u);
    RECOMP_ABI_CALL(0x0001D930u, sub_0001D930); /* call 0x0001D930 */

loc_0001DB28:;
    goto loc_0001DB3A;

loc_0001DB2A:;
    xmm0 = XMM_SCALAR(MEMF(0x180DF0)); /* movss */
    MEMF(0x1BF070) = xmm0.f[0];        /* movss */

loc_0001DB3A:;
    eax = MEM32(0x1BF1F4);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0001DB87; /* je: equal / zero */
    }

loc_0001DB43:;
    edx = MEM32(eax + 0xEC);
    ecx = MEM32(edx + 0x18);
    xmm0 = XMM_ZERO();                 /* xorps self = zero */
    xmm1 = XMM_SCALAR(MEMF(0x180B48)); /* movss */
    MEM32(eax + 0x120) = ecx;
    MEM32(0x1BF1F4) = 0;
    MEMF(0x1BF424) = xmm0.f[0]; /* movss */
    MEMF(0x1BF428) = xmm1.f[0]; /* movss */
    MEMF(0x1BF42C) = xmm0.f[0]; /* movss */
    MEMF(0x1BF2AC) = xmm0.f[0]; /* movss */

loc_0001DB87:;
    PUSH32(esp, 0x0001DB8Cu);
    RECOMP_ABI_CALL(0x000781F0u, sub_000781F0); /* call 0x000781F0 */

loc_0001DB8C:;
    POP32(esp, edi);
    eax = 1;
    POP32(esp, esi);
    esp += 4;
    return; /* ret */

loc_0001DB94:;
    POP32(esp, edi);
    eax = 0; /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    POP32(esp, esi);
    esp += 4;
    return; /* ret */
}

void title_guest_update_player_with_debug_motion(void)
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

loc_00028CB0:;
    eax = MEM32(0x1AF714);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_00028DA7; /* je: equal / zero */
    }

loc_00028CBD:;
    PUSH32(esp, 8);
    PUSH32(esp, 0x1BEF20);
    PUSH32(esp, 0x00028CC9u);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_00028CC9:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x00028CD1u);
    RECOMP_ABI_CALL(0x00060740u, sub_00060740); /* call 0x00060740 */

loc_00028CD1:;
    xmm0 = XMM_SCALAR(MEMF(0x1AF71C));     /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(esp + 4); /* subss */
    xmm1 = XMM_ZERO();                     /* xorps self = zero */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1AF71C) = xmm0.f[0];      /* movss */
    if ((float_compare_left < float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_00028DA7; /* jb: below (unsigned <) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_00028CF3:;
    eax = MEM32(0x1AF718);
    PUSH32(esp, eax);
    MEMF(0x1AF71C) = xmm1.f[0]; /* movss */
    PUSH32(esp, 0x00028D06u);
    RECOMP_ABI_CALL(0x00017610u, sub_00017610); /* call 0x00017610 */

loc_00028D06:;
    ecx = MEM32(0x1AF1DC);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 2);
    PUSH32(esp, 0x80);
    PUSH32(esp, 0);
    PUSH32(esp, 0x3F800000);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00028D24u);
    RECOMP_ABI_CALL(0x000951D0u, sub_000951D0); /* call 0x000951D0 */

loc_00028D24:;
    fp_push(MEMF(esp + 0x24));                          /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x180F78)); /* fmul dword ptr [0x180f78] */
    flag_operand = (uint32_t)(0x20) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0x20;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x1AF720));   /* fadd dword ptr [0x1af720] */
    MEMF(0x1AF720) = (float)fp_top();                     /* fst */
    PUSH32(esp, 0x00028D42u);
    RECOMP_ABI_CALL(0x000F16C8u, sub_000F16C8); /* call 0x000F16C8 */

loc_00028D42:;
    ecx = MEM32(0x1AF714);
    edx = MEM32(0x1BF2DC);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ecx (32-bit) */
    if (CMP_A(flag_result, flag_operand))
    {
        eax = ecx; /* cmova */
    }
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    if (!title_debug_snacks_unlocked())
    {
        edx = edx - eax;
    }
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    if ((signed_flag_result >= 0))
    {
        goto loc_00028D6D; /* jns: not sign (positive) */
    }

loc_00028D57:;
    MEM32(0x1BF2DC) = 0;
    MEM32(0x1AF714) = 0;
    goto loc_00028D7D;

loc_00028D6D:;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    ecx = ecx - eax;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    MEM32(0x1BF2DC) = edx;
    MEM32(0x1AF714) = ecx;
    if ((flag_result != 0))
    {
        goto loc_00028DA7; /* jne: not equal / not zero */
    }

loc_00028D7D:;
    PUSH32(esp, 9);
    PUSH32(esp, 0x1BEF20);
    PUSH32(esp, 0x00028D89u);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_00028D89:;
    edx = MEM32(0x1AF718);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00028D95u);
    RECOMP_ABI_CALL(0x00018460u, sub_00018460); /* call 0x00018460 */

loc_00028D95:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x00028D9Du);
    RECOMP_ABI_CALL(0x00060760u, sub_00060760); /* call 0x00060760 */

loc_00028D9D:;
    MEM32(0x1AF718) = 0;

loc_00028DA7:;
    esp += 4;
    return; /* ret */

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}

void title_guest_pay_gate_snacks(void)
{
    if (title_debug_player_flight())
    {
        esp += 4;
        return;
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

loc_0002AD70:;
    eax = MEM32(esp + 0xC);
    flag_operand = (uint32_t)(0x90) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    carry_flag = (int)((uint32_t)(esp) < (uint32_t)(0x90));
    esp = esp - 0x90;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002AD80u);
    RECOMP_ABI_CALL(0x00028CB0u, sub_00028CB0); /* call 0x00028CB0 */

loc_0002AD80:;
    eax = MEM32(0x1BF0D8);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002AD96; /* je: equal / zero */
    }

loc_0002AD8C:;
    ecx = MEM32(0x1BF588);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002ADA6; /* je: equal / zero */
    }

loc_0002AD96:;
    xmm0 = XMM_SCALAR(MEMF(0x180B48)); /* movss */
    MEMF(0x1BF074) = xmm0.f[0];        /* movss */

loc_0002ADA6:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    MEM32(0x1BF018) = 0;
    ecx = 3;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002ADCD; /* je: equal / zero */
    }

loc_0002ADBD:;
    edx = MEM32(0x1BEF10);
    eax = MEM32(edx + 0x2C);
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x20) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x20 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0002ADD5; /* jne: not equal / not zero */
    }

loc_0002ADCB:;
    goto loc_0002AE11;

loc_0002ADCD:;
    flag_result = (uint32_t)(MEM32(0x1BF0E0)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF0E0), ecx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AE11; /* jne: not equal / not zero */
    }

loc_0002ADD5:;
    eax = MEM32(0x1AF72C);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    SET_LO8(ebx, MEM8(0x1BF2E0));
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AE17; /* jne: not equal / not zero */
    }

loc_0002ADE4:;
    flag_result = (uint32_t)(LO8(ebx)) & 0xFFu;
    flag_operand = (uint32_t)(0x40) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(ebx), 0x40 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002ADF1; /* je: equal / zero */
    }

loc_0002ADE9:;
    MEM32(0x1BF018) = ecx;
    goto loc_0002AE07;

loc_0002ADF1:;
    flag_result = (uint32_t)(LO8(ebx)) & 0xFFu;
    flag_operand = (uint32_t)(0x10) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(ebx), 0x10 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002AE17; /* je: equal / zero */
    }

loc_0002ADF6:;
    flag_result = (uint32_t)(LO8(ebx)) & 0xFFu;
    flag_operand = (uint32_t)(0x20) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(ebx), 0x20 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    eax = 0;
    SET_LO8(eax, (TEST_NZ(flag_result, flag_operand)) ? 1 : 0); /* setne */
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x1BF018) = eax;

loc_0002AE07:;
    PUSH32(esp, 0);
    PUSH32(esp, 0x0002AE0Eu);
    RECOMP_ABI_CALL(0x00067AF0u, sub_00067AF0); /* call 0x00067AF0 */

loc_0002AE0E:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002AE11:;
    SET_LO8(ebx, MEM8(0x1BF2E0));

loc_0002AE17:;
    eax = MEM32(0x1BF068);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002AE26; /* je: equal / zero */
    }

loc_0002AE20:;
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(0x1BF068) = eax;

loc_0002AE26:;
    ecx = MEM32(0x1BEF4C);
    edx = MEM32(ecx + 0xC);
    eax = MEM32(edx + 8);
    ebp = MEM32(eax + 4);
    eax = MEM32(ebp + 4);
    edi = 0x185068;
    esi = eax;
    ecx = 9;
    carry_flag = 0; /* xor clears CF */
    edx = 0;        /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            carry_flag = (MEM8(esi) < MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0002AE80; /* je: equal / zero */
    }

loc_0002AE4A:;
    edi = 0x185058;
    esi = eax;
    ecx = 0xD;
    carry_flag = 0; /* xor clears CF */
    edx = 0;        /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            carry_flag = (MEM8(esi) < MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0002AE80; /* je: equal / zero */
    }

loc_0002AE5C:;
    edi = 0x18504C;
    esi = eax;
    ecx = 9;
    carry_flag = 0; /* xor clears CF */
    edx = 0;        /* xor self */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            carry_flag = (MEM8(esi) < MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag != 0))
    {
        goto loc_0002AE80; /* je: equal / zero */
    }

loc_0002AE6E:;
    esi = eax;
    edi = 0x18503C;
    ecx = 0xD;
    carry_flag = 0; /* xor clears CF */
    eax = 0;        /* xor self */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    zero_flag = ((flag_result == 0)) ? 1 : 0;             /* ZF in: a zero count keeps it */
    {
        int32_t direction_step = RECOMP_DF_STEP(1);
        while (ecx != 0)
        {
            zero_flag = (MEM8(esi) == MEM8(edi));
            carry_flag = (MEM8(esi) < MEM8(edi));
            esi += direction_step;
            edi += direction_step;
            ecx--;
            if (!zero_flag)
            {
                break;
            }
        }
    } /* repe cmpsb */
    if ((zero_flag == 0))
    {
        goto loc_0002AEDE; /* jne: not equal / not zero */
    }

loc_0002AE80:;
    ecx = MEM32(0x1BEF10);
    eax = MEM32(ecx + 0x30);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x30000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test eax, 0x30000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0002AEA0; /* jne: not equal / not zero */
    }

loc_0002AE90:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xC0000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test eax, 0xC0000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002AEDE; /* je: equal / zero */
    }

loc_0002AE97:;
    eax = MEM32(0x1BF2CC);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AEDE; /* jne: not equal / not zero */
    }

loc_0002AEA0:;
    eax = MEM32(0x1BF180);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AEDE; /* jne: not equal / not zero */
    }

loc_0002AEA9:;
    eax = MEM32(0x1BF188);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AEDE; /* jne: not equal / not zero */
    }

loc_0002AEB2:;
    PUSH32(esp, 0xE);
    PUSH32(esp, 0x0002AEB9u);
    RECOMP_ABI_CALL(0x00067AF0u, sub_00067AF0); /* call 0x00067AF0 */

loc_0002AEB9:;
    edx = MEM32(ebp + 4);
    SET_LO8(eax, MEM8(edx + 2));
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x31) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x31 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AFCA; /* jne: not equal / not zero */
    }

loc_0002AECA:;
    eax = MEM32(0x1BF104);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002AED5u);
    RECOMP_ABI_CALL(0x00033A90u, sub_00033A90); /* call 0x00033A90 */

loc_0002AED5:;
    SET_LO8(ebx, MEM8(0x1BF2E0));
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002AEDE:;
    flag_result = (uint32_t)(MEM32(0x1BF188)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF188), 2 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AEF1; /* jne: not equal / not zero */
    }

loc_0002AEE7:;
    MEM32(0x1BF188) = 3;

loc_0002AEF1:;
    eax = MEM32(0x1BF0D8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002AFD5; /* je: equal / zero */
    }

loc_0002AEFE:;
    ecx = MEM32(0x1BEF10);
    eax = MEM32(ecx + 0x2C);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AFD5; /* jne: not equal / not zero */
    }

loc_0002AF0F:;
    edx = MEM32(0x1BEF10);
    eax = (uint32_t)(int32_t)SMEM8(edx + 0x38);
    ecx = ZX16(MEM16(0x1BF430));
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    carry_flag = 0;                            /* logical op clears CF */
    eax = eax ^ edx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_operand = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    carry_flag = (int)((uint32_t)(eax) < (uint32_t)(edx));
    eax = eax - edx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ecx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_G(signed_flag_result, signed_flag_operand))
    {
        goto loc_0002AFD5; /* jg: greater (signed >) */
    }

loc_0002AF2D:;
    eax = MEM32(0x1BEF10);
    eax = (uint32_t)(int32_t)SMEM8(eax + 0x39);
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    carry_flag = 0;                            /* logical op clears CF */
    eax = eax ^ edx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_operand = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    carry_flag = (int)((uint32_t)(eax) < (uint32_t)(edx));
    eax = eax - edx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ecx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_G(signed_flag_result, signed_flag_operand))
    {
        goto loc_0002AFD5; /* jg: greater (signed >) */
    }

loc_0002AF43:;
    ecx = MEM32(0x1BEF4C);
    edx = MEM32(ecx + 0xC);
    eax = MEM32(edx + 8);
    eax = MEM32(eax + 4);
    carry_flag = 0; /* xor clears CF */
    esi = 0;        /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002AFFA; /* je: equal / zero */
    }

loc_0002AF5C:;
    flag_result = (uint32_t)(MEM8(eax + 0x10)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(eax + 0x10), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002AFFA; /* je: equal / zero */
    }

loc_0002AF66:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF0C0));   /* movss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0xAC)); /* movss */
    xmm2 = XMM_SCALAR(MEMF(0x1BF0C4));   /* movss */
    flag_result = (uint32_t)(MEM32(0x1AF72C)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1AF72C), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    xmm2.f[0] = xmm2.f[0] + xmm1.f[0]; /* addss */
    MEMF(0x1BF0C0) = xmm0.f[0];        /* movss */
    MEMF(0x1BF0C4) = xmm2.f[0];        /* movss */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002AFB3; /* jne: not equal / not zero */
    }

loc_0002AF9F:;
    xmm2 = XMM_SCALAR(MEMF(0x1BF0C8)); /* movss */
    xmm2.f[0] = xmm2.f[0] + xmm1.f[0]; /* addss */
    MEMF(0x1BF0C8) = xmm2.f[0];        /* movss */

loc_0002AFB3:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002AFFA; /* jp: parity */
    }

loc_0002AFC0:;
    xmm0 = XMM_SCALAR(MEMF(0x180FDC)); /* movss */
    goto loc_0002AFF2;

loc_0002AFCA:;
    SET_LO8(ebx, MEM8(0x1BF2E0));
    goto loc_0002AEE7;

loc_0002AFD5:;
    xmm0 = XMM_ZERO();                 /* xorps self = zero */
    xmm1 = XMM_SCALAR(MEMF(0x185038)); /* movss */
    MEMF(0x1BF0C4) = xmm1.f[0];        /* movss */
    MEMF(0x1BF0C8) = xmm0.f[0];        /* movss */
    carry_flag = 0;                    /* xor clears CF */
    esi = 0;                           /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */

loc_0002AFF2:;
    MEMF(0x1BF0C0) = xmm0.f[0]; /* movss */

loc_0002AFFA:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF08C)); /* movss */
    carry_flag = 0;                    /* logical op clears CF */
    SET_LO8(ebx, LO8(ebx) & 0x40);
    flag_result = (uint32_t)(LO8(ebx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result); /* and result */
    carry_flag = (int)((LO8(ebx)) != 0);
    SET_LO8(ebx, (uint32_t)(-(int32_t)LO8(ebx)));
    flag_result = (uint32_t)(LO8(ebx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result); /* neg result */
    ebx = carry_flag ? 0xFFFFFFFF : 0;                   /* sbb self (CF extend) */
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sbb result */
    carry_flag = (int)((ebx) != 0);
    ebx = (uint32_t)(-(int32_t)ebx);
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* neg result */
    flag_operand = (uint32_t)(5) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ebx) + (uint64_t)(5)) >> 32) & 1);
    ebx = ebx + 5;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    MEM32(0x1BF07C) = ebx;
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002B0B4; /* jnp: not parity */
    }

loc_0002B025:;
    PUSH32(esp, 1);
    PUSH32(esp, 0x0002B02Cu);
    RECOMP_ABI_CALL(0x00067AF0u, sub_00067AF0); /* call 0x00067AF0 */

loc_0002B02C:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF08C)); /* movss */
    eax = MEM32(0x1BF0D8);
    xmm1 = XMM_SCALAR(MEMF(esp + 0xB0)); /* movss */
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    MEMF(0x1BF08C) = xmm0.f[0];        /* movss */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B0A9; /* je: equal / zero */
    }

loc_0002B055:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x1BF510); /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0002B0A9; /* ja: above (unsigned >) (xmm0.f[0] vs MEMF(0x1BF510)) */
    }

loc_0002B05E:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x1BF50C); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B076; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x1BF50C)) */
    }

loc_0002B067:;
    ecx = MEM32(0x1BEF10);
    flag_result = (uint32_t)(MEM32(ecx + 0x2C)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x80000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(ecx + 0x2C), 0x80000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002B0A9; /* je: equal / zero */
    }

loc_0002B076:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF508));      /* movss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0];      /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x1BF088); /* addss */
    xmm1 = XMM_SCALAR(MEMF(0x1BF504));      /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm1.f[0]; /* comiss */
    MEMF(0x1BF088) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B0B4; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm1.f[0]) */
    }

loc_0002B09F:;
    MEMF(0x1BF088) = xmm1.f[0]; /* movss */
    goto loc_0002B0B4;

loc_0002B0A9:;
    xmm0 = XMM_ZERO();          /* xorps self = zero */
    MEMF(0x1BF08C) = xmm0.f[0]; /* movss */

loc_0002B0B4:;
    xmm0 = XMM_SCALAR(MEMF(esp + 0xAC)); /* movss */
    PUSH32(esp, 0x0002B0C2u);
    RECOMP_ABI_CALL(0x00022BD0u, sub_00022BD0); /* call 0x00022BD0 */

loc_0002B0C2:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF060)); /* movss */
    xmm3 = XMM_ZERO();                 /* xorps self = zero */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm3.f[0];     /* comiss */
    xmm2 = XMM_SCALAR(MEMF(esp + 0xAC)); /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B0F4; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm3.f[0]) */
    }

loc_0002B0DB:;
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1BF060) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B0F4; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */
    }

loc_0002B0EC:;
    MEMF(0x1BF060) = xmm3.f[0]; /* movss */

loc_0002B0F4:;
    xmm1 = XMM_SCALAR(MEMF(0x1BF070)); /* movss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm3.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B123; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm3.f[0]) */
    }

loc_0002B101:;
    xmm1.f[0] = xmm1.f[0] - xmm2.f[0]; /* subss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm1.f[0]; /* comiss */
    MEMF(0x1BF070) = xmm1.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B123; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm1.f[0]) */
    }

loc_0002B112:;
    xmm1 = xmm3;                /* movaps */
    MEMF(0x1BF070) = xmm1.f[0]; /* movss */
    MEM32(0x1BF0D4) = esi;

loc_0002B123:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF074)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm3.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B149; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm3.f[0]) */
    }

loc_0002B130:;
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1BF074) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B149; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */
    }

loc_0002B141:;
    MEMF(0x1BF074) = xmm3.f[0]; /* movss */

loc_0002B149:;
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm3.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002B176; /* jnp: not parity */
    }

loc_0002B152:;
    flag_result = (uint32_t)(MEM32(0x1BF010)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF010), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B176; /* je: equal / zero */
    }

loc_0002B15A:;
    eax = MEM32(0x1BF0CC);
    eax++;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 4 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    MEM32(0x1BF0CC) = eax;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B17C; /* jne: not equal / not zero */
    }

loc_0002B16A:;
    MEM32(0x1BF0CC) = 0xFFFFFFFEu;
    goto loc_0002B17C;

loc_0002B176:;
    MEM32(0x1BF0CC) = esi;

loc_0002B17C:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF06C)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm3.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B1A2; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm3.f[0]) */
    }

loc_0002B189:;
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1BF06C) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B1A2; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */
    }

loc_0002B19A:;
    MEMF(0x1BF06C) = xmm3.f[0]; /* movss */

loc_0002B1A2:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF084)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm3.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B1C8; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm3.f[0]) */
    }

loc_0002B1AF:;
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1BF084) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B1C8; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */
    }

loc_0002B1C0:;
    MEMF(0x1BF084) = xmm3.f[0]; /* movss */

loc_0002B1C8:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF078)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm3.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B1EE; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm3.f[0]) */
    }

loc_0002B1D5:;
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1BF078) = xmm0.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B1EE; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */
    }

loc_0002B1E6:;
    MEMF(0x1BF078) = xmm3.f[0]; /* movss */

loc_0002B1EE:;
    PUSH32(esp, 0x0002B1F3u);
    RECOMP_ABI_CALL(0x0001C150u, sub_0001C150); /* call 0x0001C150 */

loc_0002B1F3:;
    flag_result = (uint32_t)(MEM32(0x1BF010)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF010), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    MEM8(esp + 0x24) = 0;
    MEM8(esp + 0x25) = 0;
    MEM8(esp + 0x26) = 0;
    MEM8(esp + 0x27) = 0xFF;
    MEM8(esp + 0x1C) = 0;
    MEM8(esp + 0x1D) = 0;
    MEM8(esp + 0x1E) = 0;
    MEM8(esp + 0x1F) = 0;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B2DF; /* jne: not equal / not zero */
    }

loc_0002B227:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF070)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180B48); /* comiss */
    if ((float_compare_left < float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B264; /* jb: below (unsigned <) (xmm0.f[0] vs MEMF(0x180B48)) */
    }

loc_0002B238:;
    PUSH32(esp, 0x0002B23Du);
    RECOMP_ABI_CALL(0x00093240u, sub_00093240); /* call 0x00093240 */

loc_0002B23D:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B261; /* jne: not equal / not zero */
    }

loc_0002B241:;
    PUSH32(esp, 1);
    PUSH32(esp, 1);
    PUSH32(esp, 0x1CDE0);
    PUSH32(esp, 0x3F800000);
    edx = esp + 0x34;
    PUSH32(esp, edx);
    eax = esp + 0x30;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002B25Eu);
    RECOMP_ABI_CALL(0x00093190u, sub_00093190); /* call 0x00093190 */

loc_0002B25E:;
    flag_operand = (uint32_t)(0x18) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x18)) >> 32) & 1);
    esp = esp + 0x18;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002B261:;
    xmm3 = XMM_ZERO(); /* xorps self = zero */

loc_0002B264:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF070)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm3.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002B2C1; /* jp: parity */
    }

loc_0002B275:;
    PUSH32(esp, 0x0002B27Au);
    RECOMP_ABI_CALL(0x00078200u, sub_00078200); /* call 0x00078200 */

loc_0002B27A:;
    PUSH32(esp, 2);
    PUSH32(esp, 0x0002B281u);
    RECOMP_ABI_CALL(0x0005F2F0u, sub_0005F2F0); /* call 0x0005F2F0 */

loc_0002B281:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, 0x0002B289u);
    RECOMP_ABI_CALL(0x0005CBE0u, sub_0005CBE0); /* call 0x0005CBE0 */

loc_0002B289:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B2A0; /* jne: not equal / not zero */
    }

loc_0002B28D:;
    MEM8(0x1C2030) = 1;
    MEM8(0x1C202B) = 1;
    PUSH32(esp, 0x0002B2A0u);
    RECOMP_ABI_CALL(0x0005CBF0u, sub_0005CBF0); /* call 0x0005CBF0 */

loc_0002B2A0:;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEM32(0x1BF010) = 3;
    MEM32(0x1AF6D8) = esi;
    MEMF(0x1AF6D4) = xmm0.f[0]; /* movss */
    MEM32(0x1AF6DC) = esi;

loc_0002B2C1:;
    PUSH32(esp, 0x0002B2C6u);
    RECOMP_ABI_CALL(0x0005CBE0u, sub_0005CBE0); /* call 0x0005CBE0 */

loc_0002B2C6:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B2DF; /* je: equal / zero */
    }

loc_0002B2CA:;
    PUSH32(esp, 2);
    PUSH32(esp, esi);
    PUSH32(esp, 0x185028);
    PUSH32(esp, 0x42C80000);
    PUSH32(esp, 0x0002B2DCu);
    RECOMP_ABI_CALL(0x0005D6F0u, sub_0005D6F0); /* call 0x0005D6F0 */

loc_0002B2DC:;
    flag_operand = (uint32_t)(0x10) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x10)) >> 32) & 1);
    esp = esp + 0x10;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002B2DF:;
    ebx = MEM32(esp + 0xA8);
    ebp = MEM32(esp + 0xA4);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002B2F4u);
    RECOMP_ABI_CALL(0x00020FE0u, sub_00020FE0); /* call 0x00020FE0 */

loc_0002B2F4:;
    ecx = MEM32(esp + 0xB4);
    xmm0 = XMM_SCALAR(MEMF(esp + 0xB4)); /* movss */
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    MEMF(0x1AF1B4) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0002B314u);
    RECOMP_ABI_CALL(0x000860C0u, sub_000860C0); /* call 0x000860C0 */

loc_0002B314:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF09C)); /* movss */
    flag_operand = (uint32_t)(0x14) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x14)) >> 32) & 1);
    esp = esp + 0x14;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002B481; /* jnp: not parity */
    }

loc_0002B330:;
    xmm4 = XMM_SCALAR(MEMF(esp + 0xAC));    /* movss */
    xmm4.f[0] = xmm4.f[0] * MEMF(0x181284); /* mulss */
    xmm4.f[0] = xmm4.f[0] + xmm0.f[0];      /* addss */
    xmm0 = XMM_SCALAR(MEMF(0x18227C));      /* movss */
    float_compare_left = xmm4.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    MEMF(0x1BF09C) = xmm4.f[0];      /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B365; /* jbe: below or equal (unsigned <=) (xmm4.f[0] vs xmm0.f[0]) */
    }

loc_0002B35A:;
    xmm4 = xmm0;                /* movaps */
    MEMF(0x1BF09C) = xmm4.f[0]; /* movss */

loc_0002B365:;
    xmm5 = XMM_SCALAR(MEMF(0x1BF0A0)); /* movss */
    carry_flag = 0;                    /* xor clears CF */
    esi = 0;                           /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    xmm3 = xmm4;                                          /* movaps */
    carry_flag = 0;                                       /* xor clears CF */
    edi = 0;                                              /* xor self */
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_result = (uint32_t)(MEM16(ebx + 0xA)) & 0xFFFFu;
    flag_operand = (uint32_t)(LO16(esi)) & 0xFFFFu;
    signed_flag_result = (int32_t)(int16_t)(flag_result);
    signed_flag_operand = (int32_t)(int16_t)(flag_operand); /* cmp MEM16(ebx + 0xA), LO16(esi) (16-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    xmm3.f[0] = xmm3.f[0] * xmm4.f[0]; /* mulss */
    xmm5.f[0] = xmm5.f[0] * xmm5.f[0]; /* mulss */
    MEMF(esp + 0x10) = xmm3.f[0];      /* movss */
    MEMF(esp + 0x20) = xmm5.f[0];      /* movss */
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002B461; /* jbe: below or equal (unsigned <=) */
    }

loc_0002B392:;
    edx = MEM32(ebx + 0x34);
    edx = MEM32(edx + edi * 4);
    eax = MEM32(edx + 0x40);
    ecx = MEM32(ebp + 0x40);
    fp_push(MEMF(eax + 0x34));                            /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(ecx + 0x34)); /* fsub dword ptr [ecx + 0x34] */
    xmm1 = XMM_SCALAR(MEMF(eax + 0x38));                  /* movss */
    xmm1.f[0] = xmm1.f[0] - MEMF(ecx + 0x38);             /* subss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x30));                  /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ecx + 0x30);             /* subss */
    xmm2 = xmm1;                                          /* movaps */
    xmm2.f[0] = xmm2.f[0] * xmm1.f[0];                    /* mulss */
    xmm1 = XMM_SCALAR(MEMF(eax + 0x88));                  /* movss */
    xmm1.f[0] = xmm1.f[0] - MEMF(ecx + 0x38);             /* subss */
    xmm0.f[0] = xmm0.f[0] * xmm0.f[0];                    /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0];                    /* addss */
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm0.f[0];          /* comiss */
    xmm2 = XMM_SCALAR(MEMF(eax + 0x80));      /* movss */
    xmm2.f[0] = xmm2.f[0] - MEMF(ecx + 0x30); /* subss */
    fp_top() = fabs(fp_top());                /* fabs */
    xmm6 = xmm1;                              /* movaps */
    xmm2.f[0] = xmm2.f[0] * xmm2.f[0];        /* mulss */
    xmm6.f[0] = xmm6.f[0] * xmm1.f[0];        /* mulss */
    MEMF(esp + 0x18) = (float)fp_top();
    fp_pop();                          /* fstp */
    xmm2.f[0] = xmm2.f[0] + xmm6.f[0]; /* addss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B400; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */
    }

loc_0002B3FB:;
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm5.f[0]; /* comiss */
    if ((float_compare_left >= float_compare_right))
    {
        goto loc_0002B40A; /* jae: above or equal (unsigned >=) (xmm0.f[0] vs xmm5.f[0]) */
    }

loc_0002B400:;
    float_compare_left = xmm3.f[0];
    float_compare_right = xmm2.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B44C; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm2.f[0]) */
    }

loc_0002B405:;
    float_compare_left = xmm2.f[0];
    float_compare_right = xmm5.f[0]; /* comiss */
    if ((float_compare_left < float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B44C; /* jb: below (unsigned <) (xmm2.f[0] vs xmm5.f[0]) */
    }

loc_0002B40A:;
    xmm0 = XMM_SCALAR(MEMF(0x180B44)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(esp + 0x18); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B44C; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(esp + 0x18)) */
    }

loc_0002B419:;
    fp_push(MEMF(esp + 0x10)); /* fld float */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(sqrt(fp_top()));            /* fsqrt */
    fp_top() = RECOMP_FP_PC(MEMF(0x18227C) - fp_top()); /* fsubr dword ptr [0x18227c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x185024)); /* fmul dword ptr [0x185024] */
    MEMF(esp) = (float)fp_top();
    fp_pop(); /* fstp */
    PUSH32(esp, edx);
    PUSH32(esp, 0x0002B435u);
    RECOMP_ABI_CALL(0x0004CF30u, sub_0004CF30); /* call 0x0004CF30 */

loc_0002B435:;
    xmm4 = XMM_SCALAR(MEMF(0x1BF09C));   /* movss */
    xmm5 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    xmm3 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002B44C:;
    eax = ZX16(MEM16(ebx + 0xA));
    edi++;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002B392; /* jb: below (unsigned <) */
    }

loc_0002B459:;
    xmm0 = XMM_SCALAR(MEMF(0x18227C)); /* movss */

loc_0002B461:;
    float_compare_left = xmm4.f[0];
    float_compare_right = xmm0.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002B477; /* jp: parity */
    }

loc_0002B46A:;
    xmm0 = XMM_ZERO();          /* xorps self = zero */
    MEMF(0x1BF09C) = xmm0.f[0]; /* movss */
    goto loc_0002B483;

loc_0002B477:;
    MEMF(0x1BF0A0) = xmm4.f[0]; /* movss */
    goto loc_0002B483;

loc_0002B481:;
    carry_flag = 0; /* xor clears CF */
    esi = 0;        /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */

loc_0002B483:;
    PUSH32(esp, 0x0002B488u);
    RECOMP_ABI_CALL(0x00014530u, sub_00014530); /* call 0x00014530 */

loc_0002B488:;
    flag_result = (uint32_t)(MEM32(0x1BF1F4)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF1F4), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    eax = 1;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B49A; /* jne: not equal / not zero */
    }

loc_0002B495:;
    eax = 2;

loc_0002B49A:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    xmm0 = XMM_SCALAR(MEMF(esp + 0xAC)); /* movss */
    xmm1.f[0] = (float)(int32_t)eax;     /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0];   /* divss */
    MEMF(esp + 0xAC) = xmm0.f[0];        /* movss */
    if (CMP_LE(signed_flag_result, signed_flag_operand))
    {
        goto loc_0002B8DC; /* jle: less or equal (signed <=) */
    }

loc_0002B4BC:;
    edi = MEM32(esp + 0xAC);
    MEM32(esp + 0x18) = eax;

loc_0002B4C7:;
    ecx = MEM32(ebp + 0x40);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ecx) + (uint64_t)(0x30)) >> 32) & 1);
    ecx = ecx + 0x30;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    edx = MEM32(ecx);
    eax = MEM32(ecx + 4);
    ecx = MEM32(ecx + 8);
    PUSH32(esp, edi);
    MEM32(esp + 0x30) = edx;
    MEM32(esp + 0x34) = eax;
    MEM32(esp + 0x38) = ecx;
    PUSH32(esp, 0x0002B4E7u);
    RECOMP_ABI_CALL(0x0001A730u, sub_0001A730); /* call 0x0001A730 */

loc_0002B4E7:;
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002B4EFu);
    RECOMP_ABI_CALL(0x000865C0u, sub_000865C0); /* call 0x000865C0 */

loc_0002B4EF:;
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002B4F7u);
    RECOMP_ABI_CALL(0x00086530u, sub_00086530); /* call 0x00086530 */

loc_0002B4F7:;
    edx = MEM32(ebp + 0x40);
    flag_operand = (uint32_t)(0xC8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(edx) + (uint64_t)(0xC8)) >> 32) & 1);
    edx = edx + 0xC8;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = MEM32(edx);
    ecx = MEM32(edx + 4);
    edx = MEM32(edx + 8);
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, 0x1BF318);
    MEM32(0x1AF484) = eax;
    MEM32(0x1AF488) = ecx;
    MEM32(0x1AF48C) = edx;
    PUSH32(esp, 0x0002B525u);
    RECOMP_ABI_CALL(0x00088CF0u, sub_00088CF0); /* call 0x00088CF0 */

loc_0002B525:;
    eax = MEM32(0x1BF094);
    flag_operand = (uint32_t)(0x28) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x28)) >> 32) & 1);
    esp = esp + 0x28;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B55D; /* je: equal / zero */
    }

loc_0002B531:;
    flag_result = (uint32_t)(MEM32(eax + 0xC)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x34860) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(eax + 0xC), 0x34860 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B55D; /* jne: not equal / not zero */
    }

loc_0002B53A:;
    eax = MEM32(eax + 0x2C);
    edx = MEM32(eax + 0x20);
    ecx = MEM32(0x1BEF60);
    xmm0 = XMM_SCALAR(MEMF(edx + 0x34));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(0x1BF098);   /* subss */
    xmm0.f[0] = xmm0.f[0] + MEMF(ecx + 0x34); /* addss */
    MEMF(ecx + 0x34) = xmm0.f[0];             /* movss */

loc_0002B55D:;
    eax = MEM32(0x1BF88C);
    flag_result = (uint32_t)(MEM32(eax + 0xEC)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(eax + 0xEC), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B609; /* je: equal / zero */
    }

loc_0002B56E:;
    esi = MEM32(ebp + 0xDC);
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B60F; /* je: equal / zero */
    }

loc_0002B57C:;
    /* nop */

loc_0002B580:;
    flag_result = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x5F6E0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(esi + 4), 0x5F6E0 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B5FC; /* jne: not equal / not zero */
    }

loc_0002B589:;
    flag_result = (uint32_t)(MEM32(0x1BF034)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(3) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF034), 3 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B5F3; /* je: equal / zero */
    }

loc_0002B592:;
    xmm0 = XMM_SCALAR(MEMF(0x1AF730)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002B5F3; /* jp: parity */
    }

loc_0002B5A7:;
    PUSH32(esp, esi);
    PUSH32(esp, 0x0002B5ADu);
    RECOMP_ABI_CALL(0x0008B4D0u, sub_0008B4D0); /* call 0x0008B4D0 */

loc_0002B5AD:;
    eax = MEM32(0x1AF6A4);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B5FC; /* je: equal / zero */
    }

loc_0002B5B9:;
    carry_flag = 0; /* xor clears CF */
    ecx = 0;        /* xor self */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    MEM32(0x1AF6A4) = ecx;
    MEM32(0x1AF6A8) = ecx;
    MEM32(0x1AF6AC) = ecx;
    MEM32(0x1AF6B0) = ecx;
    MEM32(0x1AF6B4) = ecx;
    MEM32(0x1AF6B8) = ecx;
    MEM32(0x1AF6BC) = ecx;
    MEM32(0x1AF6C0) = ecx;
    MEM32(0x1AF6C4) = ecx;
    goto loc_0002B5FC;

loc_0002B5F3:;
    PUSH32(esp, esi);
    PUSH32(esp, 0x0002B5F9u);
    RECOMP_ABI_CALL(0x0008B4C0u, sub_0008B4C0); /* call 0x0008B4C0 */

loc_0002B5F9:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002B5FC:;
    esi = MEM32(esi + 0xC);
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B580; /* jne: not equal / not zero */
    }

loc_0002B607:;
    goto loc_0002B60F;

loc_0002B609:;
    MEM32(0x1AF6A4) = esi;

loc_0002B60F:;
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002B617u);
    RECOMP_ABI_CALL(0x0008B600u, sub_0008B600); /* call 0x0008B600 */

loc_0002B617:;
    eax = MEM32(ebp + 0x40);
    xmm3 = XMM_SCALAR(MEMF(eax + 0x30));      /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x34));      /* movss */
    xmm4 = XMM_SCALAR(MEMF(eax + 0x38));      /* movss */
    xmm3.f[0] = xmm3.f[0] - MEMF(esp + 0x38); /* subss */
    xmm0.f[0] = xmm0.f[0] - MEMF(esp + 0x3C); /* subss */
    xmm4.f[0] = xmm4.f[0] - MEMF(esp + 0x40); /* subss */
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(eax) + (uint64_t)(0x30)) >> 32) & 1);
    eax = eax + 0x30;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = MEM32(0x1BF2CC);
    carry_flag = 0; /* xor clears CF */
    esi = 0;        /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    MEMF(esp + 0x50) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x58) = xmm4.f[0]; /* movss */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B7ED; /* jne: not equal / not zero */
    }

loc_0002B662:;
    eax = MEM32(0x1BF034);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B674; /* je: equal / zero */
    }

loc_0002B66B:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(1) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 1 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002B7ED; /* jne: not equal / not zero */
    }

loc_0002B674:;
    xmm1 = XMM_SCALAR(MEMF(0x1BF084)); /* movss */
    xmm6 = XMM_ZERO();                 /* xorps self = zero */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm6.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002B7F0; /* jp: parity */
    }

loc_0002B68C:;
    eax = MEM32(ebp + 0x40);
    xmm2 = XMM_SCALAR(MEMF(esp + 0xAC));    /* movss */
    xmm1 = XMM_SCALAR(MEMF(eax + 0x34));    /* movss */
    xmm2.f[0] = xmm2.f[0] * MEMF(0x180B44); /* mulss */
    xmm1.f[0] = xmm1.f[0] - xmm2.f[0];      /* subss */
    MEMF(eax + 0x34) = xmm1.f[0];           /* movss */
    xmm1 = xmm4;                            /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x1BF374); /* mulss */
    xmm5 = xmm3;                            /* movaps */
    xmm5.f[0] = xmm5.f[0] * MEMF(0x1BF36C); /* mulss */
    xmm1.f[0] = xmm1.f[0] + xmm5.f[0];      /* addss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm6.f[0]; /* comiss */
    MEMF(esp + 0x28) = xmm1.f[0];    /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B7D4; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm6.f[0]) */
    }

loc_0002B6D7:;
    xmm5 = XMM_SCALAR(MEMF(0x1AF484)); /* movss */
    float_compare_left = xmm5.f[0];
    float_compare_right = xmm6.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002B70E; /* jp: parity */
    }

loc_0002B6E8:;
    xmm5 = XMM_SCALAR(MEMF(0x1AF48C)); /* movss */
    float_compare_left = xmm5.f[0];
    float_compare_right = xmm6.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002B70E; /* jp: parity */
    }

loc_0002B6F9:;
    xmm5 = XMM_SCALAR(MEMF(0x1AF730)); /* movss */
    float_compare_left = xmm5.f[0];
    float_compare_right = xmm6.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002B7D4; /* jnp: not parity */
    }

loc_0002B70E:;
    fp_push(MEMF(esp + 0x28)); /* fld float */
    MEM32(0x1BF378) = 0xFFFFFFFFu;
    fp_top() = RECOMP_FP_PC(sqrt(fp_top())); /* fsqrt */
    eax = MEM32(ebp + 0x40);
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    MEMF(esp + 0x10) = (float)fp_top();
    fp_pop();                                             /* fstp */
    fp_push(MEMF(eax + 0x34));                            /* fld float */
    xmm0.f[0] = xmm0.f[0] - MEMF(esp + 0x10);             /* subss */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(esp + 0x10)); /* fsub dword ptr [esp + 0x10] */
    xmm5 = xmm0;                                          /* movaps */
    MEMF(esp + 0x20) = xmm5.f[0];                         /* movss */
    MEMF(eax + 0x34) = (float)fp_top();
    fp_pop(); /* fstp */
    eax = MEM32(ebp + 0x40);
    fp_push(MEMF(esp + 0x58)); /* fld float */
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(eax) + (uint64_t)(0x30)) >> 32) & 1);
    eax = eax + 0x30;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);               /* add result */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x58));               /* fmul dword ptr [esp + 0x58] */
    fp_push(MEMF(esp + 0x50));                                          /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x50));               /* fmul dword ptr [esp + 0x50] */
    fp_push(MEMF(esp + 0x54));                                          /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x54));               /* fmul dword ptr [esp + 0x54] */
    fp_top() = RECOMP_FP_PC(fp_top() + fp_st1());                       /* fadd st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() + g_fp_stack[(g_fp_top + 2) & 7]); /* fadd st(2) */
    fp_push(MEMF(esp + 0x20));                                          /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x20));               /* fmul dword ptr [esp + 0x20] */
    fp_top() = RECOMP_FP_PC(fp_top() + g_fp_stack[(g_fp_top + 2) & 7]); /* fadd st(2) */
    fp_top() = RECOMP_FP_PC(fp_top() + g_fp_stack[(g_fp_top + 3) & 7]); /* fadd st(3) */
    fp_st1() = RECOMP_FP_PC(fp_st1() / fp_top());
    fp_pop();                                /* fdivp st(1) */
    fp_top() = RECOMP_FP_PC(sqrt(fp_top())); /* fsqrt */
    g_fp_stack[(g_fp_top + 2) & 7] = fp_top();
    fp_pop();                                           /* fstp st(2) */
    fp_pop();                                           /* fstp st(0) */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x180B48)); /* fsub dword ptr [0x180b48] */
    MEMF(esp + 0x10) = (float)fp_top();
    fp_pop();                            /* fstp */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    xmm0 = xmm1;                         /* movaps */
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0];   /* mulss */
    xmm3 = XMM_SCALAR(MEMF(0x180DFC));   /* movss */
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0];   /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(eax);   /* addss */
    xmm2 = xmm1;                         /* movaps */
    MEMF(eax) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 4));    /* movss */
    xmm2.f[0] = xmm2.f[0] * xmm5.f[0];   /* mulss */
    xmm2.f[0] = xmm2.f[0] * xmm3.f[0];   /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0];   /* addss */
    MEMF(eax + 4) = xmm0.f[0];           /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 8));    /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm4.f[0];   /* mulss */
    xmm1.f[0] = xmm1.f[0] * xmm3.f[0];   /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0];   /* addss */
    MEMF(eax + 8) = xmm0.f[0];           /* movss */
    goto loc_0002B7F0;

loc_0002B7D4:;
    float_compare_left = xmm6.f[0];
    float_compare_right = xmm1.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B7E5; /* jbe: below or equal (unsigned <=) (xmm6.f[0] vs xmm1.f[0]) */
    }

loc_0002B7D9:;
    MEM32(0x1BF378) = 1;
    goto loc_0002B7F0;

loc_0002B7E5:;
    MEM32(0x1BF378) = esi;
    goto loc_0002B7F0;

loc_0002B7ED:;
    xmm6 = XMM_ZERO(); /* xorps self = zero */

loc_0002B7F0:;
    flag_result = (uint32_t)(MEM32(0x1AF4F8)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1AF4F8), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002B8C2; /* je: equal / zero */
    }

loc_0002B7FC:;
    xmm1 = XMM_SCALAR(MEMF(0x1AF730)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF024)); /* movss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm6.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002B877; /* jnp: not parity */
    }

loc_0002B815:;
    fp_push(MEMF(0x1BF374)); /* fld float */
    eax = MEM32(ebp + 0x40);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x1BF374)); /* fmul dword ptr [0x1bf374] */
    xmm0 = XMM_SCALAR(MEMF(esp + 0xAC));                /* movss */
    fp_push(MEMF(0x1BF36C));                            /* fld float */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x185020);             /* mulss */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x1BF36C)); /* fmul dword ptr [0x1bf36c] */
    xmm1 = xmm0;                                        /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x1BF36C);             /* mulss */
    xmm1.f[0] = xmm1.f[0] + MEMF(eax + 0x30);           /* addss */
    fp_st1() = RECOMP_FP_PC(fp_st1() + fp_top());
    fp_pop();                     /* faddp st(1) */
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */
    MEMF(eax + 0x30) = xmm1.f[0]; /* movss */
    eax = MEM32(ebp + 0x40);
    fp_top() = RECOMP_FP_PC(sqrt(fp_top()));              /* fsqrt */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    fp_top() = RECOMP_FP_PC(MEMF(eax + 0x34) - fp_top()); /* fsubr dword ptr [eax + 0x34] */
    MEMF(eax + 0x34) = (float)fp_top();
    fp_pop();                               /* fstp */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x1BF374); /* mulss */
    goto loc_0002B8B5;

loc_0002B877:;
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm6.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002B8C2; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm6.f[0]) */
    }

loc_0002B87C:;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x18501C);   /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0xAC); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x185018);   /* mulss */
    eax = MEM32(ebp + 0x40);
    xmm1 = xmm0;                              /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x1AF508);   /* mulss */
    xmm1.f[0] = xmm1.f[0] + MEMF(eax + 0x30); /* addss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x1AF510);   /* mulss */
    MEMF(eax + 0x30) = xmm1.f[0];             /* movss */

loc_0002B8B5:;
    eax = MEM32(ebp + 0x40);
    xmm0.f[0] = xmm0.f[0] + MEMF(eax + 0x38); /* addss */
    MEMF(eax + 0x38) = xmm0.f[0];             /* movss */

loc_0002B8C2:;
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002B8CAu);
    RECOMP_ABI_CALL(0x000206F0u, sub_000206F0); /* call 0x000206F0 */

loc_0002B8CA:;
    eax = MEM32(esp + 0x24);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    if ((flag_result != 0))
    {
        goto loc_0002B4C7; /* jne: not equal / not zero */
    }

loc_0002B8DC:;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(0x1AF1B4));        /* movss */
    MEMF(esp + 0xAC) = xmm0.f[0];             /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x30));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x80); /* subss */
    MEMF(0x1AF0F0) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x34));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x84); /* subss */
    MEMF(0x1AF0F4) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x38));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x88); /* subss */
    flag_result = (uint32_t)(MEM32(0x1BF2CC)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF2CC), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    MEMF(0x1AF0F8) = xmm0.f[0]; /* movss */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BAF6; /* jne: not equal / not zero */
    }

loc_0002B93B:;
    edx = MEM32(0x1BF88C);
    MEM32(esp + 0x5C) = esi;
    eax = MEM32(edx + 0x40);
    eax = MEM32(eax);
    ecx = MEM32(eax);
    flag_operand = (uint32_t)(0x50) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ecx) + (uint64_t)(0x50)) >> 32) & 1);
    ecx = ecx + 0x50;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    edx = MEM32(ecx);
    MEM32(esp + 0x2C) = edx;
    eax = MEM32(ecx + 4);
    MEM32(esp + 0x30) = eax;
    edx = MEM32(ecx + 8);
    xmm0 = XMM_SCALAR(MEMF(esp + 0x30));    /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x180DC8); /* addss */
    MEM32(esp + 0x34) = edx;
    eax = MEM32(ecx + 0xC);
    MEM32(esp + 0x38) = eax;
    edx = MEM32(ecx + 0x10);
    MEM32(esp + 0x3C) = edx;
    eax = MEM32(ecx + 0x14);
    ecx = esp + 0x5C;
    PUSH32(esp, ecx);
    edx = esp + 0x30;
    MEM32(esp + 0x44) = eax;
    PUSH32(esp, edx);
    eax = ebp + 0x4C;
    PUSH32(esp, eax);
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0002B99Fu);
    RECOMP_ABI_CALL(0x00085370u, sub_00085370); /* call 0x00085370 */

loc_0002B99F:;
    SET_LO8(eax, MEM8(esp + 0x68));
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x11) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(eax), 0x11 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0002B9B5; /* jne: not equal / not zero */
    }

loc_0002B9AA:;
    PUSH32(esp, 0x11);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002B9B2u);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_0002B9B2:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002B9B5:;
    flag_result = (uint32_t)(MEM32(0x1BF2CC)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF2CC), esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BAF6; /* jne: not equal / not zero */
    }

loc_0002B9C1:;
    ecx = MEM32(ebp + 0x40);
    flag_operand = (uint32_t)(0x80) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ecx) + (uint64_t)(0x80)) >> 32) & 1);
    ecx = ecx + 0x80;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    edx = MEM32(ecx);
    MEM32(esp + 0x2C) = edx;
    eax = MEM32(ecx + 4);
    MEM32(esp + 0x30) = eax;
    ecx = MEM32(ecx + 8);
    xmm0 = XMM_SCALAR(MEMF(esp + 0x30));    /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x181B68); /* addss */
    edx = esp + 0x38;
    PUSH32(esp, 0x1AF0F0);
    PUSH32(esp, edx);
    MEM32(esp + 0x3C) = ecx;
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0002BA01u);
    RECOMP_ABI_CALL(0x0008DB30u, sub_0008DB30); /* call 0x0008DB30 */

loc_0002BA01:;
    MEMF(esp + 0x50) = (float)fp_top();
    fp_pop();          /* fstp */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    ecx = MEM32(0x1BF88C);
    eax = esp + 0x64;
    PUSH32(esp, eax);
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x58) = 0xC00;
    MEM32(esp + 0x68) = esi;
    edx = MEM32(ecx + 0x40);
    PUSH32(esp, edx);
    eax = esp + 0x3C;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002BA33u);
    RECOMP_ABI_CALL(0x000A1E30u, sub_000A1E30); /* call 0x000A1E30 */

loc_0002BA33:;
    SET_LO8(eax, MEM8(esp + 0x70));
    flag_operand = (uint32_t)(0x14) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x14)) >> 32) & 1);
    esp = esp + 0x14;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(eax), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0002BADA; /* jne: not equal / not zero */
    }

loc_0002BA42:;
    eax = MEM32(ebp + 0x44);
    edx = ZX8(MEM8(eax + 0xB));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    edi = eax + 0x1A4;
    ebx = edx + eax + 0xC;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, ebx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002BAF6; /* jae: above or equal (unsigned >=) */
    }

loc_0002BA5E:;
    edi = edi;

loc_0002BA60:;
    esi = MEM32(edi + 8);
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BAD1; /* je: equal / zero */
    }

loc_0002BA67:;
    PUSH32(esp, 0x18500C);
    PUSH32(esp, 0x0002BA71u);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002BA71:;
    ecx = MEM32(esi);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BAB3; /* je: equal / zero */
    }

loc_0002BA7A:;
    PUSH32(esp, 0x185000);
    PUSH32(esp, 0x0002BA84u);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002BA84:;
    ecx = MEM32(esi);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BAB3; /* je: equal / zero */
    }

loc_0002BA8D:;
    PUSH32(esp, 0x184FF4);
    PUSH32(esp, 0x0002BA97u);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002BA97:;
    ecx = MEM32(esi);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BAB3; /* je: equal / zero */
    }

loc_0002BAA0:;
    PUSH32(esp, 0x184FE8);
    PUSH32(esp, 0x0002BAAAu);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002BAAA:;
    ecx = MEM32(esi);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BAD1; /* jne: not equal / not zero */
    }

loc_0002BAB3:;
    ecx = MEM32(esi + 0x2C);
    eax = esp + 0x5C;
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    edx = esp + 0x34;
    PUSH32(esp, edx);
    PUSH32(esp, 0x0002BAC6u);
    RECOMP_ABI_CALL(0x000A2200u, sub_000A2200); /* call 0x000A2200 */

loc_0002BAC6:;
    SET_LO8(eax, MEM8(esp + 0x68));
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(eax), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0002BADA; /* jne: not equal / not zero */
    }

loc_0002BAD1:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(edi) + (uint64_t)(0x44)) >> 32) & 1);
    edi = edi + 0x44;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, ebx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002BA60; /* jb: below (unsigned <) */
    }

loc_0002BAD8:;
    goto loc_0002BAF6;

loc_0002BADA:;
    eax = MEM32(ebp + 0x40);
    ecx = eax + 0x80;
    edx = MEM32(ecx);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(eax) + (uint64_t)(0x30)) >> 32) & 1);
    eax = eax + 0x30;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEM32(eax) = edx;
    edx = MEM32(ecx + 4);
    MEM32(eax + 4) = edx;
    ecx = MEM32(ecx + 8);
    MEM32(eax + 8) = ecx;

loc_0002BAF6:;
    eax = MEM32(ebp + 0x44);
    esi = eax + 0x1A4;
    ebx = eax + 0x94;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    MEM32(esp + 0x28) = esi;
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002BBAA; /* jae: above or equal (unsigned >=) */
    }

loc_0002BB11:;
    flag_result = (uint32_t)(MEM8(ebx)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(ebx), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002BB9F; /* je: equal / zero */
    }

loc_0002BB1A:;
    xmm0 = XMM_SCALAR(MEMF(ebx + 0x3C));    /* movss */
    xmm1 = XMM_SCALAR(MEMF(ebx + 0x34));    /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x1AF48C); /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x1AF484); /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0];      /* addss */
    PUSH32(esp, ebx);
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0002BB44u);
    RECOMP_ABI_CALL(0x0007A420u, sub_0007A420); /* call 0x0007A420 */

loc_0002BB44:;
    xmm1 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    xmm0 = XMM_ZERO();                   /* xorps self = zero */
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002BB9F; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */
    }

loc_0002BB55:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(ebx + 0x38); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002BB9F; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(ebx + 0x38)) */
    }

loc_0002BB5B:;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002BB61u);
    RECOMP_ABI_CALL(0x0007A470u, sub_0007A470); /* call 0x0007A470 */

loc_0002BB61:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BB9F; /* je: equal / zero */
    }

loc_0002BB68:;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(esp + 0xAC));      /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x180DB0);   /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(eax + 0x34); /* addss */
    MEMF(eax + 0x34) = xmm0.f[0];             /* movss */
    edi = MEM32(ebp + 0x44);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(edi) + (uint64_t)(0xC)) >> 32) & 1);
    edi = edi + 0xC;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    ecx = 0x11;
    esi = ebx;
    {
        uint64_t _n = (uint64_t)ecx * 4u;
        if (!g_df && esi < 0xFD000000u && edi < 0xFD000000u && (uint64_t)esi + _n <= 0xFD000000u &&
            (uint64_t)edi + _n <= 0xFD000000u)
        {
            uint8_t *_d = (uint8_t *)XBOX_PTR(edi), *_s = (uint8_t *)XBOX_PTR(esi);
            if ((uint64_t)edi + _n <= esi || (uint64_t)esi + _n <= edi)
            {
                memcpy(_d, _s, (size_t)_n);
            }
            else
            {
                uint32_t _i;
                for (_i = 0; _i < ecx; _i++)
                {
                    MEM32(edi + _i * 4u) = MEM32(esi + _i * 4u);
                }
            }
            esi += ecx * 4u;
            edi += ecx * 4u;
        }
        else
        {
            uint32_t _i;
            int32_t direction_step = RECOMP_DF_STEP(4);
            for (_i = 0; _i < ecx; _i++)
            {
                MEM32(edi + _i * direction_step) = MEM32(esi + _i * direction_step);
            }
            esi += ecx * direction_step;
            edi += ecx * direction_step;
        }
        ecx = 0;
    } /* rep movsd */
    esi = MEM32(esp + 0x28);
    MEM32(ebx) = 0;

loc_0002BB9F:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ebx) + (uint64_t)(0x44)) >> 32) & 1);
    ebx = ebx + 0x44;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002BB11; /* jb: below (unsigned <) */
    }

loc_0002BBAA:;
    PUSH32(esp, 0x0002BBAFu);
    RECOMP_ABI_CALL(0x000145A0u, sub_000145A0); /* call 0x000145A0 */

loc_0002BBAF:;
    edx = MEM32(esp + 0xAC);
    eax = MEM32(esp + 0xA8);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002BBC5u);
    RECOMP_ABI_CALL(0x00028ED0u, sub_00028ED0); /* call 0x00028ED0 */

loc_0002BBC5:;
    MEM32(0x1BF05C) = 0;
    eax = MEM32(ebp + 0x44);
    SET_LO8(ecx, MEM8(eax + 7));
    SET_LO8(edx, MEM8(eax + 6));
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(LO8(edx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), LO8(edx) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* jbe: below or equal (unsigned <=) */
    }

loc_0002BBE3:;
    edx = ZX8(MEM8(eax + 6));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    eax = edx + eax + 0xC;
    eax = MEM32(eax + 8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* je: equal / zero */
    }

loc_0002BBF9:;
    flag_result = (uint32_t)(MEM8(eax + 4)) & 0xFFu;
    flag_operand = (uint32_t)(0x18) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(eax + 4), 0x18 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* jne: not equal / not zero */
    }

loc_0002BC03:;
    eax = MEM32(0x1BF2E0);
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(8) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 8 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002BC42; /* je: equal / zero */
    }

loc_0002BC0D:;
    ecx = MEM32(ebp + 0x44);
    eax = ZX8(MEM8(ecx + 6));
    eax = (uint32_t)((int32_t)eax * (int32_t)0x44);
    edx = MEM32(eax + ecx + 0x14);
    flag_result = (uint32_t)(MEM8(edx + 4)) & 0xFFu;
    flag_operand = (uint32_t)(0x18) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(edx + 4), 0x18 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    eax = eax + ecx + 0xC;
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BC42; /* jne: not equal / not zero */
    }

loc_0002BC25:;
    edx = ZX8(MEM8(ecx + 7));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    PUSH32(esp, 0x3DCCCCCD);
    edi = edx + ecx + 0xC;
    ebx = 0x250E0;
    PUSH32(esp, 0x0002BC3Fu);
    RECOMP_ABI_CALL(0x0001A880u, sub_0001A880); /* call 0x0001A880 */

loc_0002BC3F:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002BC42:;
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002BC48u);
    RECOMP_ABI_CALL(0x00025190u, sub_00025190); /* call 0x00025190 */

loc_0002BC48:;
    esi = MEM32(ebp + 0x44);
    ecx = MEM32(esi + 0x14);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esi) + (uint64_t)(0xC)) >> 32) & 1);
    esi = esi + 0xC;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* je: equal / zero */
    }

loc_0002BC58:;
    flag_result = (uint32_t)(MEM8(ecx + 4)) & 0xFFu;
    flag_operand = (uint32_t)(0x18) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(ecx + 4), 0x18 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* jne: not equal / not zero */
    }

loc_0002BC5E:;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x10)); /* movss */
    xmm0 = XMM_ZERO();                   /* xorps self = zero */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002BCA8; /* jp: parity */
    }

loc_0002BC6F:;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x18)); /* movss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002BCA8; /* jp: parity */
    }

loc_0002BC7D:;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002BC83u);
    RECOMP_ABI_CALL(0x00015430u, sub_00015430); /* call 0x00015430 */

loc_0002BC83:;
    edi = eax;
    eax = MEM32(esi + 8);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002BC8Eu);
    RECOMP_ABI_CALL(0x00015480u, sub_00015480); /* call 0x00015480 */

loc_0002BC8E:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(1) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, 1 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* jne: not equal / not zero */
    }

loc_0002BC96:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BCA8; /* jne: not equal / not zero */
    }

loc_0002BC9A:;
    ecx = MEM32(esi + 8);
    PUSH32(esp, 0x37);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002BCA5u);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_0002BCA5:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002BCA8:;
    eax = MEM32(ebp + 0x44);
    SET_LO8(edx, MEM8(eax + 7));
    flag_result = (uint32_t)(LO8(edx)) & 0xFFu;
    flag_operand = (uint32_t)(MEM8(eax + 6)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(edx), MEM8(eax + 6) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002BCFC; /* jbe: below or equal (unsigned <=) */
    }

loc_0002BCB3:;
    ecx = ZX8(MEM8(eax + 6));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x44);
    eax = ecx + eax + 0xC;
    eax = MEM32(eax + 8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BCFC; /* je: equal / zero */
    }

loc_0002BCC5:;
    flag_result = (uint32_t)(MEM8(eax + 4)) & 0xFFu;
    flag_operand = (uint32_t)(0x1B) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(eax + 4), 0x1B (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BCFC; /* jne: not equal / not zero */
    }

loc_0002BCCB:;
    eax = MEM32(ebp + 0x44);
    edx = ZX8(MEM8(eax + 7));
    ecx = ZX8(MEM8(eax + 6));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x44);
    edi = edx + eax + 0xC;
    PUSH32(esp, 0x3DCCCCCD);
    eax = ecx + eax + 0xC;
    ebx = 0x1BDE0;
    PUSH32(esp, 0x0002BCF3u);
    RECOMP_ABI_CALL(0x0001A880u, sub_0001A880); /* call 0x0001A880 */

loc_0002BCF3:;
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002BCF9u);
    RECOMP_ABI_CALL(0x00025280u, sub_00025280); /* call 0x00025280 */

loc_0002BCF9:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002BCFC:;
    eax = MEM32(0x1BF184);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BDA1; /* je: equal / zero */
    }

loc_0002BD09:;
    eax = MEM32(ebp + 0x44);
    SET_LO8(edx, MEM8(eax + 5));
    flag_result = (uint32_t)(LO8(edx)) & 0xFFu;
    flag_operand = (uint32_t)(MEM8(eax + 4)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(edx), MEM8(eax + 4) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002BDA1; /* jbe: below or equal (unsigned <=) */
    }

loc_0002BD18:;
    eax = MEM32(ebp + 0x2C);
    eax = MEM32(eax + 0xC);
    ecx = MEM32(eax + 8);
    edx = MEM32(ecx + 4);
    eax = MEM32(edx + 0x10);
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(4) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 4 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002BDA1; /* je: equal / zero */
    }

loc_0002BD2C:;
    eax = MEM32(ebp + 0x44);
    ecx = ZX8(MEM8(eax + 4));
    edx = ZX8(MEM8(eax + 5));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x44);
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    edi = ecx + eax + 0xC;
    ebx = edx + eax + 0xC;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, ebx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002BDA1; /* jae: above or equal (unsigned >=) */
    }

loc_0002BD49:;
    /* nop */

loc_0002BD50:;
    esi = MEM32(edi + 8);
    SET_LO8(eax, MEM8(esi + 0x15));
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x1C) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x1C (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BD5E; /* je: equal / zero */
    }

loc_0002BD5A:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x1D) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x1D (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BD92; /* jne: not equal / not zero */
    }

loc_0002BD5E:;
    eax = MEM32(esi + 0x2C);
    ecx = MEM32(eax + 0x20);
    eax = MEM32(ebp + 0x2C);
    fp_push(MEMF(ecx + 0x34)); /* fld float */
    edx = MEM32(eax + 0x20);
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(edx + 0x34)); /* fsub dword ptr [edx + 0x34] */
    fp_top() = fabs(fp_top());                            /* fabs */
    fp_push(MEMF(0x180B44));                              /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1());
    fp_pop(); /* fcompi */
    fp_pop(); /* fstp st(0) */
    if ((g_fp_cmp != 1) /* fcompi */)
    {
        goto loc_0002BD92; /* jbe: below or equal (unsigned <=) */
    }

loc_0002BD7E:;
    PUSH32(esp, 0x3F5DB37D);
    PUSH32(esp, esi);
    ecx = ebp;
    PUSH32(esp, 0x0002BD8Bu);
    RECOMP_ABI_CALL(0x0001A3F0u, sub_0001A3F0); /* call 0x0001A3F0 */

loc_0002BD8B:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BD9B; /* jne: not equal / not zero */
    }

loc_0002BD92:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(edi) + (uint64_t)(0x44)) >> 32) & 1);
    edi = edi + 0x44;
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(edi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edi, ebx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002BD50; /* jb: below (unsigned <) */
    }

loc_0002BD99:;
    goto loc_0002BDA1;

loc_0002BD9B:;
    MEM32(0x1BF100) = esi;

loc_0002BDA1:;
    eax = MEM32(ebp + 0x44);
    SET_LO8(ecx, MEM8(eax + 5));
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(MEM8(eax + 4)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), MEM8(eax + 4) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002BEA3; /* jbe: below or equal (unsigned <=) */
    }

loc_0002BDB0:;
    eax = MEM32(0x1BF104);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BEA3; /* je: equal / zero */
    }

loc_0002BDBD:;
    ecx = MEM32(0x1BF184);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BEA3; /* je: equal / zero */
    }

loc_0002BDCB:;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002BDD1u);
    RECOMP_ABI_CALL(0x00030520u, sub_00030520); /* call 0x00030520 */

loc_0002BDD1:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BEA3; /* jne: not equal / not zero */
    }

loc_0002BDDC:;
    esi = MEM32(0x1BF104);
    flag_result = (uint32_t)(MEM8(esi + 0x15)) & 0xFFu;
    flag_operand = (uint32_t)(0x1C) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(esi + 0x15), 0x1C (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BEA3; /* jne: not equal / not zero */
    }

loc_0002BDEC:;
    ecx = MEM32(ebp + 0x44);
    edx = ZX8(MEM8(ecx + 4));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    eax = edx + ecx + 0xC;
    edx = ZX8(MEM8(ecx + 5));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    edx = edx + ecx + 0xC;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, edx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002BEA3; /* jae: above or equal (unsigned >=) */
    }

loc_0002BE0D:;
    xmm0 = XMM_SCALAR(MEMF(0x184FE4)); /* movss */

loc_0002BE15:;
    ecx = MEM32(eax + 8);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, esi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BE22; /* jne: not equal / not zero */
    }

loc_0002BE1C:;
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(eax + 0x38); /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0002BE2B; /* ja: above (unsigned >) (xmm0.f[0] vs MEMF(eax + 0x38)) */
    }

loc_0002BE22:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(eax) + (uint64_t)(0x44)) >> 32) & 1);
    eax = eax + 0x44;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, edx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002BE15; /* jb: below (unsigned <) */
    }

loc_0002BE29:;
    goto loc_0002BEA3;

loc_0002BE2B:;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(eax + 0x30));    /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x180F08); /* addss */
    MEMF(eax + 0x30) = xmm0.f[0];           /* movss */
    MEM32(0x1BF15C) = 1;
    edx = MEM32(ecx + 0x40);
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(eax + 0x84));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(edx + 0x34); /* subss */
    MEMF(0x1BF190) = xmm0.f[0];               /* movss */
    eax = MEM32(ecx + 0x40);
    edx = MEM32(ebp + 0x40);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(eax) + (uint64_t)(0x30)) >> 32) & 1);
    eax = eax + 0x30;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esi = MEM32(eax);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(edx) + (uint64_t)(0x30)) >> 32) & 1);
    edx = edx + 0x30;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEM32(edx) = esi;
    esi = MEM32(eax + 4);
    MEM32(edx + 4) = esi;
    eax = MEM32(eax + 8);
    MEM32(edx + 8) = eax;
    ecx = MEM32(ecx + 0x2C);
    edx = MEM32(ecx + 0x20);
    eax = MEM32(ebp + 0x2C);
    ecx = MEM32(eax + 0x20);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(edx) + (uint64_t)(0x30)) >> 32) & 1);
    edx = edx + 0x30;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    eax = MEM32(edx);
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ecx) + (uint64_t)(0x30)) >> 32) & 1);
    ecx = ecx + 0x30;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    MEM32(ecx) = eax;
    eax = MEM32(edx + 4);
    MEM32(ecx + 4) = eax;
    edx = MEM32(edx + 8);
    MEM32(ecx + 8) = edx;

loc_0002BEA3:;
    eax = MEM32(ebp + 0x44);
    SET_LO8(ecx, MEM8(eax + 5));
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(MEM8(eax + 4)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), MEM8(eax + 4) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002C2A6; /* jbe: below or equal (unsigned <=) */
    }

loc_0002BEB2:;
    edx = ZX8(MEM8(eax + 4));
    ecx = ZX8(MEM8(eax + 5));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x44);
    esi = edx + eax + 0xC;
    edx = MEM32(ebp + 0x2C);
    edi = ecx + eax + 0xC;
    eax = MEM32(edx + 0xC);
    ecx = MEM32(eax + 8);
    edx = MEM32(ecx + 4);
    eax = MEM32(edx + 0x10);
    carry_flag = 0; /* logical op clears CF */
    eax = eax & 0x1E;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xC (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002BEF8; /* jne: not equal / not zero */
    }

loc_0002BEDF:;
    PUSH32(esp, 0x3F350481);
    ebx = 0x24BF0;
    eax = esi;
    PUSH32(esp, 0x0002BEF0u);
    RECOMP_ABI_CALL(0x0001A880u, sub_0001A880); /* call 0x0001A880 */

loc_0002BEF0:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    goto loc_0002C2A6;

loc_0002BEF8:;
    ecx = MEM32(0x1BF010);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C15F; /* je: equal / zero */
    }

loc_0002BF06:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xE) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xE (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF47; /* je: equal / zero */
    }

loc_0002BF0B:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0xA) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0xA (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C15F; /* jne: not equal / not zero */
    }

loc_0002BF14:;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(eax + 0x84)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(eax + 0x34); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C15F; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(eax + 0x34)) */
    }

loc_0002BF29:;
    eax = MEM32(esi + 8);
    SET_LO8(eax, MEM8(eax + 0x15));
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x16) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x16 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF47; /* je: equal / zero */
    }

loc_0002BF33:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x18) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x18 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF47; /* je: equal / zero */
    }

loc_0002BF37:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF47; /* je: equal / zero */
    }

loc_0002BF3B:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x15) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x15 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF47; /* je: equal / zero */
    }

loc_0002BF3F:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x24) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x24 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C15F; /* jne: not equal / not zero */
    }

loc_0002BF47:;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, edi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002C2A6; /* jae: above or equal (unsigned >=) */
    }

loc_0002BF4F:;
    ebx = 2;

loc_0002BF54:;
    xmm0 = XMM_SCALAR(MEMF(0x181B60)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(esi + 0x38); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C14F; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(esi + 0x38)) */
    }

loc_0002BF66:;
    eax = MEM32(esi + 8);
    flag_result = (uint32_t)(MEM8(eax + 0x15)) & 0xFFu;
    flag_operand = (uint32_t)(0x24) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(eax + 0x15), 0x24 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C102; /* jne: not equal / not zero */
    }

loc_0002BF73:;
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002BF7Au);
    RECOMP_ABI_CALL(0x000375C0u, sub_000375C0); /* call 0x000375C0 */

loc_0002BF7A:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C14F; /* je: equal / zero */
    }

loc_0002BF85:;
    eax = ebx;
    edx = 1;

loc_0002BF8C:;
    ecx = MEM32(ebp + 0x2C);
    ecx = MEM32(ecx + 0x20);
    ecx = MEM32(ecx + 0x30);
    edi = MEM32(ebp + 0x40);
    MEM32(edi + 0x30) = ecx;
    ecx = MEM32(ebp + 0x2C);
    ecx = MEM32(ecx + 0x20);
    ecx = MEM32(ecx + 0x34);
    edi = MEM32(ebp + 0x40);
    MEM32(edi + 0x34) = ecx;
    ecx = MEM32(ebp + 0x2C);
    ecx = MEM32(ecx + 0x20);
    ecx = MEM32(ecx + 0x38);
    edi = MEM32(ebp + 0x40);
    MEM32(edi + 0x38) = ecx;
    ecx = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(ecx + 0x34)); /* movss */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, edx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    MEMF(0x1AF6CC) = xmm0.f[0]; /* movss */
    eax = 0x1BF4D0;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BFD7; /* je: equal / zero */
    }

loc_0002BFD2:;
    eax = 0x1BF4E0;

loc_0002BFD7:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF4A0)); /* movss */
    MEMF(0x1BF2B0) = xmm0.f[0];        /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 8));  /* movss */
    MEMF(0x1BF2B4) = xmm0.f[0];        /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 4));  /* movss */
    MEMF(0x1BF2B8) = xmm0.f[0];        /* movss */
    ecx = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(ecx + 0xE4)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C01B; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x180C70)) */
    }

loc_0002C015:;
    edi = MEM32(0x1BF034);

loc_0002C01B:;
    eax = MEM32(eax + 0xC);
    MEM32(ecx + 0xE4) = eax;
    MEM32(0x1BF034) = ebx;
    ecx = MEM32(ebp + 0x2C);
    eax = MEM32(ecx + 0xC);
    ecx = MEM32(eax + 8);
    eax = MEM32(ecx + 4);
    eax = MEM32(eax + 0x10);
    ecx = eax;
    carry_flag = 0; /* logical op clears CF */
    ecx = ecx & 0x1E;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(0xE) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), 0xE (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C048; /* je: equal / zero */
    }

loc_0002C043:;
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002C04E; /* je: equal / zero */
    }

loc_0002C048:;
    MEM32(0x1BF080) = edx;

loc_0002C04E:;
    eax = MEM32(esi + 8);
    SET_LO8(eax, MEM8(eax + 0x15));
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x16) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x16 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C064; /* je: equal / zero */
    }

loc_0002C058:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x18) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x18 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C064; /* je: equal / zero */
    }

loc_0002C05C:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x15) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0x15 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C064; /* je: equal / zero */
    }

loc_0002C060:;
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C06A; /* jne: not equal / not zero */
    }

loc_0002C064:;
    MEM32(0x1BF080) = edx;

loc_0002C06A:;
    ecx = MEM32(esi + 8);
    flag_result = (uint32_t)(MEM8(ecx + 0x15)) & 0xFFu;
    flag_operand = (uint32_t)(0x24) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(ecx + 0x15), 0x24 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C079; /* jne: not equal / not zero */
    }

loc_0002C073:;
    MEM32(0x1BF080) = ebx;

loc_0002C079:;
    MEM32(0x1BF2BC) = edx;
    MEM32(0x1BF2C0) = edx;
    edx = (uint32_t)(int32_t)SMEM8(0x1C2028);
    PUSH32(esp, ebx);
    PUSH32(esp, edx);
    PUSH32(esp, 0x0002C093u);
    RECOMP_ABI_CALL(0x00073BA0u, sub_00073BA0); /* call 0x00073BA0 */

loc_0002C093:;
    ecx = (uint32_t)(int32_t)SMEM16(0x1AF1F4);
    PUSH32(esp, 0x3FC00000);
    PUSH32(esp, 0x1E);
    eax = esp + 0x24;
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002C0ACu);
    RECOMP_ABI_CALL(0x00072550u, sub_00072550); /* call 0x00072550 */

loc_0002C0AC:;
    edx = MEM32(esp + 0x2C);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, ebx);
    MEM16(0x1AF1F4) = LO16(eax);
    eax = (uint32_t)(int32_t)SMEM16(0x1AF1F8);
    ecx = MEM32(eax * 4 + 0x1AF1FC);
    PUSH32(esp, 0x80);
    PUSH32(esp, edx);
    PUSH32(esp, 0x3F800000);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002C0DAu);
    RECOMP_ABI_CALL(0x000951D0u, sub_000951D0); /* call 0x000951D0 */

loc_0002C0DA:;
    flag_operand = (uint32_t)(0x34) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x34)) >> 32) & 1);
    esp = esp + 0x34;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    /* nop */

loc_0002C0E0:;
    PUSH32(esp, 0x0002C0E5u);
    RECOMP_ABI_CALL(0x000F18C3u, sub_000F18C3); /* call 0x000F18C3 */

loc_0002C0E5:;
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    ecx = 3;
    {
        int64_t _dividend = ((int64_t)(int32_t)edx << 32) | eax;
        eax = (uint32_t)((int32_t)(_dividend / (int32_t)ecx));
        edx = (uint32_t)((int32_t)(_dividend % (int32_t)ecx));
    }
    flag_result = (uint32_t)(LO16(edx)) & 0xFFFFu;
    flag_operand = (uint32_t)(MEM16(0x1AF1F8)) & 0xFFFFu;
    signed_flag_result = (int32_t)(int16_t)(flag_result);
    signed_flag_operand = (int32_t)(int16_t)(flag_operand); /* cmp LO16(edx), MEM16(0x1AF1F8) (16-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C0E0; /* je: equal / zero */
    }

loc_0002C0F6:;
    MEM16(0x1AF1F8) = LO16(edx);
    goto loc_0002C2A6;

loc_0002C102:;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C108u);
    RECOMP_ABI_CALL(0x0004CB30u, sub_0004CB30); /* call 0x0004CB30 */

loc_0002C108:;
    edx = 1;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, edx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF8C; /* je: equal / zero */
    }

loc_0002C118:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, ebx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002BF8C; /* je: equal / zero */
    }

loc_0002C120:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 4 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C14F; /* jne: not equal / not zero */
    }

loc_0002C125:;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x34)); /* movss */
    xmm0 = XMM_ZERO();                   /* xorps self = zero */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002C14F; /* jp: parity */
    }

loc_0002C136:;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x3C)); /* movss */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm0.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002C14F; /* jp: parity */
    }

loc_0002C144:;
    ecx = MEM32(ebp + 0x40);
    MEMF(ecx + 0xE4) = xmm0.f[0]; /* movss */

loc_0002C14F:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esi) + (uint64_t)(0x44)) >> 32) & 1);
    esi = esi + 0x44;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, edi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002BF54; /* jb: below (unsigned <) */
    }

loc_0002C15A:;
    goto loc_0002C2A6;

loc_0002C15F:;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, edi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002C2A6; /* jae: above or equal (unsigned >=) */
    }

loc_0002C167:;
    ebx = 2;
    /* nop */

loc_0002C170:;
    edx = MEM32(esi + 8);
    PUSH32(esp, esi);
    PUSH32(esp, edx);
    PUSH32(esp, 0x0002C17Au);
    RECOMP_ABI_CALL(0x0004CED0u, sub_0004CED0); /* call 0x0004CED0 */

loc_0002C17A:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    carry_flag = (int)((uint32_t)(eax) < (uint32_t)(ebx));
    eax = eax - ebx;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    if ((flag_result == 0))
    {
        goto loc_0002C1B4; /* je: equal / zero */
    }

loc_0002C181:;
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((flag_result == 0))
    {
        goto loc_0002C282; /* je: equal / zero */
    }

loc_0002C188:;
    eax--;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((flag_result != 0))
    {
        goto loc_0002C275; /* jne: not equal / not zero */
    }

loc_0002C18F:;
    esi = MEM32(esi + 8);
    PUSH32(esp, 1);
    PUSH32(esp, esi);
    PUSH32(esp, 0x0002C19Au);
    RECOMP_ABI_CALL(0x0001D970u, sub_0001D970); /* call 0x0001D970 */

loc_0002C19A:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C2A6; /* je: equal / zero */
    }

loc_0002C1A5:;
    edx = (uint32_t)(int32_t)SMEM8(0x1C2028);
    PUSH32(esp, 4);
    PUSH32(esp, edx);
    goto loc_0002C29E;

loc_0002C1B4:;
    xmm0 = XMM_SCALAR(MEMF(0x181B60)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(esi + 0x38); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C275; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(esi + 0x38)) */
    }

loc_0002C1C6:;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(eax + 0x34)); /* movss */
    MEMF(0x1AF6CC) = xmm0.f[0];          /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF4A0));   /* movss */
    MEMF(0x1BF2B0) = xmm0.f[0];          /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF4E8));   /* movss */
    MEMF(0x1BF2B4) = xmm0.f[0];          /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF4E4));   /* movss */
    MEMF(0x1BF2B8) = xmm0.f[0];          /* movss */
    ecx = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(ecx + 0xE4)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C236; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x180C70)) */
    }

loc_0002C21A:;
    flag_result = (uint32_t)(MEM32(0x1BF034)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF034), ebx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C236; /* jne: not equal / not zero */
    }

loc_0002C222:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF4EC)); /* movss */
    edx = ecx;
    MEMF(edx + 0xE4) = xmm0.f[0]; /* movss */
    goto loc_0002C249;

loc_0002C236:;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(0x1BF4EC)); /* movss */
    MEMF(eax + 0xE4) = xmm0.f[0];      /* movss */

loc_0002C249:;
    ecx = (uint32_t)(int32_t)SMEM8(0x1C2028);
    eax = 1;
    PUSH32(esp, ebx);
    PUSH32(esp, ecx);
    MEM32(0x1BF034) = ebx;
    MEM32(0x1BF080) = ebx;
    MEM32(0x1BF2BC) = eax;
    MEM32(0x1BF2C0) = eax;
    PUSH32(esp, 0x0002C272u);
    RECOMP_ABI_CALL(0x00073BA0u, sub_00073BA0); /* call 0x00073BA0 */

loc_0002C272:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C275:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esi) + (uint64_t)(0x44)) >> 32) & 1);
    esi = esi + 0x44;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, edi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002C170; /* jb: below (unsigned <) */
    }

loc_0002C280:;
    goto loc_0002C2A6;

loc_0002C282:;
    esi = MEM32(esi + 8);
    PUSH32(esp, 1);
    PUSH32(esp, esi);
    PUSH32(esp, 0x0002C28Du);
    RECOMP_ABI_CALL(0x0001D970u, sub_0001D970); /* call 0x0001D970 */

loc_0002C28D:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C2A6; /* je: equal / zero */
    }

loc_0002C294:;
    eax = (uint32_t)(int32_t)SMEM8(0x1C2028);
    PUSH32(esp, 4);
    PUSH32(esp, eax);

loc_0002C29E:;
    PUSH32(esp, 0x0002C2A3u);
    RECOMP_ABI_CALL(0x00073BA0u, sub_00073BA0); /* call 0x00073BA0 */

loc_0002C2A3:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C2A6:;
    eax = MEM32(0x1BF010);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C44A; /* je: equal / zero */
    }

loc_0002C2B3:;
    eax = MEM32(ebp + 0x44);
    SET_LO8(ecx, MEM8(eax + 7));
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(MEM8(eax + 6)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), MEM8(eax + 6) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002C44A; /* jbe: below or equal (unsigned <=) */
    }

loc_0002C2C2:;
    edx = ZX8(MEM8(eax + 6));
    ecx = ZX8(MEM8(eax + 7));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x44);
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x44);
    esi = edx + eax + 0xC;
    edi = ecx + eax + 0xC;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, edi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002C44A; /* jae: above or equal (unsigned >=) */
    }

loc_0002C2E0:;
    SET_LO8(ebx, 0xF);

loc_0002C2E2:;
    eax = MEM32(esi + 8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C330; /* je: equal / zero */
    }

loc_0002C2E9:;
    flag_result = (uint32_t)(MEM8(eax + 4)) & 0xFFu;
    flag_operand = (uint32_t)(LO8(ebx)) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(eax + 4), LO8(ebx) (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C330; /* jne: not equal / not zero */
    }

loc_0002C2EE:;
    xmm0 = XMM_SCALAR(MEMF(0x181B94)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(esi + 0x38); /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0002C33C; /* ja: above (unsigned >) (xmm0.f[0] vs MEMF(esi + 0x38)) */
    }

loc_0002C2FC:;
    flag_result = (uint32_t)(MEM8(eax + 0x15)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(eax + 0x15), 1 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C330; /* jne: not equal / not zero */
    }

loc_0002C302:;
    eax = MEM32(eax + 0xF0);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C330; /* je: equal / zero */
    }

loc_0002C30C:;
    edx = MEM32(ebp + 0x2C);
    ecx = MEM32(edx + 0xC);
    edx = MEM32(ecx + 8);
    ecx = MEM32(edx + 4);
    edx = MEM32(ecx + 0x10);
    carry_flag = 0; /* logical op clears CF */
    edx = edx & 0x1E;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(LO8(edx)) & 0xFFu;
    flag_operand = (uint32_t)(0xC) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(edx), 0xC (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C330; /* jne: not equal / not zero */
    }

loc_0002C323:;
    ecx = esi + 0x34;
    PUSH32(esp, ecx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C32Du);
    RECOMP_ABI_CALL(0x0004CC20u, sub_0004CC20); /* call 0x0004CC20 */

loc_0002C32D:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C330:;
    flag_operand = (uint32_t)(0x44) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esi) + (uint64_t)(0x44)) >> 32) & 1);
    esi = esi + 0x44;
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(edi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, edi (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002C2E2; /* jb: below (unsigned <) */
    }

loc_0002C337:;
    goto loc_0002C44A;

loc_0002C33C:;
    SET_LO8(ecx, MEM8(eax + 0x15));
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C41B; /* jne: not equal / not zero */
    }

loc_0002C347:;
    edx = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(edx + 0x34)); /* movss */
    MEMF(0x1AF6CC) = xmm0.f[0];          /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF4A0));   /* movss */
    MEMF(0x1BF2B0) = xmm0.f[0];          /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF4E8));   /* movss */
    MEMF(0x1BF2B4) = xmm0.f[0];          /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF4E4));   /* movss */
    MEMF(0x1BF2B8) = xmm0.f[0];          /* movss */
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(eax + 0xE4)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C3B9; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x180C70)) */
    }

loc_0002C39B:;
    flag_result = (uint32_t)(MEM32(0x1BF034)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(2) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF034), 2 (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C3B9; /* jne: not equal / not zero */
    }

loc_0002C3A4:;
    ecx = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(0x1BF4EC)); /* movss */
    MEMF(ecx + 0xE4) = xmm0.f[0];      /* movss */
    goto loc_0002C3CC;

loc_0002C3B9:;
    edx = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(0x1BF4EC)); /* movss */
    MEMF(edx + 0xE4) = xmm0.f[0];      /* movss */

loc_0002C3CC:;
    MEM32(0x1BF034) = 2;
    eax = MEM32(ebp + 0x2C);
    ecx = MEM32(eax + 0xC);
    edx = MEM32(ecx + 8);
    eax = MEM32(edx + 4);
    eax = MEM32(eax + 0x10);
    ecx = eax;
    carry_flag = 0; /* logical op clears CF */
    ecx = ecx & 0x1E;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(0xE) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), 0xE (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C3F4; /* je: equal / zero */
    }

loc_0002C3EF:;
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002C40A; /* je: equal / zero */
    }

loc_0002C3F4:;
    eax = 1;
    MEM32(0x1BF080) = eax;
    MEM32(0x1BF2BC) = eax;
    MEM32(0x1BF2C0) = eax;
    goto loc_0002C44A;

loc_0002C40A:;
    eax = 1;
    MEM32(0x1BF2BC) = eax;
    MEM32(0x1BF2C0) = eax;
    goto loc_0002C44A;

loc_0002C41B:;
    flag_result = (uint32_t)(LO8(ecx)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(ecx), 1 (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C44A; /* jne: not equal / not zero */
    }

loc_0002C420:;
    eax = MEM32(eax + 0xF0);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C44A; /* je: equal / zero */
    }

loc_0002C42A:;
    edx = MEM32(ebp + 0x2C);
    ecx = MEM32(edx + 0xC);
    edx = MEM32(ecx + 8);
    ecx = MEM32(edx + 4);
    edx = MEM32(ecx + 0x10);
    carry_flag = 0; /* logical op clears CF */
    edx = edx & 0x1E;
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* and result */
    flag_result = (uint32_t)(LO8(edx)) & 0xFFu;
    flag_operand = (uint32_t)(0xE) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(edx), 0xE (8-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C44A; /* jne: not equal / not zero */
    }

loc_0002C441:;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C447u);
    RECOMP_ABI_CALL(0x0004CB30u, sub_0004CB30); /* call 0x0004CB30 */

loc_0002C447:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C44A:;
    edi = MEM32(esp + 0xAC);
    esi = MEM32(esp + 0xA8);
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C460u);
    RECOMP_ABI_CALL(0x0001E500u, sub_0001E500); /* call 0x0001E500 */

loc_0002C460:;
    xmm1 = XMM_SCALAR(MEMF(esp + 0xB8)); /* movss */
    PUSH32(esp, 0x0002C46Eu);
    RECOMP_ABI_CALL(0x00021600u, sub_00021600); /* call 0x00021600 */

loc_0002C46E:;
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C476u);
    RECOMP_ABI_CALL(0x00028FB0u, sub_00028FB0); /* call 0x00028FB0 */

loc_0002C476:;
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C47Eu);
    RECOMP_ABI_CALL(0x000290B0u, sub_000290B0); /* call 0x000290B0 */

loc_0002C47E:;
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C486u);
    RECOMP_ABI_CALL(0x0002A920u, sub_0002A920); /* call 0x0002A920 */

loc_0002C486:;
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C48Du);
    RECOMP_ABI_CALL(0x00029C20u, sub_00029C20); /* call 0x00029C20 */

loc_0002C48D:;
    xmm1 = XMM_SCALAR(MEMF(0x1BF41C)); /* movss */
    xmm2 = XMM_SCALAR(MEMF(0x1BF428)); /* movss */
    flag_operand = (uint32_t)(0x38) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x38)) >> 32) & 1);
    esp = esp + 0x38;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    float_compare_left = xmm1.f[0];
    float_compare_right = xmm2.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (RECOMP_PARITY8((flag_result) & (flag_operand)))
    {
        goto loc_0002C4BA; /* jp: parity */
    }

loc_0002C4A9:;
    float_compare_left = xmm1.f[0];
    float_compare_right = MEMF(0x180B48); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002C5E2; /* jnp: not parity */
    }

loc_0002C4BA:;
    xmm5 = XMM_SCALAR(MEMF(0x1BF420)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1BF424)); /* movss */
    xmm4 = XMM_SCALAR(MEMF(0x1BF418)); /* movss */
    xmm7 = XMM_SCALAR(MEMF(0x180DFC)); /* movss */
    xmm2.f[0] = xmm2.f[0] - xmm1.f[0]; /* subss */
    xmm1 = XMM_SCALAR(MEMF(0x1BF42C)); /* movss */
    xmm1.f[0] = xmm1.f[0] - xmm5.f[0]; /* subss */
    xmm3 = xmm1;                       /* movaps */
    xmm3.f[0] = xmm3.f[0] * xmm1.f[0]; /* mulss */
    xmm6 = xmm2;                       /* movaps */
    xmm6.f[0] = xmm6.f[0] * xmm2.f[0]; /* mulss */
    xmm3.f[0] = xmm3.f[0] + xmm6.f[0]; /* addss */
    xmm0.f[0] = xmm0.f[0] - xmm4.f[0]; /* subss */
    xmm6 = xmm0;                       /* movaps */
    xmm6.f[0] = xmm6.f[0] * xmm0.f[0]; /* mulss */
    xmm3.f[0] = xmm3.f[0] + xmm6.f[0]; /* addss */
    xmm6 = XMM_SCALAR(MEMF(0x1BF2AC)); /* movss */
    float_compare_left = xmm7.f[0];
    float_compare_right = xmm6.f[0]; /* comiss */
    MEMF(esp + 0x14) = xmm3.f[0];    /* movss */
    if ((float_compare_left < float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C528; /* jb: below (unsigned <) (xmm7.f[0] vs xmm6.f[0]) */
    }

loc_0002C51E:;
    xmm6 = XMM_SCALAR(MEMF(0x184FE0)); /* movss */
    goto loc_0002C54A;

loc_0002C528:;
    xmm7 = XMM_SCALAR(MEMF(0x180B44)); /* movss */
    float_compare_left = xmm6.f[0];
    float_compare_right = xmm7.f[0]; /* comiss */
    if ((float_compare_left < float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C53A; /* jb: below (unsigned <) (xmm6.f[0] vs xmm7.f[0]) */
    }

loc_0002C535:;
    xmm6 = xmm7; /* movaps */
    goto loc_0002C54A;

loc_0002C53A:;
    xmm6.f[0] = xmm6.f[0] - MEMF(0x180DFC); /* subss */
    xmm6.f[0] = xmm6.f[0] * MEMF(0x184FDC); /* mulss */

loc_0002C54A:;
    xmm7 = xmm6;                       /* movaps */
    xmm7.f[0] = xmm7.f[0] * xmm6.f[0]; /* mulss */
    float_compare_left = xmm7.f[0];
    float_compare_right = xmm3.f[0]; /* comiss */
    MEMF(esp + 0x10) = xmm6.f[0];    /* movss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C580; /* jbe: below or equal (unsigned <=) (xmm7.f[0] vs xmm3.f[0]) */
    }

loc_0002C55C:;
    eax = MEM32(0x1BF424);
    ecx = MEM32(0x1BF428);
    edx = MEM32(0x1BF42C);
    MEM32(0x1BF418) = eax;
    MEM32(0x1BF41C) = ecx;
    MEM32(0x1BF420) = edx;
    goto loc_0002C5E2;

loc_0002C580:;
    fp_push(MEMF(esp + 0x14)); /* fld float */
    PUSH32(esp, 0x1BF418);
    fp_top() = RECOMP_FP_PC(sqrt(fp_top())); /* fsqrt */
    PUSH32(esp, 0x1BF418);
    fp_top() = RECOMP_FP_PC(MEMF(0x180B48) / fp_top());   /* fdivr dword ptr [0x180b48] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x18)); /* fmul dword ptr [esp + 0x18] */
    MEMF(esp + 0x1C) = (float)fp_top();
    fp_pop();                               /* fstp */
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C));    /* movss */
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0];      /* mulss */
    xmm2.f[0] = xmm2.f[0] * xmm3.f[0];      /* mulss */
    xmm2.f[0] = xmm2.f[0] + MEMF(0x1BF41C); /* addss */
    xmm1.f[0] = xmm1.f[0] * xmm3.f[0];      /* mulss */
    xmm4.f[0] = xmm4.f[0] + xmm0.f[0];      /* addss */
    xmm5.f[0] = xmm5.f[0] + xmm1.f[0];      /* addss */
    MEMF(0x1BF418) = xmm4.f[0];             /* movss */
    MEMF(0x1BF41C) = xmm2.f[0];             /* movss */
    MEMF(0x1BF420) = xmm5.f[0];             /* movss */
    PUSH32(esp, 0x0002C5DDu);
    RECOMP_ABI_CALL(0x0008DB30u, sub_0008DB30); /* call 0x0008DB30 */

loc_0002C5DD:;
    fp_pop(); /* fstp st(0) */
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C5E2:;
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C5EAu);
    RECOMP_ABI_CALL(0x0001E1F0u, sub_0001E1F0); /* call 0x0001E1F0 */

loc_0002C5EA:;
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C5F2u);
    RECOMP_ABI_CALL(0x000861A0u, sub_000861A0); /* call 0x000861A0 */

loc_0002C5F2:;
    eax = MEM32(0x1BF1F4);
    flag_operand = (uint32_t)(0x18) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x18)) >> 32) & 1);
    esp = esp + 0x18;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C657; /* je: equal / zero */
    }

loc_0002C5FE:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(MEM32(0x1BF1F8)) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, MEM32(0x1BF1F8) (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C6C2; /* je: equal / zero */
    }

loc_0002C60A:;
    eax = MEM32(0x1BEF4C);
    ecx = MEM32(eax + 0x20);
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    flag_operand = (uint32_t)(0x30) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(ecx) + (uint64_t)(0x30)) >> 32) & 1);
    ecx = ecx + 0x30;
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    edx = MEM32(ecx);
    MEM32(0x1AF46C) = edx;
    eax = MEM32(ecx + 4);
    MEM32(0x1AF470) = eax;
    ecx = MEM32(ecx + 8);
    MEM32(0x1AF474) = ecx;
    edx = MEM32(ebp + 0x40);
    MEMF(edx + 0xE0) = xmm0.f[0]; /* movss */
    eax = MEM32(ebp + 0x40);
    MEMF(eax + 0xE4) = xmm0.f[0]; /* movss */
    ecx = MEM32(ebp + 0x40);
    MEMF(ecx + 0xE8) = xmm0.f[0]; /* movss */
    eax = MEM32(0x1BF1F4);

loc_0002C657:;
    flag_result = (uint32_t)(MEM32(0x1BF1F8)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x1BF1F8), eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C6C2; /* je: equal / zero */
    }

loc_0002C65F:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C68D; /* je: equal / zero */
    }

loc_0002C663:;
    PUSH32(esp, 0x20);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C66Bu);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_0002C66B:;
    eax = MEM32(0x1BF1F4);
    ecx = MEM32(eax + 0x134);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C68D; /* je: equal / zero */
    }

loc_0002C67D:;
    PUSH32(esp, 0x20);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002C685u);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_0002C685:;
    eax = MEM32(0x1BF1F4);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C68D:;
    ecx = MEM32(0x1BF1F8);
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C6C2; /* je: equal / zero */
    }

loc_0002C697:;
    PUSH32(esp, 0x21);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002C69Fu);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_0002C69F:;
    edx = MEM32(0x1BF1F8);
    eax = MEM32(edx + 0x134);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C6BD; /* je: equal / zero */
    }

loc_0002C6B2:;
    PUSH32(esp, 0x21);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C6BAu);
    RECOMP_ABI_CALL(0x000141B0u, sub_000141B0); /* call 0x000141B0 */

loc_0002C6BA:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C6BD:;
    eax = MEM32(0x1BF1F4);

loc_0002C6C2:;
    MEM32(0x1BF1F8) = eax;
    eax = MEM32(ebp + 0x40);
    xmm0 = XMM_SCALAR(MEMF(eax + 0x30));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x80); /* subss */
    MEMF(0x1AF44C) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x34));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x84); /* subss */
    MEMF(0x1AF450) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x38));      /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x88); /* subss */
    MEMF(0x1AF454) = xmm0.f[0];               /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x1AF730));        /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002C7C1; /* jnp: not parity */
    }

loc_0002C722:;
    eax = MEM32(0x1BF36C);
    ecx = MEM32(0x1BF370);
    edx = MEM32(0x1BF374);
    MEM32(0x1AF508) = eax;
    eax = MEM32(ebp + 0x44);
    eax = MEM32(eax + 0x14);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    MEM32(0x1AF50C) = ecx;
    MEM32(0x1AF510) = edx;
    MEM32(0x1AF4F8) = eax;
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C7C1; /* je: equal / zero */
    }

loc_0002C753:;
    PUSH32(esp, 0x18500C);
    PUSH32(esp, 0x0002C75Du);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002C75D:;
    ecx = MEM32(0x1AF4F8);
    edx = MEM32(ecx);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C7C1; /* je: equal / zero */
    }

loc_0002C76C:;
    PUSH32(esp, 0x185000);
    PUSH32(esp, 0x0002C776u);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002C776:;
    edx = MEM32(0x1AF4F8);
    ecx = MEM32(edx);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C7C1; /* je: equal / zero */
    }

loc_0002C785:;
    PUSH32(esp, 0x184FF4);
    PUSH32(esp, 0x0002C78Fu);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002C78F:;
    ecx = MEM32(0x1AF4F8);
    edx = MEM32(ecx);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C7C1; /* je: equal / zero */
    }

loc_0002C79E:;
    PUSH32(esp, 0x184FE8);
    PUSH32(esp, 0x0002C7A8u);
    RECOMP_ABI_CALL(0x00096F50u, sub_00096F50); /* call 0x00096F50 */

loc_0002C7A8:;
    edx = MEM32(0x1AF4F8);
    ecx = MEM32(edx);
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(ecx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ecx, eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C7C1; /* je: equal / zero */
    }

loc_0002C7B7:;
    MEM32(0x1AF4F8) = 0;

loc_0002C7C1:;
    xmm0 = XMM_SCALAR(MEMF(0x1BF0FC)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x180C70); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((float_compare_left == float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x40u
                                                 : 0u) |
                                            (((float_compare_left != float_compare_left ||
                                               float_compare_right != float_compare_right))
                                                 ? 0x04u
                                                 : 0u) |
                                            (((float_compare_left < float_compare_right ||
                                               (float_compare_left != float_compare_left ||
                                                float_compare_right != float_compare_right)))
                                                 ? 0x01u
                                                 : 0u) |
                                            0x02u)
                                 << 8); /* lahf (ucomiss) */
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0x44) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 0x44 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if ((!RECOMP_PARITY8((flag_result) & (flag_operand))))
    {
        goto loc_0002C7E4; /* jnp: not parity */
    }

loc_0002C7D6:;
    PUSH32(esp, edi);
    PUSH32(esp, 0x0002C7DCu);
    RECOMP_ABI_CALL(0x0002A3F0u, sub_0002A3F0); /* call 0x0002A3F0 */

loc_0002C7DC:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    goto loc_0002C9AE;

loc_0002C7E4:;
    carry_flag = 0; /* xor clears CF */
    ebx = 0;        /* xor self */
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    MEM32(0x1BF0F8) = ebx;
    flag_result = (uint32_t)(MEM16(esi + 6)) & 0xFFFFu;
    flag_operand = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    signed_flag_result = (int32_t)(int16_t)(flag_result);
    signed_flag_operand = (int32_t)(int16_t)(flag_operand); /* cmp MEM16(esi + 6), LO16(ebx) (16-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002C988; /* jbe: below or equal (unsigned <=) */
    }

loc_0002C7F6:;
    eax = MEM32(esi + 0x2C);
    eax = MEM32(eax + ebx * 4);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C96B; /* je: equal / zero */
    }

loc_0002C804:;
    flag_result = (uint32_t)(MEM8(eax + 6)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(eax + 6), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002C96B; /* je: equal / zero */
    }

loc_0002C80E:;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C814u);
    RECOMP_ABI_CALL(0x00087CB0u, sub_00087CB0); /* call 0x00087CB0 */

loc_0002C814:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C96B; /* je: equal / zero */
    }

loc_0002C81F:;
    ecx = MEM32(esi + 0x2C);
    edi = MEM32(ecx + ebx * 4);
    flag_result = (uint32_t)(MEM8(edi + 0xF0)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(edi + 0xF0), 1 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002C96B; /* je: equal / zero */
    }

loc_0002C832:;
    edx = MEM32(edi + 0x2C);
    eax = MEM32(edx + 0x20);
    xmm2 = XMM_SCALAR(MEMF(0x1BEF6C));        /* movss */
    xmm2.f[0] = xmm2.f[0] - MEMF(eax + 0x30); /* subss */
    xmm1 = XMM_SCALAR(MEMF(0x1BEF70));        /* movss */
    xmm1.f[0] = xmm1.f[0] - MEMF(eax + 0x34); /* subss */
    xmm0 = XMM_SCALAR(MEMF(0x1BEF74));        /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax + 0x38); /* subss */
    xmm3 = XMM_SCALAR(MEMF(0x1BEF78));        /* movss */
    xmm3.f[0] = xmm3.f[0] * MEMF(0x18501C);   /* mulss */
    xmm4 = xmm2;                              /* movaps */
    xmm4.f[0] = xmm4.f[0] * xmm2.f[0];        /* mulss */
    xmm2 = xmm1;                              /* movaps */
    xmm2.f[0] = xmm2.f[0] * xmm1.f[0];        /* mulss */
    xmm1 = xmm0;                              /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0];        /* mulss */
    xmm4.f[0] = xmm4.f[0] + xmm2.f[0];        /* addss */
    xmm0 = xmm3;                              /* movaps */
    xmm4.f[0] = xmm4.f[0] + xmm1.f[0];        /* addss */
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0];        /* mulss */
    float_compare_left = xmm0.f[0];
    float_compare_right = xmm4.f[0]; /* comiss */
    if ((float_compare_left <= float_compare_right ||
         (float_compare_left != float_compare_left || float_compare_right != float_compare_right)))
    {
        goto loc_0002C958; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm4.f[0]) */
    }

loc_0002C89C:;
    MEM32(0x1BF0F8) = edi;
    flag_result = (uint32_t)(MEM8(edi + 0xF0)) & 0xFFu;
    flag_operand = (uint32_t)(2) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test MEM8(edi + 0xF0), 2 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_NZ(flag_result, flag_operand))
    {
        goto loc_0002C96B; /* jne: not equal / not zero */
    }

loc_0002C8AF:;
    eax = MEM32(0x1BF0D8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C92C; /* je: equal / zero */
    }

loc_0002C8BC:;
    eax = MEM32(0x1AF768);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C8CF; /* je: equal / zero */
    }

loc_0002C8C5:;
    eax = MEM32(eax);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(MEM32(0x198540)) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, MEM32(0x198540) (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002C92C; /* jae: above or equal (unsigned >=) */
    }

loc_0002C8CF:;
    ecx = MEM32(0x1AF760);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0002C8DBu);
    RECOMP_ABI_CALL(0x000AC6F0u, sub_000AC6F0); /* call 0x000AC6F0 */

loc_0002C8DB:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp LO8(eax), 0 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C8F0; /* je: equal / zero */
    }

loc_0002C8E2:;
    edx = MEM32(0x198540);
    flag_result = (uint32_t)(edx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(MEM32(0x1AF764)) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp edx, MEM32(0x1AF764) (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_BE(flag_result, flag_operand))
    {
        goto loc_0002C92C; /* jbe: below or equal (unsigned <=) */
    }

loc_0002C8F0:;
    eax = MEM32(0x198544);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C901; /* je: equal / zero */
    }

loc_0002C8F9:;
    flag_result = (uint32_t)(MEM32(0x198554)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp MEM32(0x198554), eax (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_AE(flag_result, flag_operand))
    {
        goto loc_0002C92C; /* jae: above or equal (unsigned >=) */
    }

loc_0002C901:;
    xmm0 = XMM_SCALAR(MEMF(0x198548)); /* movss */
    float_compare_left = xmm0.f[0];
    float_compare_right = MEMF(0x198550); /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0002C92C; /* ja: above (unsigned >) (xmm0.f[0] vs MEMF(0x198550)) */
    }

loc_0002C912:;
    xmm0 = XMM_SCALAR(MEMF(0x19854C)); /* movss */
    MEMF(0x1AF76C) = xmm0.f[0];        /* movss */
    MEM32(0x1AF768) = 0x198540;

loc_0002C92C:;
    xmm0 = XMM_SCALAR(MEMF(0x180DF0)); /* movss */
    eax = esp + 0x2C;
    PUSH32(esp, eax);
    PUSH32(esp, 0xC3);
    PUSH32(esp, edi);
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0002C94Au);
    RECOMP_ABI_CALL(0x000141D0u, sub_000141D0); /* call 0x000141D0 */

loc_0002C94A:;
    eax = MEM32(edi + 0xF0);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    carry_flag = 0;                                       /* logical op clears CF */
    eax = eax | 2;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* or result */
    goto loc_0002C965;

loc_0002C958:;
    eax = MEM32(edi + 0xF0);
    flag_result = (uint32_t)(LO8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(2) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test LO8(eax), 2 (8-bit) */
    carry_flag = 0;                                        /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002C96B; /* je: equal / zero */
    }

loc_0002C962:;
    carry_flag = 0; /* logical op clears CF */
    eax = eax ^ 2;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */

loc_0002C965:;
    MEM32(edi + 0xF0) = eax;

loc_0002C96B:;
    ecx = ZX16(MEM16(esi + 6));
    ebx++;
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    flag_operand = (flag_result == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    flag_result = (uint32_t)(ebx) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(ecx) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp ebx, ecx (32-bit) */
    carry_flag = (int)(flag_result < flag_operand);
    if (CMP_B(flag_result, flag_operand))
    {
        goto loc_0002C7F6; /* jb: below (unsigned <) */
    }

loc_0002C978:;
    eax = MEM32(0x1BF0F8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    edi = MEM32(esp + 0xAC);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C9AE; /* jne: not equal / not zero */
    }

loc_0002C988:;
    eax = MEM32(0x1BF034);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C9AE; /* jne: not equal / not zero */
    }

loc_0002C991:;
    eax = MEM32(0x1BF0D8);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C9AE; /* je: equal / zero */
    }

loc_0002C99A:;
    edx = MEM32(0x1BEF10);
    flag_result = (uint32_t)(MEM32(edx + 0x30)) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0x20000) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* test MEM32(edx + 0x30), 0x20000 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0002C9AE; /* je: equal / zero */
    }

loc_0002C9A9:;
    PUSH32(esp, 0x0002C9AEu);
    RECOMP_ABI_CALL(0x00037B50u, sub_00037B50); /* call 0x00037B50 */

loc_0002C9AE:;
    eax = MEM32(0x1BF104);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002C9C2; /* je: equal / zero */
    }

loc_0002C9B7:;
    PUSH32(esp, edi);
    PUSH32(esp, esi);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0002C9BFu);
    RECOMP_ABI_CALL(0x000331D0u, sub_000331D0); /* call 0x000331D0 */

loc_0002C9BF:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0002C9C2:;
    eax = MEM32(0x1BF034);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002C9D6; /* jne: not equal / not zero */
    }

loc_0002C9CB:;
    eax = MEM32(ebp + 0x40);
    ecx = MEM32(eax + 0x34);
    PUSH32(esp, ecx);
    PUSH32(esp, 1);
    goto loc_0002C9DA;

loc_0002C9D6:;
    PUSH32(esp, 0);
    PUSH32(esp, 0);

loc_0002C9DA:;
    PUSH32(esp, 0x0002C9DFu);
    RECOMP_ABI_CALL(0x00083220u, sub_00083220); /* call 0x00083220 */

loc_0002C9DF:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    PUSH32(esp, edi);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x0002C9E9u);
    RECOMP_ABI_CALL(0x00022F70u, sub_00022F70); /* call 0x00022F70 */

loc_0002C9E9:;
    eax = MEM32(0x1BF0D8);
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0002CA16; /* jne: not equal / not zero */
    }

loc_0002C9F9:;
    eax = MEM32(0x1BF0DC);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    carry_flag = 0;                                         /* nothing borrows from zero */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0002CA16; /* je: equal / zero */
    }

loc_0002CA02:;
    MEM32(0x1BF0D8) = 1;
    MEM32(0x1BF0DC) = 0;

loc_0002CA16:;
    flag_operand = (uint32_t)(0x90) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    carry_flag = (int)((((uint64_t)(esp) + (uint64_t)(0x90)) >> 32) & 1);
    esp = esp + 0x90;
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
