"""Run the experimental Vulkan backend on the real driver, offscreen.

Requires a Vulkan-capable driver; no SDK, shader compiler or game assets.
"""
from pathlib import Path
import subprocess
from test_support import msbuild

root = Path(__file__).resolve().parents[1]
out = root / "tests/irq-test-build"
backend = root / "runtime_overrides/src/kernel"
code = r'''
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static bool missing_loader;
static HMODULE WINAPI test_load_library(LPCWSTR name, HANDLE file, DWORD flags)
{
    return missing_loader ? nullptr : LoadLibraryExW(name, file, flags);
}
#define LoadLibraryExW test_load_library
'''
code += f'#include "{(backend / "nv2a_vulkan.cpp").as_posix()}"\n'
code += f'#include "{(backend / "nv2a_combiner.c").as_posix()}"\n'
code += r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static bool internal_scaling;
static unsigned selected_width = 1280, selected_height = 960;
extern "C" int xbox_DisplayInternalScaling(void) { return internal_scaling; }
extern "C" unsigned xbox_DisplayResolutionSelected(void) { return 0; }
extern "C" void xbox_DisplayResolutionSize(unsigned, unsigned *width, unsigned *height)
{ *width = selected_width; *height = selected_height; }
static uint32_t pixels[64*64], other[64*64];
static float depths[64*64];
static unsigned decode_calls;
static uint32_t texture_invert;
static PFN_vkInvalidateMappedMemoryRanges real_invalidate;
static unsigned invalidations;
static VKAPI_ATTR VkResult VKAPI_CALL observe_invalidate(VkDevice device, uint32_t count, const VkMappedMemoryRange *ranges)
{
    if (count != 1 || ranges[0].memory != readback_memory || ranges[0].offset != 0 || ranges[0].size != VK_WHOLE_SIZE)
    {
        return VK_ERROR_MEMORY_MAP_FAILED;
    }
    invalidations++;
    return real_invalidate(device, count, ranges);
}
static int decode(void *, unsigned x, unsigned y, uint32_t *pixel)
{
    *pixel = y ? (x ? 0xFFFFFFFF : 0xFF0000FF) : (x ? 0xFF00FF00 : 0xFFFF0000);
    *pixel ^= texture_invert;
    decode_calls++;
    return 1;
}
static int refuse(void *, unsigned, unsigned, uint32_t *) { return 0; }
static uint64_t swizzled(unsigned, unsigned x, unsigned y)
{
    unsigned offset = 0;
    for (unsigned bit = 0; bit < 6; bit++)
    {
        offset |= ((x >> bit) & 1) << (2 * bit);
        offset |= ((y >> bit) & 1) << (2 * bit + 1);
    }
    return offset * 4;
}
static void quad(float z, float r, float g, float b, float a, bool indexed = false)
{
    const float xy[6][2] = {{0,0},{64,0},{0,64},{64,0},{64,64},{0,64}};
    NvGpuVertex v[6] = {};
    for (unsigned i = 0; i < 6; i++)
    {
        v[i].pos[0] = xy[i][0]; v[i].pos[1] = xy[i][1]; v[i].pos[2] = z; v[i].pos[3] = 1;
        v[i].color[0] = r; v[i].color[1] = g; v[i].color[2] = b; v[i].color[3] = a;
        v[i].tex[0] = xy[i][0] / 64; v[i].tex[1] = xy[i][1] / 64; v[i].tex[3] = 1;
        for (unsigned j = 0; j < 3; j++) memcpy(v[i].extra[j], v[i].tex, 16);
        v[i].fog = 1;
    }
    if (indexed)
    {
        for (unsigned i = 0; i < 6; i++) nv_vk_append_vertex(v + i);
        unsigned a[3] = {0,1,2}, b[3] = {3,4,5};
        nv_vk_indexed_triangle(a); nv_vk_indexed_triangle(b);
    }
    else
    {
        nv_vk_triangle(v); nv_vk_triangle(v + 3);
    }
    nv_vk_end();
}
int main()
{
    char error[512];
    missing_loader = true;
    CHECK(!nv_vk_initialize(error, sizeof error));
    CHECK(strstr(error, "Vulkan loader unavailable"));
    CHECK(!device && !instance && !loader);
    missing_loader = false;
    CHECK(nv_vk_initialize(error, sizeof error) || (fprintf(stderr, "%s\n", error), false));
    // Prefer cached readback, including noncoherent cached memory over an
    // uncached coherent type. Upload/device-local callers retain their rules.
    VkPhysicalDeviceMemoryProperties saved_memory = memory_properties;
    memory_properties = {};
    memory_properties.memoryTypeCount = 3;
    memory_properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    memory_properties.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    memory_properties.memoryTypes[2].propertyFlags = memory_properties.memoryTypes[1].propertyFlags | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    CHECK(memory_type(7, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 2);
    CHECK(memory_type(3, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 1);
    CHECK(memory_type(1, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 0);
    CHECK(memory_type(7, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0);
    bool rejected_memory = false;
    try { memory_type(7, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); }
    catch (const std::exception &) { rejected_memory = true; }
    CHECK(rejected_memory);
    memory_properties = saved_memory;
    NvGpuDraw d = {};
    d.color = (uint8_t *)pixels; d.depth = depths;
    d.width = d.height = d.clip_w = d.clip_h = d.depth_stride = 64;
    d.pitch = 256; d.zscale = 1; d.src = 1; d.mask = 0x01010101; d.mode = 2;
    d.depth_test = d.depth_write = 1; d.depth_func = 0x201;
    for (float &z : depths) z = 1;
    CHECK(nv_vk_begin(&d)); quad(.5f, 1, 0, 0, 1);
    CHECK(nv_vk_begin(&d)); quad(.8f, 0, 1, 0, 1);
    CHECK(pixels[32*64+32] == 0); // no guest publication before sync
    nv_vk_sync();
    CHECK(pixels[32*64+32] == 0xFFFF0000);
    CHECK(fabs(depths[32*64+32] - .5f) < .00001f);
    // Force both fallback branches on this driver's actual mapped allocation.
    // This checks invalidation/range handling, not another device's cache model.
    VkMemoryPropertyFlags saved_flags = readback_memory_flags;
    real_invalidate = vkInvalidateMappedMemoryRanges;
    vkInvalidateMappedMemoryRanges = observe_invalidate;
    readback_memory_flags &= ~(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    CHECK(nv_vk_begin(&d)); quad(.3f, 1, 0, 0, 1); nv_vk_sync();
    CHECK(invalidations == 1 && pixels[32*64+32] == 0xFFFF0000);
    CHECK(fabs(depths[32*64+32] - .3f) < .00001f);
    readback_memory_flags = saved_flags;
    vkInvalidateMappedMemoryRanges = real_invalidate;
    d.depth_test = d.depth_write = 0;
    d.blend = 1; d.src = 0x302; d.dst = 0x303;
    CHECK(nv_vk_begin(&d)); quad(.2f, 0, 0, 1, .5f, true); nv_vk_sync();
    CHECK(((pixels[32*64+32] >> 16) & 255) >= 127 && ((pixels[32*64+32] >> 16) & 255) <= 128);
    CHECK((pixels[32*64+32] & 255) >= 127 && (pixels[32*64+32] & 255) <= 128);
    d.blend = 0; d.alpha_test = 1; d.alpha_func = 0x204; d.alpha_ref = 200;
    uint32_t before = pixels[32*64+32];
    CHECK(nv_vk_begin(&d)); quad(.2f, 0, 1, 0, .1f); nv_vk_sync();
    CHECK(pixels[32*64+32] == before);
    d.alpha_test = 0; d.textured = 1; d.tw = d.th = 2; d.decode = decode;
    d.addr_u = d.addr_v = 3; d.mode = 1; d.combiner.stage_program = 1;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(pixels[8*64+8] == 0xFFFF0000); // Y orientation and BGRA interpretation
    CHECK(pixels[8*64+56] == 0xFF00FF00);
    CHECK(pixels[56*64+8] == 0xFF0000FF);
    d.linear = 1;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    unsigned red = (pixels[32*64+32] >> 16) & 255;
    CHECK(red > 100 && red < 160);
    d.linear = 0; d.mode = 0; d.combiner.control = 1;
    d.combiner.color_icw[0] = 0x08090000; d.combiner.alpha_icw[0] = 0x18190000;
    d.combiner.color_ocw[0] = d.combiner.alpha_ocw[0] = 0xC00;
    d.extra[0].valid = 1; d.extra[0].width = d.extra[0].height = 2;
    d.extra[0].decode = decode; d.extra[0].addr_u = d.extra[0].addr_v = 3;
    d.combiner.stage_program = 1 | (1 << 5);
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(pixels[8*64+8] == 0xFFFF0000 && pixels[56*64+8] == 0xFF0000FF);
    d.extra[0].valid = 0; d.textured = 0; d.mode = 2;
    d.clip_w = 32;
    CHECK(nv_vk_begin(&d)); quad(.1f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(pixels[8*64+8] == 0xFF00FF00 && pixels[8*64+56] == 0xFF00FF00);
    CHECK(pixels[56*64+56] == 0xFFFFFFFF);
    d.clip_w = 64;
    CHECK(nv_vk_begin(&d)); quad(.1f, 0, 0, 1, 1);
    uint64_t reads = readbacks;
    nv_vk_texture_barrier((const uint8_t *)other, sizeof other); CHECK(readbacks == reads);
    nv_vk_texture_barrier((const uint8_t *)pixels, sizeof pixels); CHECK(readbacks == reads + 1);
    CHECK(pixels[0] == 0xFF0000FF);
    // CPU edits must survive reload and channel write masking.
    pixels[0] = 0xFFAA3344; d.mask = 0x100;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(pixels[0] == 0xFFAAFF44);
    d.mask = 0x01010101; d.surface_offset = swizzled; d.surface_format = 1;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 0, 1, 1); nv_vk_sync();
    CHECK(pixels[swizzled(1, 17, 29)/4] == 0xFFFF00FF);
    d.surface_offset = nullptr; d.color = (uint8_t *)other;
    CHECK(nv_vk_begin(&d)); quad(.1f, 0, 1, 1, 1); nv_vk_sync();
    CHECK(other[0] == 0xFF00FFFF && pixels[0] == 0xFFFF00FF);
    d.textured = 1; d.mode = 1; d.combiner.stage_program = 1;
    d.tw = d.th = 2; d.decode = decode; d.texture_key = 555; d.texture_hash = 1;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    unsigned calls = decode_calls;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(decode_calls == calls && other[8*64+8] == 0xFFFF0000);
    d.texture_hash = 2; texture_invert = 0x00FFFFFF;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(decode_calls == calls + 4 && other[8*64+8] == 0xFF00FFFF);
    // Reusing cached textures must still retire descriptor sets before the
    // bounded pool fills, even when no new images have been allocated.
    for (unsigned i = 0; i < 600; i++)
    {
        CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1);
    }
    nv_vk_sync(); CHECK(other[8*64+8] == 0xFF00FFFF);
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); // queued old hash
    d.texture_hash = 3; texture_invert = 0; d.clip_w = 32;
    CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(other[8*64+8] == 0xFFFF0000 && other[8*64+56] == 0xFFFF00FF);
    d.clip_w = 64;
    NvGpuDraw static_texture = d;
    static_texture.texture_key = 777; static_texture.texture_hash = 1;
    CHECK(nv_vk_begin(&static_texture)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    size_t entries = cached_textures.size();
    for (unsigned update = 0; update < 300; update++)
    {
        d.texture_hash = 4 + update;
        CHECK(nv_vk_begin(&d)); quad(.1f, 1, 1, 1, 1);
    }
    CHECK(cached_textures.size() == entries); // versions do not fill the cache
    calls = decode_calls;
    CHECK(nv_vk_begin(&static_texture)); quad(.1f, 1, 1, 1, 1); nv_vk_sync();
    CHECK(decode_calls == calls && other[0] == 0xFFFF0000);
    d.textured = 1; d.tw = d.th = 2; d.decode = refuse;
    d.texture_key = 0;
    CHECK(!nv_vk_begin(&d)); CHECK(other[0] == 0xFFFF0000);
    d.width = 1025; CHECK(!nv_vk_begin(&d));
    uint32_t padded_pixels[68*64];
    float padded_depth[68*64];
    for (unsigned i = 0; i < 68*64; i++) { padded_pixels[i] = 0x11223344; padded_depth[i] = 13; }
    NvGpuDraw padded = {};
    padded.color = (uint8_t *)padded_pixels; padded.depth = padded_depth;
    padded.width = padded.height = padded.clip_w = padded.clip_h = 64;
    padded.pitch = 68*4; padded.depth_stride = 68;
    padded.zscale = 13; padded.mask = 0x01010101; padded.mode = 2;
    padded.depth_test = padded.depth_write = 1; padded.depth_func = 0x201;
    CHECK(nv_vk_begin(&padded)); quad(.25f, 0, 1, 0, 1); nv_vk_sync();
    for (unsigned y = 0; y < 64; y++)
    {
        CHECK(padded_pixels[y*68] == 0xFF00FF00 && fabs(padded_depth[y*68] - .25f) < .00001f);
        for (unsigned x = 64; x < 68; x++)
        {
            CHECK(padded_pixels[y*68+x] == 0x11223344 && padded_depth[y*68+x] == 13);
        }
    }
    NvGpuDraw c = padded;
    c.color = (uint8_t *)pixels; c.depth = depths; c.pitch = 256; c.depth_stride = 64; c.zscale = 1;
    for (unsigned i = 0; i < 4096; i++) { pixels[i] = 0; depths[i] = 1; }
    CHECK(nv_vk_begin(&c)); quad(.2f, 1, 0, 0, 1);
    NvGpuClear clear = {};
    clear.depth = depths; clear.depth_value = 1;
    uint64_t clear_reads = readbacks;
    CHECK(nv_vk_clear_attachments(&clear));
    for (float &z : depths) z = 1; // executor's immediate CPU mirror
    CHECK(readbacks == clear_reads && color_dirty && !depth_dirty);
    c.clip_w = 32; c.depth_write = 0;
    CHECK(nv_vk_begin(&c)); quad(.8f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(pixels[8*64+8] == 0xFF00FF00 && pixels[8*64+56] == 0xFFFF0000);
    CHECK(depths[8*64+8] == 1);
    c.clip_w = 64; c.depth_write = 1;
    CHECK(nv_vk_begin(&c)); quad(.2f, 1, 0, 0, 1);
    clear = {}; clear.color = (uint8_t *)pixels; clear.pitch = 256;
    clear.width = clear.height = 64; clear.color_value = 0xFF0000FF;
    CHECK(nv_vk_clear_attachments(&clear));
    for (uint32_t &pixel : pixels) pixel = clear.color_value;
    CHECK(!color_dirty && depth_dirty);
    CHECK(nv_vk_begin(&c)); quad(.8f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(pixels[0] == 0xFF0000FF && fabs(depths[0] - .2f) < .00001f);
    CHECK(nv_vk_begin(&c)); quad(.1f, 1, 0, 0, 1);
    clear.depth = depths; clear.depth_value = 1; clear.color_value = 0xFF123456;
    CHECK(nv_vk_clear_attachments(&clear));
    for (unsigned i = 0; i < 4096; i++) { pixels[i] = clear.color_value; depths[i] = 1; }
    c.clip_w = 32;
    CHECK(nv_vk_begin(&c)); quad(.7f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(pixels[8*64+8] == 0xFF00FF00 && pixels[8*64+56] == 0xFF123456);
    c.clip_w = 64;
    CHECK(nv_vk_begin(&c)); quad(.1f, 1, 0, 0, 1);
    clear.depth = nullptr; clear.color_value = 0xFF0000FF;
    CHECK(nv_vk_clear_attachments(&clear));
    for (uint32_t &pixel : pixels) pixel = clear.color_value;
    pixels[8*64+56] = 0xFFFFFF00; // CPU edit following its matching clear
    c.clip_w = 32; c.depth_test = 0; c.depth_write = 0;
    CHECK(nv_vk_begin(&c)); quad(.8f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(pixels[8*64+56] == 0xFFFFFF00 && fabs(depths[0] - .1f) < .00001f);
    CHECK(nv_vk_begin(&c)); quad(.1f, 1, 0, 0, 1);
    clear.width = 63;
    CHECK(!nv_vk_clear_attachments(&clear)); CHECK(color_dirty);
    clear.width = 64; clear.depth = depths; clear.depth_value = NAN;
    CHECK(!nv_vk_clear_attachments(&clear)); CHECK(color_dirty);
    nv_vk_sync();
    for (uint32_t &pixel : pixels) pixel = 0;
    // CPU fallback clears must retain untouched color pixels, but need not
    // publish depth that the executor immediately overwrites in its entirety.
    c.clip_w = 64; c.depth_test = c.depth_write = 1;
    CHECK(nv_vk_begin(&c)); quad(.05f, 0, 1, 0, 1);
    uint64_t unrelated_waits = wait_ticks;
    nv_vk_prepare_cpu_clear((uint8_t *)padded_pixels, 68*4, 0, 0, 0, 64, 64, padded_depth);
    CHECK(recording && color_dirty && depth_dirty && wait_ticks == unrelated_waits);
    for (uint32_t &pixel : padded_pixels) pixel = 0;
    for (float &z : padded_depth) z = 1;
    nv_vk_prepare_cpu_clear((uint8_t *)pixels, 256, 0, 0, 0, 32, 64, depths);
    CHECK(pixels[8*64+56] == 0xFF00FF00);
    for (unsigned y = 0; y < 64; y++)
        for (unsigned x = 0; x < 32; x++) pixels[y*64+x] = 0xFF123456;
    for (float &z : depths) z = 1;
    CHECK(nv_vk_begin(&c)); quad(.8f, 0, 0, 1, 1); nv_vk_sync();
    CHECK(pixels[8*64+56] == 0xFF0000FF && fabs(depths[0] - .8f) < .00001f);
    // A full overwrite needs neither attachment published, yet still waits
    // for the submission before invalidating resources and allowing CPU writes.
    CHECK(nv_vk_begin(&c)); quad(.1f, 1, 0, 0, 1);
    uint64_t overwritten_reads = readbacks;
    nv_vk_prepare_cpu_clear((uint8_t *)pixels, 256, 0, 0, 0, 64, 64, depths);
    CHECK(readbacks == overwritten_reads && !recording);
    for (uint32_t &pixel : pixels) pixel = 0xFF123456;
    for (float &z : depths) z = 1;
    CHECK(nv_vk_begin(&c)); quad(.8f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(pixels[0] == 0xFF00FF00 && fabs(depths[0] - .8f) < .00001f);
    c.depth_test = c.depth_write = 0;
    for (uint32_t &pixel : pixels) pixel = 0;
    c.clip_w = 64;
    uint64_t waits_before = wait_ticks, reads_before = readbacks, early_before = early_submissions;
    uint64_t passes_before = render_passes;
    for (unsigned draw = 0; draw < 80; draw++)
    {
        CHECK(nv_vk_begin(&c)); quad(.1f, 0, 0, 1, 1);
    }
    CHECK(early_submissions >= early_before + 2);
    CHECK(render_passes - passes_before <= 3); // compatible draws share each segment's pass
    CHECK(wait_ticks == waits_before && readbacks == reads_before && pixels[0] == 0);
    nv_vk_sync(); CHECK(pixels[0] == 0xFF0000FF);
    // Blend order must remain stable within a shared pass and across early
    // submissions. Repeated red/blue half-alpha draws converge to 85/170.
    for (uint32_t &pixel : pixels) pixel = 0;
    c.blend = 1; c.src = 0x302; c.dst = 0x303;
    for (unsigned pair = 0; pair < 64; pair++)
    {
        CHECK(nv_vk_begin(&c)); quad(.1f, 1, 0, 0, .5f);
        CHECK(nv_vk_begin(&c)); quad(.1f, 0, 0, 1, .5f);
    }
    nv_vk_sync();
    CHECK(abs((int)((pixels[0] >> 16) & 255) - 85) <= 2);
    CHECK(abs((int)(pixels[0] & 255) - 170) <= 2);
    // Scale actual geometry, not an expanded native-resolution image. A tiny
    // triangle inside a single guest pixel has distinct physical samples.
    nv_vk_sync();
    static uint32_t large_color[640*480];
    static float large_depth[640*480];
    std::fill(large_depth, large_depth + 640*480, 1.f);
    NvGpuDraw scaled = {};
    scaled.color = (uint8_t *)large_color;
    scaled.width = scaled.clip_w = 640;
    scaled.height = scaled.clip_h = 480;
    scaled.pitch = 640*4;
    scaled.depth = large_depth;
    scaled.depth_stride = 640;
    scaled.zscale = 1;
    scaled.depth_test = scaled.depth_write = 1;
    scaled.depth_func = 0x201;
    scaled.mask = 0x01010101;
    scaled.mode = 2;
    scaled.src = 1;
    internal_scaling = true;
    CHECK(nv_vk_begin(&scaled));
    NvGpuVertex tiny[3] = {};
    const float positions[3][2] = {{20,20},{20.8f,20},{20,20.8f}};
    for (unsigned i = 0; i < 3; i++)
    {
        tiny[i].pos[0] = positions[i][0]; tiny[i].pos[1] = positions[i][1];
        tiny[i].pos[2] = .25f; tiny[i].pos[3] = 1;
        tiny[i].color[0] = tiny[i].color[3] = 1;
    }
    nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(target_width == 1280 && target_height == 960);
    unsigned high_width = 0, high_height = 0;
    const uint32_t *high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high_width == 1280 && high_height == 960);
    CHECK(high[40*1280+40] == 0xFFFF0000);
    CHECK(high[41*1280+41] == 0);
    CHECK(large_color[20*640+20] == 0);
    CHECK(large_depth[20*640+20] == 1.f);
    // Preserve physical color across native publication/reload. A masked
    // green-channel write must not erase the subpixel red triangle.
    scaled.mask = 0x00000100;
    CHECK(nv_vk_begin(&scaled));
    nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[40*1280+40] == 0xFFFF0000);
    // Physical depth must survive publication even when the guest's center
    // sample misses the tiny triangle entirely.
    scaled.mask = 0x01010101;
    for (NvGpuVertex &v : tiny)
    {
        v.pos[2] = .75f; v.color[0] = 0; v.color[2] = 1;
    }
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[40*1280+40] == 0xFFFF0000);
    // A native clear can leave the old center sample (1) unchanged while it
    // must still erase subpixel depth. The CPU-clear boundary invalidates it.
    nv_vk_discard_snapshots(nullptr, large_depth);
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[40*1280+40] == 0xFF0000FF);
    large_depth[20*640+20] = .5f;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    large_color[0] = 0xFF00FF00;
    CHECK(!nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height));
    // Settings changes invalidate stale snapshots and resize physical targets.
    selected_width = 1920; selected_height = 1080;
    CHECK(!nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height));
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(target_width == 1920 && target_height == 1080);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[0] == 0xFF00FF00);
    selected_width = 2560; selected_height = 1440;
    scaled.clip_x = 21; scaled.clip_w = 619;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(target_width == 2560 && target_height == 1440);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high_width == 2560 && high_height == 1440);
    CHECK(high[60*2560+80] == large_color[20*640+20]);
    CHECK(high[0] == 0xFF00FF00);
    selected_width = 3840; selected_height = 2160;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(target_width == 3840 && target_height == 2160);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high_width == 3840 && high_height == 2160);
    // Clear a pooled main target while an intermediate target is bound.
    NvGpuDraw intermediate = scaled;
    intermediate.color = (uint8_t *)pixels; intermediate.depth = depths;
    intermediate.width = intermediate.height = intermediate.clip_w = intermediate.clip_h = 64;
    intermediate.clip_x = intermediate.clip_y = 0;
    intermediate.pitch = 256; intermediate.depth_stride = 64;
    CHECK(nv_vk_begin(&intermediate)); quad(.1f, 0, 1, 0, 1); nv_vk_sync();
    NvGpuClear pooled_clear = {};
    pooled_clear.color = (uint8_t *)large_color;
    pooled_clear.pitch = 640*4;
    pooled_clear.width = 640; pooled_clear.height = 480;
    pooled_clear.color_value = 0xFF101010;
    CHECK(nv_vk_clear_attachments(&pooled_clear));
    std::fill(large_color, large_color + 640*480, pooled_clear.color_value);
    nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[0] == pooled_clear.color_value && high[3840*2160-1] == pooled_clear.color_value);
    uint64_t uploads_before = color_upload_bytes + depth_upload_bytes;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(color_upload_bytes + depth_upload_bytes == uploads_before);
    uint64_t resolves_before = native_resolves;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end();
    nv_vk_sync_reason("test native consumer");
    CHECK(!native_resolve_supported || native_resolves == resolves_before + 1);
    CHECK(depth_read_width == 3840 && depth_read_height == 480);
    CHECK(color_read_width == (native_resolve_supported ? 640u : 3840u));
    CHECK(large_color[0] == 0xFF101010);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high_width == 3840 && high_height == 2160);
    bool saved_resolve_support = native_resolve_supported;
    native_resolve_supported = false;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end();
    nv_vk_sync_reason("test native fallback");
    CHECK(color_read_width == 3840 && large_color[0] == 0xFF101010);
    native_resolve_supported = saved_resolve_support;
    scaled.clip_x = 0; scaled.clip_w = 640;
    for (NvGpuVertex &v : tiny)
    {
        v.pos[2] = .1f; v.color[0] = 1; v.color[2] = 0;
    }
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[90*3840+120] == 0xFFFF0000);
    CHECK(high[93*3840+123] == 0xFF101010);
    CHECK(large_color[20*640+20] == 0xFF101010);
    // Changed native data uploads at native size, then expands on the GPU.
    // Existing physical snapshots cannot hide either of these CPU edits.
    large_color[0] = 0xFF00FF00;
    large_depth[0] = .125f;
    uint64_t color_bytes_before = color_upload_bytes, depth_bytes_before = depth_upload_bytes;
    uint64_t expansions_before = native_upload_expansions;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    uint64_t expected_upload = native_resolve_supported ? 640*480*4 : 3840*2160*4;
    CHECK(color_upload_bytes - color_bytes_before == expected_upload);
    CHECK(depth_upload_bytes - depth_bytes_before == expected_upload);
    CHECK(!native_resolve_supported || native_upload_expansions == expansions_before + 2);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[0] == 0xFF00FF00 && large_depth[0] == .125f);
    // Devices without the blit features retain the conservative full upload.
    saved_resolve_support = native_resolve_supported;
    native_resolve_supported = false;
    large_color[0] = 0xFF0000FF;
    large_depth[0] = .25f;
    color_bytes_before = color_upload_bytes; depth_bytes_before = depth_upload_bytes;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(color_upload_bytes - color_bytes_before == 3840*2160*4);
    CHECK(depth_upload_bytes - depth_bytes_before == 3840*2160*4);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[0] == 0xFF0000FF && large_depth[0] == .25f);
    native_resolve_supported = saved_resolve_support;
    // Evict the main target while retaining its validated depth history.
    // Reload must restore subpixel depth GPU-to-GPU, without a host upload.
    static uint32_t evict_colors[8][64*64];
    for (unsigned i = 0; i < 8; i++)
    {
        intermediate.color = (uint8_t *)evict_colors[i];
        intermediate.depth = nullptr;
        CHECK(nv_vk_begin(&intermediate)); quad(.1f, 0, 1, 0, 1); nv_vk_sync();
    }
    depth_bytes_before = depth_upload_bytes;
    uint64_t gpu_history_before = gpu_depth_history_bytes;
    for (NvGpuVertex &v : tiny)
    {
        v.pos[2] = .2f; v.color[0] = 0; v.color[2] = 1;
    }
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(depth_upload_bytes == depth_bytes_before);
    CHECK(gpu_depth_history_bytes == gpu_history_before + 3840ull*2160*4);
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[90*3840+120] == 0xFFFF0000);
    // The row-packed depth range must also work through the uncached-memory
    // bulk-copy fallback, without treating it as a full physical attachment.
    saved_flags = readback_memory_flags;
    readback_memory_flags &= ~VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[90*3840+120] == 0xFFFF0000);
    CHECK(large_depth[0] == .25f);
    readback_memory_flags = saved_flags;
    NvGpuDraw no_depth_test = scaled;
    no_depth_test.depth_test = 0;
    no_depth_test.depth_write = 1;
    for (NvGpuVertex &v : tiny)
    {
        v.pos[2] = .9f; v.color[0] = v.color[2] = 0; v.color[1] = 1;
    }
    CHECK(nv_vk_begin(&no_depth_test)); nv_vk_triangle(tiny); nv_vk_end();
    CHECK(depth_dirty);
    nv_vk_sync();
    // The maintained write-only pipeline uses ALWAYS when the guest test is
    // disabled but its write mask is set. Its .9 write must admit .2 blue.
    for (NvGpuVertex &v : tiny)
    {
        v.pos[2] = .2f; v.color[1] = 0; v.color[2] = 1;
    }
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[90*3840+120] == 0xFF0000FF);
    // Rebinding another depth allocation to the same color surface must carry
    // forward physical subpixel color, even when native samples cannot see it.
    static float alternate_large_depth[640*480];
    std::fill(alternate_large_depth, alternate_large_depth + 640*480, 1.f);
    NvGpuDraw alternate_scaled = scaled;
    alternate_scaled.depth = alternate_large_depth;
    for (NvGpuVertex &v : tiny)
    {
        v.pos[0] += 2; v.pos[2] = .1f;
        v.color[0] = v.color[2] = 0; v.color[1] = 1;
    }
    CHECK(nv_vk_begin(&alternate_scaled)); nv_vk_triangle(tiny); nv_vk_end();
    nv_vk_sync_color_reason("presentation");
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[90*3840+120] == 0xFF0000FF && high[90*3840+132] == 0xFF00FF00);
    for (NvGpuVertex &v : tiny)
    {
        v.pos[0] += 2; v.color[0] = 1; v.color[1] = 0;
    }
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    high = nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height);
    CHECK(high && high[90*3840+120] == 0xFF0000FF && high[90*3840+132] == 0xFF00FF00);
    internal_scaling = false;
    CHECK(nv_vk_begin(&scaled)); nv_vk_triangle(tiny); nv_vk_end(); nv_vk_sync();
    CHECK(target_width == 640 && target_height == 480);
    CHECK(!nv_vk_present_image((uint8_t *)large_color, 640*4, &high_width, &high_height));
    nv_vk_report(); nv_vk_shutdown(); nv_vk_shutdown();
    CHECK(nv_vk_initialize(error, sizeof error));
    // GPU-only depth ownership: two color targets sharing one depth allocation
    // must see the newest physical depth without publishing it through RAM.
    static uint32_t owned_color[3][64*64];
    static float owned_depth[64*64], other_depth[64*64];
    std::fill(owned_depth, owned_depth + 64*64, 1.f);
    std::fill(other_depth, other_depth + 64*64, 1.f);
    NvGpuDraw owned = {};
    owned.color = (uint8_t *)owned_color[0]; owned.depth = owned_depth;
    owned.width = owned.height = owned.clip_w = owned.clip_h = 64;
    owned.pitch = 256; owned.depth_stride = 64; owned.zscale = 1;
    owned.depth_test = owned.depth_write = 1; owned.depth_func = 0x201;
    owned.mask = 0x01010101; owned.mode = 2; owned.depth_generation = 1;
    CHECK(nv_vk_begin(&owned)); quad(.2f, 1, 0, 0, 1);
    nv_vk_sync_color_reason("presentation");
    CHECK(owned_depth[0] == 1.f && depth_dirty && !recording);
    owned.color = (uint8_t *)owned_color[1];
    CHECK(nv_vk_begin(&owned)); quad(.8f, 0, 1, 0, 1);
    nv_vk_sync_color_reason("presentation");
    CHECK(owned_color[1][0] == 0 && owned_depth[0] == 1.f);
    CHECK(deferred_depth_copies > 0);
    // A real software consumer materializes the latest version, including
    // cached depth when another unrelated target is currently active.
    owned.color = (uint8_t *)owned_color[2]; owned.depth = other_depth;
    CHECK(nv_vk_begin(&owned)); quad(.6f, 0, 0, 1, 1);
    nv_vk_sync();
    CHECK(fabs(owned_depth[0] - .2f) < .00001f);
    CHECK(fabs(other_depth[0] - .6f) < .00001f);
    // Reassign a host slot while its old GPU depth is pending. Publication
    // must never write the previous allocation into the replacement buffer.
    owned.color = (uint8_t *)owned_color[0]; owned.depth = owned_depth;
    CHECK(nv_vk_begin(&owned)); quad(.1f, 1, 0, 0, 1);
    CHECK(recording);
    nv_vk_depth_slot_reused(owned_depth);
    std::fill(owned_depth, owned_depth + 64*64, 1.f);
    owned.depth_generation = 2;
    CHECK(nv_vk_begin(&owned)); quad(.8f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(owned_color[0][0] == 0xFF00FF00 && fabs(owned_depth[0] - .8f) < .00001f);
    CHECK(nv_vk_begin(&owned)); quad(.1f, 1, 0, 0, 1);
    std::fill(owned_depth, owned_depth + 64*64, 1.f);
    nv_vk_sync_color_reason("presentation");
    // A color-only publication must not relabel CPU-edited depth as matching
    // an older GPU image. The next batch must consume the new CPU values.
    CHECK(nv_vk_begin(&owned)); quad(.5f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(owned_color[0][0] == 0xFF00FF00 && fabs(owned_depth[0] - .5f) < .00001f);
    // A full depth clear supersedes every cached version, even if its CPU
    // value equals an earlier snapshot. The old .1 GPU image cannot return.
    CHECK(nv_vk_begin(&owned)); quad(.1f, 1, 0, 0, 1);
    NvGpuClear owned_clear = {};
    owned_clear.depth = owned_depth; owned_clear.depth_value = .8f;
    CHECK(nv_vk_clear_attachments(&owned_clear));
    std::fill(owned_depth, owned_depth + 64*64, .8f);
    owned.color = (uint8_t *)owned_color[1];
    CHECK(nv_vk_begin(&owned)); quad(.5f, 0, 0, 1, 1); nv_vk_sync();
    CHECK(owned_color[1][0] == 0xFF0000FF && fabs(owned_depth[0] - .5f) < .00001f);
    // Different native dimensions require explicit materialization before
    // interpreting the allocation with a different depth layout.
    CHECK(nv_vk_begin(&owned)); quad(.2f, 1, 0, 0, 1);
    nv_vk_sync_color_reason("presentation");
    owned.color = (uint8_t *)owned_color[2];
    owned.width = owned.height = owned.clip_w = owned.clip_h = 32;
    CHECK(nv_vk_begin(&owned)); quad(.7f, 0, 1, 0, 1); nv_vk_sync();
    CHECK(fabs(owned_depth[0] - .2f) < .00001f);
    // More outstanding allocations than the bounded pool must materialize
    // depth before eviction, without destroying in-flight image resources.
    static uint32_t eviction_color[10][64*64];
    static float eviction_depth[10][64*64];
    owned.width = owned.height = owned.clip_w = owned.clip_h = 64;
    for (unsigned allocation = 0; allocation < 10; allocation++)
    {
        std::fill(eviction_depth[allocation], eviction_depth[allocation] + 64*64, 1.f);
        owned.color = (uint8_t *)eviction_color[allocation]; owned.depth = eviction_depth[allocation];
        owned.depth_generation = allocation + 10;
        CHECK(nv_vk_begin(&owned)); quad(.3f, 1, 0, 0, 1);
        nv_vk_sync_color_reason("presentation");
    }
    nv_vk_sync();
    for (unsigned allocation = 0; allocation < 10; allocation++)
    {
        CHECK(fabs(eviction_depth[allocation][0] - .3f) < .00001f);
    }
    nv_vk_shutdown();
    puts("Passed: Vulkan color/depth, indexed geometry, blend, alpha, textures, filtering, multistage combiners, scissor, publication, CPU edits, swizzled surfaces, target switches, fallback and lifecycle.");
}
'''
(out / "vulkan_raster.cpp").write_text(code)
project = (out / "apu_irq_harness.vcxproj").read_text()
project = project.replace("apu_irq_harness.c", "vulkan_raster.cpp").replace("obj\\apu_irq\\", "obj\\vulkan_raster\\")
(out / "vulkan_raster.vcxproj").write_text(project)
subprocess.run([msbuild, str(out / "vulkan_raster.vcxproj"), "/p:Configuration=Release", "/p:Platform=x64", "/verbosity:minimal"], check=True)
subprocess.run([str(out / "bin/vulkan_raster.exe")], check=True)
