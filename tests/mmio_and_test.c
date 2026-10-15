#include <stdio.h>
#include "platform/mmio_decode.h"

static unsigned reads, writes;
static int last_size;
static uint64_t device_value;

static uint64_t read_register(void *dev, uint32_t off, int size)
{
    (void)dev;
    if (off != 0x34) return 0;
    reads++;
    last_size = size;
    return device_value;
}

static void write_register(void *dev, uint32_t off, uint64_t value, int size)
{
    (void)dev; (void)off; (void)value; (void)size;
    writes++;
}

#define CHECK(test) do { if (!(test)) { printf("FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void)
{
    /* Exact faulting instruction from both supplied crash dumps. */
    unsigned char crash[] = {0x23, 0x14, 0x08};
    unsigned char word[] = {0x66, 0x23, 0x50, 0x34};
    unsigned char wide[] = {0x4C, 0x23, 0x14, 0x25, 0x34, 0, 0, 0};
    unsigned char unsupported[] = {0x23, 0xD0};
    CONTEXT ctx = {0};
    ctx.Rip = (DWORD64)crash;
    ctx.Rdx = 0xFFFFFFFF8000C000ULL;
    ctx.EFlags = 0xA03; /* Carry and overflow must clear; interrupt remains set. */
    device_value = 0x80000000;
    CHECK(mmio_emulate(&ctx, 0x34, NULL, read_register, write_register));
    CHECK(ctx.Rdx == 0x80000000 && ctx.Rip == (DWORD64)crash + sizeof crash);
    CHECK(reads == 1 && writes == 0 && last_size == 4);
    CHECK((ctx.EFlags & 0xAC5) == 0x284); /* IF, sign and even parity. */

    ctx.Rip = (DWORD64)word;
    ctx.Rdx = 0x123456789ABCFFFFULL;
    device_value = 0;
    CHECK(mmio_emulate(&ctx, 0x34, NULL, read_register, write_register));
    CHECK(ctx.Rdx == 0x123456789ABC0000ULL && last_size == 2);
    CHECK(ctx.Rip == (DWORD64)word + sizeof word && (ctx.EFlags & 0xC4) == 0x44);

    ctx.Rip = (DWORD64)wide;
    ctx.R10 = ~0ULL;
    device_value = 0x8000000000000001ULL;
    CHECK(mmio_emulate(&ctx, 0x34, NULL, read_register, write_register));
    CHECK(ctx.R10 == device_value && last_size == 8);
    CHECK(ctx.Rip == (DWORD64)wide + sizeof wide && !(ctx.EFlags & 0x4));
    CHECK(reads == 3 && writes == 0);

    ctx.Rip = (DWORD64)unsupported;
    CHECK(!mmio_emulate(&ctx, 0x34, NULL, read_register, write_register));
    CHECK(reads == 3 && writes == 0 && ctx.Rip == (DWORD64)unsupported);
    {
        unsigned char poll[] = {0xF7, 0x84, 0x06, 0xF0, 0x1F, 0xFE, 0xFF, 0x18, 0, 0, 0};
        ctx.Rip = (DWORD64)poll;
        ctx.Rax = 0x12345678;
        device_value = 0x18;
        CHECK(mmio_emulate(&ctx, 0x34, NULL, read_register, write_register));
        CHECK(ctx.Rax == 0x12345678 && reads == 4 && writes == 0);
        CHECK(ctx.Rip == (DWORD64)poll + sizeof poll && !(ctx.EFlags & 0x40));
    }
    puts("Passed: crash instruction, single MMIO read, no writes, register widths, flags and instruction lengths.");
    return 0;
}
