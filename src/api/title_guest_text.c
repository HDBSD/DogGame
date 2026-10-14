/* Original text-box layout and renderer. Paged-dialogue continuation is a
 * guest pointer returned through EAX; policy converts it back to the source. */
#define RECOMP_GENERATED_CODE
#include "gen/recomp_types.h"
#include "gen/recomp_funcs.h"
#include "title_guest_text.h"
extern uint32_t xbox_HeapAlloc(uint32_t size, uint32_t alignment);

uint32_t title_guest_text_current_box(void)
{
    return MEM32(esp + 4);
}

uint32_t title_guest_text_source(uint32_t box)
{
    return MEM32(box + 0x44);
}

void title_guest_text_set_source(uint32_t box, uint32_t source)
{
    MEM32(box + 0x44) = source;
}

char *title_guest_text_string(uint32_t address)
{
    return (char *)XBOX_PTR(address);
}

uint32_t title_guest_text_allocate_buffer(uint32_t bytes)
{
    return xbox_HeapAlloc(bytes, 16);
}

uint32_t title_guest_text_continuation(void)
{
    return eax;
}

void title_guest_text_set_continuation(uint32_t address)
{
    eax = address;
}

/* Native text renderer retained under a private name for the direct-call override. */
void title_prompt_draw_original(void)
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

loc_0008CE00:;
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* sub source, before the write */
    esp = esp - 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, 0x0008CE09u);
    RECOMP_ABI_CALL(0x000A4340u, sub_000A4340); /* call 0x000A4340 */

loc_0008CE09:;
    ebx = MEM32(esp + 0x14);
    eax = MEM32(ebx);
    flag_result = (uint32_t)(HI8(eax)) & 0xFFu;
    flag_operand = (uint32_t)(1) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* test HI8(eax), 1 (8-bit) */
    if (TEST_Z(flag_result, flag_operand))
    {
        goto loc_0008CE23; /* je: equal / zero */
    }

loc_0008CE14:;
    PUSH32(esp, ebx);
    PUSH32(esp, 0x0008CE1Au);
    RECOMP_ABI_CALL(0x000A4380u, sub_000A4380); /* call 0x000A4380 */

loc_0008CE1A:;
    PUSH32(esp, ebx);
    PUSH32(esp, 0x0008CE20u);
    RECOMP_ABI_CALL(0x000A44F0u, sub_000A44F0); /* call 0x000A44F0 */

loc_0008CE20:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */

loc_0008CE23:;
    eax = MEM32(ebx + 0x44);
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0008CF43; /* je: equal / zero */
    }

loc_0008CE2E:;
    xmm0 = XMM_SCALAR(MEMF(ebx + 0xC));       /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(ebx + 4);    /* addss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ebx + 0x38); /* subss */
    xmm1 = XMM_SCALAR(MEMF(ebx + 0x10));      /* movss */
    xmm2 = XMM_SCALAR(MEMF(ebx + 0x34));      /* movss */
    xmm1.f[0] = xmm1.f[0] + MEMF(ebx + 8);    /* addss */
    xmm4 = XMM_ZERO();                        /* xorps self = zero */
    float_compare_left = xmm4.f[0];
    float_compare_right = xmm0.f[0];          /* comiss */
    xmm2.f[0] = xmm2.f[0] + MEMF(ebx + 4);    /* addss */
    xmm3 = XMM_SCALAR(MEMF(ebx + 0x3C));      /* movss */
    xmm1.f[0] = xmm1.f[0] - MEMF(ebx + 0x40); /* subss */
    xmm3.f[0] = xmm3.f[0] + MEMF(ebx + 8);    /* addss */
    MEMF(esp + 0x14) = xmm2.f[0];             /* movss */
    MEMF(esp + 0xC) = xmm0.f[0];              /* movss */
    MEMF(esp + 4) = xmm1.f[0];                /* movss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0008CF43; /* ja: above (unsigned >) (xmm4.f[0] vs xmm0.f[0]) */
    }

loc_0008CE7E:;
    float_compare_left = xmm4.f[0];
    float_compare_right = xmm1.f[0]; /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0008CF43; /* ja: above (unsigned >) (xmm4.f[0] vs xmm1.f[0]) */
    }

loc_0008CE87:;
    float_compare_left = xmm2.f[0];
    float_compare_right = MEMF(0x1906EC); /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0008CF43; /* ja: above (unsigned >) (xmm2.f[0] vs MEMF(0x1906EC)) */
    }

loc_0008CE94:;
    float_compare_left = xmm3.f[0];
    float_compare_right = MEMF(0x18D5D8); /* comiss */
    if ((float_compare_left > float_compare_right))
    {
        goto loc_0008CF43; /* ja: above (unsigned >) (xmm3.f[0] vs MEMF(0x18D5D8)) */
    }

loc_0008CEA1:;
    PUSH32(esp, esi);
    MEMF(0x1C50DC) = xmm2.f[0]; /* movss */
    MEMF(0x1C50E0) = xmm3.f[0]; /* movss */
    esi = MEM32(ebx + 0x44);
    PUSH32(esp, ebx);
    PUSH32(esp, 0x0008CEBBu);
    RECOMP_ABI_CALL(0x000A43C0u, sub_000A43C0); /* call 0x000A43C0 */

loc_0008CEBB:;
    flag_operand = (uint32_t)(4) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 4;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp esi, 0 (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0008CF23; /* je: equal / zero */
    }

loc_0008CEC2:;
    flag_result = (uint32_t)(MEM8(esi)) & 0xFFu;
    flag_operand = (uint32_t)(0) & 0xFFu;
    signed_flag_result = (int32_t)(int8_t)(flag_result);
    signed_flag_operand = (int32_t)(int8_t)(flag_operand); /* cmp MEM8(esi), 0 (8-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0008CF34; /* je: equal / zero */
    }

loc_0008CEC7:;
    fp_push(MEMF(esp + 8));                             /* fld float */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10));                /* movss */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x1C50E0)); /* fsub dword ptr [0x1c50e0] */
    xmm0.f[0] = xmm0.f[0] - MEMF(esp + 0x18);           /* subss */
    MEMF(esp + 0xC) = (float)fp_top();
    fp_pop(); /* fstp */
    eax = MEM32(esp + 0xC);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    eax = esi;
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0008CEF3u);
    RECOMP_ABI_CALL(0x0008C9C0u, sub_0008C9C0); /* call 0x0008C9C0 */

loc_0008CEF3:;
    flag_operand = (uint32_t)(8) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 8;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, esi (32-bit) */
    if (CMP_EQ(flag_result, flag_operand))
    {
        goto loc_0008CF27; /* je: equal / zero */
    }

loc_0008CEFA:;
    flag_result = (uint32_t)(eax) & 0xFFFFFFFFu;
    flag_operand = (uint32_t)(0) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result);
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* cmp eax, 0 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x18));                    /* movss */
    MEMF(0x1C50DC) = xmm0.f[0];                             /* movss */
    xmm0 = XMM_SCALAR(MEMF(ebx + 0x2C));                    /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x1C50E0);                 /* addss */
    esi = eax;
    MEMF(0x1C50E0) = xmm0.f[0]; /* movss */
    if (CMP_NE(flag_result, flag_operand))
    {
        goto loc_0008CEC2; /* jne: not equal / not zero */
    }

loc_0008CF23:;
    esi = MEM32(esp + 0x18);

loc_0008CF27:;
    PUSH32(esp, 0x0008CF2Cu);
    RECOMP_ABI_CALL(0x000A4440u, sub_000A4440); /* call 0x000A4440 */

loc_0008CF2C:;
    eax = esi;
    POP32(esp, esi);
    POP32(esp, ebx);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0008CF34:;
    esi = 0; /* xor self */
    flag_result = (uint32_t)(esi) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* xor result */
    PUSH32(esp, 0x0008CF3Bu);
    RECOMP_ABI_CALL(0x000A4440u, sub_000A4440); /* call 0x000A4440 */

loc_0008CF3B:;
    eax = esi;
    POP32(esp, esi);
    POP32(esp, ebx);
    flag_operand = (uint32_t)(0xC) & 0xFFFFFFFFu;
    signed_flag_operand = (int32_t)(int32_t)(flag_operand); /* add source, before the write */
    esp = esp + 0xC;
    flag_result = (uint32_t)(esp) & 0xFFFFFFFFu;
    signed_flag_result = (int32_t)(int32_t)(flag_result); /* add result */
    esp += 4;
    return; /* ret */

loc_0008CF43:;
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

#undef fp_push
#undef fp_pop
#undef fp_top
#undef fp_st
#undef fp_st1
}
