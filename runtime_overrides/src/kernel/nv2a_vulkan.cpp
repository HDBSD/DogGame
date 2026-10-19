/* Experimental Vulkan raster backend. This consumes the
 * maintained packet executor's NvGpuDraw contract, not recovered game source.
 * Color publication and explicit CPU depth consumers have separate contracts.
 * Vulkan objects and mapped uploads are owned solely by the title thread. */
#include "nv2a_vulkan.h"
extern "C"
{
#include "../../include/xbox/display_options.h"
}
#define VK_NO_PROTOTYPES
#include "nv2a_vulkan_shaders.h"
#include "vulkan_headers/vulkan_core.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

namespace
{
/* Load the system driver's loader only when explicitly selected. Normal D3D11
 * startup has no Vulkan DLL import or Vulkan SDK installation requirement. */
// clang-format off
#define VULKAN_FUNCTIONS(X) \
    X(AllocateCommandBuffers) \
    X(BindBufferMemory) \
    X(CreateDescriptorPool) \
    X(UpdateDescriptorSets) \
    X(InvalidateMappedMemoryRanges) \
    X(CmdClearColorImage) \
    X(CmdClearDepthStencilImage) \
    X(CreateInstance) \
    X(DestroyInstance) \
    X(EnumeratePhysicalDevices) \
    X(GetPhysicalDeviceProperties) \
    X(GetPhysicalDeviceFeatures) \
    X(GetPhysicalDeviceQueueFamilyProperties) \
    X(GetPhysicalDeviceMemoryProperties) \
    X(GetPhysicalDeviceFormatProperties) \
    X(CreateDevice) \
    X(DestroyDevice) \
    X(GetDeviceQueue) \
    X(CreateCommandPool) \
    X(DestroyCommandPool) \
    X(ResetCommandPool) \
    X(BeginCommandBuffer) \
    X(EndCommandBuffer) \
    X(CreateFence) \
    X(DestroyFence) \
    X(ResetFences) \
    X(WaitForFences) \
    X(QueueSubmit) \
    X(DeviceWaitIdle) \
    X(CreateBuffer) \
    X(DestroyBuffer) \
    X(GetBufferMemoryRequirements) \
    X(AllocateMemory) \
    X(FreeMemory) \
    X(MapMemory) \
    X(UnmapMemory) \
    X(CreateImage) \
    X(DestroyImage) \
    X(GetImageMemoryRequirements) \
    X(BindImageMemory) \
    X(CreateImageView) \
    X(DestroyImageView) \
    X(CreateSampler) \
    X(DestroySampler) \
    X(CreateShaderModule) \
    X(DestroyShaderModule) \
    X(CreateRenderPass) \
    X(DestroyRenderPass) \
    X(CreateFramebuffer) \
    X(DestroyFramebuffer) \
    X(CreateDescriptorSetLayout) \
    X(DestroyDescriptorSetLayout) \
    X(CreatePipelineLayout) \
    X(DestroyPipelineLayout) \
    X(DestroyDescriptorPool) \
    X(ResetDescriptorPool) \
    X(AllocateDescriptorSets) \
    X(CreateGraphicsPipelines) \
    X(DestroyPipeline) \
    X(CmdPipelineBarrier) \
    X(CmdCopyBufferToImage) \
    X(CmdCopyImageToBuffer) \
    X(CmdCopyImage) \
    X(CmdBlitImage) \
    X(CmdBeginRenderPass) \
    X(CmdEndRenderPass) \
    X(CmdBindPipeline) \
    X(CmdSetViewport) \
    X(CmdSetScissor) \
    X(CmdBindVertexBuffers) \
    X(CmdBindDescriptorSets) \
    X(CmdDraw)
// clang-format on
#define DECLARE(name) PFN_vk##name vk##name;
VULKAN_FUNCTIONS(DECLARE)
#undef DECLARE
HMODULE loader;
PFN_vkGetInstanceProcAddr get_instance_proc;
PFN_vkGetDeviceProcAddr get_device_proc;
VkInstance instance;
VkPhysicalDevice physical;
VkDevice device;
VkQueue queue;
VkPhysicalDeviceMemoryProperties memory_properties;
VkPhysicalDeviceProperties properties;
VkCommandPool command_pool;
VkCommandBuffer command;
std::vector<VkCommandBuffer> command_segments;
unsigned command_segment, segment_draws;
const unsigned draws_per_submission = 32;
uint64_t early_submissions;
VkFence fence;
VkRenderPass render_pass;
VkFramebuffer framebuffer;
VkDescriptorSetLayout descriptor_layout;
VkDescriptorPool descriptor_pool;
VkPipelineLayout pipeline_layout;
VkShaderModule vertex_shader, pixel_shader;
bool recording, loaded_target, color_dirty, depth_dirty;
bool render_pass_open;
uint64_t render_passes;
NvGpuDraw target_state, draw_state;
uint64_t batches, triangles, readbacks, fallbacks;
uint64_t prepare_ticks, record_ticks, submit_ticks, wait_ticks, publish_ticks;
uint64_t gpu_color_clears, gpu_depth_clears;
bool mirrored_color_clear, mirrored_depth_clear;
uint32_t mirrored_color_value;
float mirrored_depth_value;

struct SyncAudit
{
    std::string reason;
    uint64_t calls = 0, wait = 0, color_bytes = 0, depth_bytes = 0;
};
std::vector<SyncAudit> sync_audit;

struct CpuTimer
{
    uint64_t &total;
    LARGE_INTEGER start;

    explicit CpuTimer(uint64_t &destination) : total(destination)
    {
        QueryPerformanceCounter(&start);
    }

    ~CpuTimer()
    {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        total += (uint64_t)(end.QuadPart - start.QuadPart);
    }
};

struct Image
{
    VkImage handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};
Image color_image, depth_image;
Image resolve_color, resolve_depth;
Image depth_history_image;
unsigned depth_history_width, depth_history_height;
uint64_t gpu_depth_history_bytes;
unsigned resolve_width, resolve_height;
unsigned color_read_width, color_read_height, depth_read_width, depth_read_height;
bool native_resolve_supported;
uint64_t native_resolves;
uint64_t native_upload_expansions;
std::vector<double> present_intervals;
unsigned present_interval_cursor;
LONGLONG previous_present_tick;

/* Title-thread publication cadence, not monitor scanout timing. Retain a
 * bounded recent window; a report consumes it without changing rendering. */
struct PresentInterval
{
    ~PresentInterval()
    {
        LARGE_INTEGER now, frequency;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&frequency);
        if (previous_present_tick)
        {
            double ms = (now.QuadPart - previous_present_tick) * 1000.0 / frequency.QuadPart;
            if (present_intervals.size() < 2048)
            {
                present_intervals.push_back(ms);
            }
            else
            {
                present_intervals[present_interval_cursor++ % 2048] = ms;
            }
        }
        previous_present_tick = now.QuadPart;
    }
};
unsigned target_width, target_height;
std::vector<Image> draw_textures;
std::vector<Image> retired_textures;
struct CachedTexture
{
    NvGpuTexture state;
    Image image;
};
std::vector<CachedTexture> cached_textures;
VkDeviceSize cached_texture_bytes;
unsigned draw_sets;
uint64_t texture_cache_hits, texture_uploads, texture_replacements;
std::vector<NvGpuVertex> vertices;
std::vector<unsigned> indices;

/* A fence protects uploads, uniform snapshots and vertices until sync. CPU
 * reads use a separate cached buffer: write-combined upload memory can make
 * per-pixel readback orders of magnitude slower even after the GPU is idle. */
/* A fallback/full-history 4K color/depth upload needs ~63 MiB. Leave room for
 * geometry, constants and texture uploads instead of flushing every draw. */
const VkDeviceSize arena_bytes = 128ull * 1024 * 1024;
/* Largest selectable physical target. Guest targets remain limited separately.
 * Color and depth occupy disjoint, fixed-size ranges in the readback buffer. */
const VkDeviceSize attachment_bytes = 3840ull * 2160 * 4;
std::vector<uint32_t> present_pixels, present_native;
const uint8_t *present_guest;
unsigned present_pitch, present_width, present_height;
std::vector<uint32_t> target_native_color;
std::vector<float> target_native_depth;
struct ResidentTarget
{
    NvGpuDraw state;
    Image color, depth;
    VkFramebuffer framebuffer;
    unsigned width, height;
    std::vector<uint32_t> native_color;
    std::vector<float> native_depth;
    bool pending_depth;
    uint64_t depth_revision;
    uint64_t color_revision;
};
std::vector<ResidentTarget> resident_targets;
uint64_t depth_revision, depth_revision_clock;
uint64_t color_revision, color_revision_clock;
uint64_t deferred_depth_boundaries, deferred_depth_copies;
void sync_active_target(const char *reason, bool publish_depth);
void publish_resident_depth(ResidentTarget &entry);

bool same_depth_allocation(const NvGpuDraw &a, const NvGpuDraw &b)
{
    return a.depth == b.depth && a.depth_generation == b.depth_generation;
}

bool same_depth_layout(const NvGpuDraw &a, const NvGpuDraw &b)
{
    return same_depth_allocation(a, b) && a.width == b.width && a.height == b.height &&
        a.depth_stride == b.depth_stride && a.zscale == b.zscale;
}

bool depth_cpu_matches(const NvGpuDraw &state, const std::vector<float> &snapshot)
{
    if (snapshot.size() != (size_t)state.width * state.height)
    {
        return false;
    }
    for (unsigned y = 0; state.depth && y < state.height; y++)
    {
        if (std::memcmp(state.depth + (size_t)y * state.depth_stride,
                        snapshot.data() + (size_t)y * state.width, state.width * sizeof(float)))
        {
            return false;
        }
    }
    return true;
}
uint64_t resident_hits, color_upload_bytes, depth_upload_bytes;
uint64_t physical_color_restore_bytes, physical_depth_restore_bytes;

bool color_cpu_matches(const NvGpuDraw &state, const std::vector<uint32_t> &snapshot)
{
    if (snapshot.size() != (size_t)state.width * state.height)
    {
        return false;
    }
    for (unsigned y = 0; y < state.height; y++)
    {
        if (!state.surface_offset)
        {
            if (std::memcmp(state.color + (size_t)y * state.pitch,
                            snapshot.data() + (size_t)y * state.width, state.width * 4))
            {
                return false;
            }
        }
        else
        {
            for (unsigned x = 0; x < state.width; x++)
            {
                uint32_t value;
                std::memcpy(&value, state.color + state.surface_offset(state.surface_format, x, y), 4);
                if (value != snapshot[(size_t)y * state.width + x])
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool native_color_matches(const NvGpuDraw &state)
{
    for (const ResidentTarget &entry : resident_targets)
    {
        if (entry.state.color == state.color && entry.color_revision > color_revision)
        {
            return false;
        }
    }
    return color_cpu_matches(state, target_native_color);
}

bool native_depth_matches(const NvGpuDraw &state)
{
    if (!same_depth_layout(state, target_state))
    {
        return false;
    }
    for (const ResidentTarget &entry : resident_targets)
    {
        if (same_depth_allocation(state, entry.state) && entry.depth_revision > depth_revision)
        {
            return false;
        }
    }
    return depth_cpu_matches(state, target_native_depth);
}

void snapshot_native_target(const NvGpuDraw &state, bool snapshot_color = true, bool snapshot_depth = true)
{
    if (snapshot_color)
    {
        target_native_color.resize((size_t)state.width * state.height);
    }
    if (snapshot_depth)
    {
        target_native_depth.resize((size_t)state.width * state.height);
    }
    for (unsigned y = 0; y < state.height; y++)
    {
        if (snapshot_color && !state.surface_offset)
        {
            std::memcpy(target_native_color.data() + (size_t)y * state.width,
                        state.color + (size_t)y * state.pitch, state.width * 4);
        }
        else if (snapshot_color)
        {
            for (unsigned x = 0; x < state.width; x++)
            {
                std::memcpy(&target_native_color[(size_t)y * state.width + x],
                            state.color + state.surface_offset(state.surface_format, x, y), 4);
            }
        }
        if (snapshot_depth && state.depth)
        {
            std::memcpy(target_native_depth.data() + (size_t)y * state.width,
                        state.depth + (size_t)y * state.depth_stride, state.width * sizeof(float));
        }
    }
}

bool same_color_target(const NvGpuDraw &a, const NvGpuDraw &b)
{
    return a.color == b.color && a.width == b.width && a.height == b.height && a.pitch == b.pitch &&
           a.surface_format == b.surface_format && a.surface_offset == b.surface_offset;
}
std::vector<float> preserved_native_depth;
const float *preserved_depth_guest;
unsigned preserved_depth_stride, preserved_depth_width, preserved_depth_height;
unsigned preserved_depth_guest_width, preserved_depth_guest_height;
float preserved_depth_zscale;

bool depth_snapshot_matches(const NvGpuDraw &state, unsigned width, unsigned height)
{
    if (!depth_history_image.handle || !state.depth || state.depth != preserved_depth_guest || state.depth_stride != preserved_depth_stride ||
        state.zscale != preserved_depth_zscale || width != preserved_depth_width || height != preserved_depth_height ||
        width != depth_history_width || height != depth_history_height ||
        state.width != preserved_depth_guest_width || state.height != preserved_depth_guest_height)
    {
        return false;
    }
    for (unsigned y = 0; y < state.height; y++)
    {
        if (std::memcmp(state.depth + (size_t)y * state.depth_stride,
                        preserved_native_depth.data() + (size_t)y * state.width, state.width * sizeof(float)))
        {
            return false;
        }
    }
    return true;
}

void physical_target_size(const NvGpuDraw &state, unsigned &width, unsigned &height)
{
    width = state.width;
    height = state.height;
    if (xbox_DisplayInternalScaling() && state.width == 640 && state.height <= 480 && !state.surface_offset)
    {
        unsigned selected_width, selected_height;
        xbox_DisplayResolutionSize(xbox_DisplayResolutionSelected(), &selected_width, &selected_height);
        if (selected_width >= 640 && selected_width <= 3840 && selected_height >= 480 && selected_height <= 2160)
        {
            width = selected_width;
            height = std::max(1u, (unsigned)((uint64_t)state.height * selected_height / 480));
        }
    }
}

bool present_matches(const uint8_t *guest, unsigned pitch)
{
    if (guest != present_guest || pitch != present_pitch || present_native.size() != 640 * 480)
    {
        return false;
    }
    for (unsigned y = 0; y < 480; y++)
    {
        if (std::memcmp(guest + (size_t)y * pitch, present_native.data() + y * 640, 640 * 4))
        {
            return false;
        }
    }
    return true;
}
VkBuffer arena;
VkDeviceMemory arena_memory;
uint8_t *mapped;
VkDeviceSize cursor;
VkBuffer readback_buffer;
VkDeviceMemory readback_memory;
uint8_t *readback_mapped;
VkMemoryPropertyFlags readback_memory_flags;

struct PipelineKey
{
    unsigned blend, src, dst, equation, mask, depth_test, depth_write, depth_func, mode;
};
struct Pipeline
{
    PipelineKey key;
    VkPipeline handle;
};
std::vector<Pipeline> pipelines;
VkPipeline draw_pipeline;
VkDescriptorSet draw_descriptor;

void check(VkResult result, const char *operation)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string(operation) + " failed (VkResult " + std::to_string(result) +
                                 ")");
    }
}

void destroy_image(Image &image)
{
    if (image.sampler)
    {
        vkDestroySampler(device, image.sampler, nullptr);
    }
    if (image.view)
    {
        vkDestroyImageView(device, image.view, nullptr);
    }
    if (image.handle)
    {
        vkDestroyImage(device, image.handle, nullptr);
    }
    if (image.memory)
    {
        vkFreeMemory(device, image.memory, nullptr);
    }
    image = {};
}

void clear_texture_cache()
{
    /* Call only after the queue's references have completed. */
    for (CachedTexture &entry : cached_textures)
    {
        destroy_image(entry.image);
    }
    cached_textures.clear();
    cached_texture_bytes = 0;
}

size_t find_texture(const NvGpuTexture &texture)
{
    for (size_t i = 0; i < cached_textures.size(); i++)
    {
        const NvGpuTexture &old = cached_textures[i].state;
        if ((!texture.valid && !old.valid) ||
            (texture.valid && old.valid && texture.key && old.key == texture.key &&
             old.hash == texture.hash && old.width == texture.width && old.height == texture.height &&
             old.addr_u == texture.addr_u && old.addr_v == texture.addr_v && old.linear == texture.linear))
        {
            return i;
        }
    }
    return SIZE_MAX;
}

void cache_texture(const NvGpuTexture &texture, const Image &image)
{
    for (CachedTexture &entry : cached_textures)
    {
        const NvGpuTexture &old = entry.state;
        bool same_identity =
            (!texture.valid && !old.valid) ||
            (texture.valid && old.valid && old.key == texture.key && old.width == texture.width &&
             old.height == texture.height && old.addr_u == texture.addr_u && old.addr_v == texture.addr_v &&
             old.linear == texture.linear);
        if (same_identity)
        {
            /* Recorded descriptors can still refer to the old hash version.
             * Replace its lookup entry now, but release its image only after
             * the submission fence. Static entries survive animated updates. */
            retired_textures.push_back(entry.image);
            entry.state = texture;
            entry.image = image;
            texture_replacements++;
            return;
        }
    }
    cached_textures.push_back({texture, image});
    cached_texture_bytes += (VkDeviceSize)texture.width * texture.height * 4;
}

unsigned memory_type(unsigned allowed, VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred = 0)
{
    unsigned selected = UINT32_MAX;
    int best_score = -1;
    for (unsigned i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        VkMemoryPropertyFlags flags = memory_properties.memoryTypes[i].propertyFlags;
        if ((allowed & (1u << i)) && (flags & required) == required)
        {
            int score = (flags & preferred) == preferred ? 2 : 0;
            if (preferred && (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                score++;
            }
            if (score > best_score)
            {
                selected = i;
                best_score = score;
            }
        }
    }
    if (selected != UINT32_MAX)
    {
        return selected;
    }
    throw std::runtime_error("Required Vulkan memory type is unavailable");
}

void make_image(Image &image, unsigned width, unsigned height, VkFormat format, VkImageUsageFlags usage,
                VkImageAspectFlags aspect)
{
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {width, height, 1};
    info.mipLevels = info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    try
    {
        check(vkCreateImage(device, &info, nullptr, &image.handle), "create image");
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device, image.handle, &requirements);
        VkMemoryAllocateInfo allocation = {};
        allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex =
            memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device, &allocation, nullptr, &image.memory), "allocate image memory");
        check(vkBindImageMemory(device, image.handle, image.memory, 0), "bind image memory");
        VkImageViewCreateInfo view = {};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = image.handle;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange = {aspect, 0, 1, 0, 1};
        check(vkCreateImageView(device, &view, nullptr, &image.view), "create image view");
    }
    catch (...)
    {
        destroy_image(image);
        throw;
    }
}

void begin_commands()
{
    if (recording)
    {
        return;
    }
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(command, &info), "begin commands");
    recording = true;
}

void end_render_pass()
{
    if (render_pass_open)
    {
        vkCmdEndRenderPass(command);
        render_pass_open = false;
    }
}

void begin_render_pass()
{
    if (render_pass_open)
    {
        return;
    }
    VkRenderPassBeginInfo pass = {};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = render_pass;
    pass.framebuffer = framebuffer;
    pass.renderArea.extent = {target_width, target_height};
    vkCmdBeginRenderPass(command, &pass, VK_SUBPASS_CONTENTS_INLINE);
    render_pass_open = true;
    render_passes++;
}

void memory_barrier();

void submit_completed_segment()
{
    /* Start GPU work while the title thread prepares subsequent draws. Upload
     * offsets and descriptors remain immutable; no pool or image is recycled
     * here. The final same-queue fence covers every earlier submission too:
     * Vulkan specification, Synchronization / Fence Signaling Operations. */
    end_render_pass();
    memory_barrier();
    check(vkEndCommandBuffer(command), "end early command segment");
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    {
        CpuTimer timer(submit_ticks);
        check(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE), "submit early command segment");
    }
    early_submissions++;
    recording = false;
    segment_draws = 0;
    command_segment++;
    if (command_segment == command_segments.size())
    {
        VkCommandBufferAllocateInfo allocation = {};
        allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocation.commandPool = command_pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        VkCommandBuffer next;
        check(vkAllocateCommandBuffers(device, &allocation, &next), "allocate early command segment");
        command_segments.push_back(next);
    }
    command = command_segments[command_segment];
    begin_commands();
}

void memory_barrier()
{
    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                         1, &barrier, 0, nullptr, 0, nullptr);
}

void initialize_layout(const Image &image, VkImageAspectFlags aspect)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.handle;
    barrier.subresourceRange = {aspect, 0, 1, 0, 1};
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
}

VkDeviceSize allocate_upload(VkDeviceSize bytes, VkDeviceSize alignment = 16)
{
    VkDeviceSize offset = (cursor + alignment - 1) / alignment * alignment;
    if (offset + bytes > arena_bytes)
    {
        throw std::runtime_error("Vulkan upload arena exhausted by a single batch");
    }
    cursor = offset + bytes;
    return offset;
}

void copy_image(const Image &image, VkImageAspectFlags aspect, unsigned width, unsigned height,
                VkDeviceSize offset, bool upload)
{
    VkBufferImageCopy copy = {};
    copy.bufferOffset = offset;
    copy.imageSubresource = {aspect, 0, 0, 1};
    copy.imageExtent = {width, height, 1};
    if (upload)
    {
        vkCmdCopyBufferToImage(command, arena, image.handle, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
    }
    else
    {
        vkCmdCopyImageToBuffer(command, image.handle, VK_IMAGE_LAYOUT_GENERAL, readback_buffer, 1, &copy);
    }
}

void prepare_native_resolve()
{
    if (resolve_width != target_state.width || resolve_height != target_state.height)
    {
        destroy_image(resolve_color);
        destroy_image(resolve_depth);
        make_image(resolve_color, target_state.width, target_state.height, VK_FORMAT_B8G8R8A8_UNORM,
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        make_image(resolve_depth, target_state.width, target_state.height, VK_FORMAT_D32_SFLOAT,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        initialize_layout(resolve_color, VK_IMAGE_ASPECT_COLOR_BIT);
        initialize_layout(resolve_depth, VK_IMAGE_ASPECT_DEPTH_BIT);
        resolve_width = target_state.width;
        resolve_height = target_state.height;
    }
}

void resolve_to_native(const Image &source, const Image &destination, VkImageAspectFlags aspect)
{
    VkImageBlit blit = {};
    blit.srcSubresource = {aspect, 0, 0, 1};
    blit.dstSubresource = {aspect, 0, 0, 1};
    blit.srcOffsets[1] = {(int32_t)target_width, (int32_t)target_height, 1};
    blit.dstOffsets[1] = {(int32_t)target_state.width, (int32_t)target_state.height, 1};
    vkCmdBlitImage(command, source.handle, VK_IMAGE_LAYOUT_GENERAL, destination.handle,
                   VK_IMAGE_LAYOUT_GENERAL, 1, &blit, VK_FILTER_NEAREST);
    native_resolves++;
}

void expand_to_physical(const Image &source, const Image &destination, VkImageAspectFlags aspect)
{
    /* Reconstruct a changed native guest surface using nearest sampling.
     * Blit support is checked for both formats before selecting this path.
     * Physical history is uploaded verbatim instead when it is still valid. */
    VkImageBlit blit = {};
    blit.srcSubresource = {aspect, 0, 0, 1};
    blit.dstSubresource = {aspect, 0, 0, 1};
    blit.srcOffsets[1] = {(int32_t)target_state.width, (int32_t)target_state.height, 1};
    blit.dstOffsets[1] = {(int32_t)target_width, (int32_t)target_height, 1};
    vkCmdBlitImage(command, source.handle, VK_IMAGE_LAYOUT_GENERAL, destination.handle,
                   VK_IMAGE_LAYOUT_GENERAL, 1, &blit, VK_FILTER_NEAREST);
    native_upload_expansions++;
}

void copy_depth_history(const Image &source, const Image &destination, unsigned width, unsigned height)
{
    VkImageCopy copy = {};
    copy.srcSubresource = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1};
    copy.dstSubresource = copy.srcSubresource;
    copy.extent = {width, height, 1};
    vkCmdCopyImage(command, source.handle, VK_IMAGE_LAYOUT_GENERAL, destination.handle,
                   VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
}

void retain_depth_history()
{
    if (depth_history_width != target_width || depth_history_height != target_height)
    {
        /* Target changes synchronize first, so a differently sized history
         * image cannot still be referenced by an earlier submission. */
        destroy_image(depth_history_image);
        make_image(depth_history_image, target_width, target_height, VK_FORMAT_D32_SFLOAT,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        initialize_layout(depth_history_image, VK_IMAGE_ASPECT_DEPTH_BIT);
        depth_history_width = target_width;
        depth_history_height = target_height;
    }
    copy_depth_history(depth_image, depth_history_image, target_width, target_height);
}

void copy_depth_sample_rows()
{
    /* Copy the exact physical rows used by native pixel-center publication.
     * The driver test's depth blit picked the other texel at a tie; retaining the
     * physical columns lets the CPU preserve its existing exact X samples.
     * One command records the selected rows, reducing the 4K payload by 4.5x. */
    static std::vector<VkBufferImageCopy> rows;
    rows.resize(target_state.height);
    for (unsigned y = 0; y < target_state.height; y++)
    {
        VkBufferImageCopy &row = rows[y];
        row = {};
        row.bufferOffset = attachment_bytes + (VkDeviceSize)y * target_width * 4;
        row.imageSubresource = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1};
        row.imageOffset.y = (int32_t)std::min(target_height - 1,
            (unsigned)(((uint64_t)y * 2 + 1) * target_height / (target_state.height * 2)));
        row.imageExtent = {target_width, 1, 1};
    }
    vkCmdCopyImageToBuffer(command, depth_image.handle, VK_IMAGE_LAYOUT_GENERAL,
                           readback_buffer, (uint32_t)rows.size(), rows.data());
}

void ensure_target(const NvGpuDraw &state)
{
    unsigned width, height;
    physical_target_size(state, width, height);
    bool same = loaded_target && target_state.color == state.color && same_depth_layout(target_state, state) &&
                target_state.width == state.width && target_state.height == state.height &&
                target_state.pitch == state.pitch && target_state.depth_stride == state.depth_stride &&
                target_state.zscale == state.zscale && target_state.surface_format == state.surface_format &&
                target_state.surface_offset == state.surface_offset && target_width == width && target_height == height;
    if (same && (recording || (native_color_matches(state) && native_depth_matches(state))))
    {
        begin_commands();
        return;
    }
    nv_vk_sync_color_reason("render target switch");
    if (color_image.handle && (!same_color_target(target_state, state) || !same_depth_layout(target_state, state) ||
                              target_width != width || target_height != height))
    {
        /* Color is fence-complete and published. Depth may remain authoritative
         * on the GPU; its CPU snapshot detects an intervening software write. */
        ResidentTarget saved = {target_state, color_image, depth_image, framebuffer, target_width, target_height,
                                std::move(target_native_color), std::move(target_native_depth), depth_dirty,
                                depth_revision, color_revision};
        resident_targets.push_back(std::move(saved));
        color_image = {};
        depth_image = {};
        framebuffer = VK_NULL_HANDLE;
        loaded_target = false;
        depth_dirty = false;
        depth_revision = 0;
        color_revision = 0;
        auto found = std::find_if(resident_targets.begin(), resident_targets.end(),
            [&](const ResidentTarget &entry)
            {
                return entry.width == width && entry.height == height && same_color_target(entry.state, state) &&
                    same_depth_layout(entry.state, state);
            });
        if (found != resident_targets.end())
        {
            color_image = found->color;
            depth_image = found->depth;
            framebuffer = found->framebuffer;
            target_state = found->state;
            target_width = found->width;
            target_height = found->height;
            target_native_color = std::move(found->native_color);
            target_native_depth = std::move(found->native_depth);
            resident_targets.erase(found);
            loaded_target = true;
            depth_dirty = found->pending_depth;
            depth_revision = found->depth_revision;
            color_revision = found->color_revision;
            resident_hits++;
        }
        if (resident_targets.size() > 6)
        {
            ResidentTarget &old = resident_targets.front();
            publish_resident_depth(old);
            vkDestroyFramebuffer(device, old.framebuffer, nullptr);
            destroy_image(old.color);
            destroy_image(old.depth);
            resident_targets.erase(resident_targets.begin());
        }
    }
    bool recreate = !color_image.handle || !depth_image.handle || !framebuffer ||
                    target_width != width || target_height != height;
    bool upload_color = recreate || !loaded_target || !native_color_matches(state);
    bool upload_depth = recreate || !loaded_target || !native_depth_matches(state);
    uint64_t newest_color = target_state.color == state.color ? color_revision : 0;
    uint64_t newest_depth = same_depth_allocation(target_state, state) ? depth_revision : 0;
    for (const ResidentTarget &entry : resident_targets)
    {
        if (entry.state.color == state.color)
        {
            newest_color = std::max(newest_color, entry.color_revision);
        }
        if (same_depth_allocation(entry.state, state))
        {
            newest_depth = std::max(newest_depth, entry.depth_revision);
        }
    }
    Image color_source = {};
    uint64_t source_color_revision = 0;
    if (upload_color)
    {
        for (const ResidentTarget &entry : resident_targets)
        {
            if (same_color_target(state, entry.state) && entry.width == width && entry.height == height &&
                entry.color_revision == newest_color && entry.color_revision > source_color_revision &&
                color_cpu_matches(state, entry.native_color))
            {
                color_source = entry.color;
                source_color_revision = entry.color_revision;
            }
        }
    }
    Image depth_source = {};
    uint64_t source_revision = 0;
    if (upload_depth && state.depth)
    {
        for (ResidentTarget &entry : resident_targets)
        {
            if (entry.pending_depth && same_depth_allocation(state, entry.state) && entry.depth_revision == newest_depth)
            {
                if (same_depth_layout(state, entry.state) && entry.width == width && entry.height == height &&
                    depth_cpu_matches(state, entry.native_depth))
                {
                    if (entry.depth_revision > source_revision)
                    {
                        depth_source = entry.depth;
                        source_revision = entry.depth_revision;
                    }
                }
                else
                {
                    /* A different depth interpretation needs native values.
                     * CPU edits supersede the old GPU version instead. */
                    if (depth_cpu_matches(entry.state, entry.native_depth))
                    {
                        publish_resident_depth(entry);
                    }
                    else
                    {
                        entry.pending_depth = false;
                        entry.native_depth.clear();
                    }
                }
            }
        }
    }
    if (recreate)
    {
        if (framebuffer)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        framebuffer = VK_NULL_HANDLE;
        destroy_image(color_image);
        destroy_image(depth_image);
        make_image(color_image, width, height, VK_FORMAT_B8G8R8A8_UNORM,
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        make_image(depth_image, width, height, VK_FORMAT_D32_SFLOAT,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        VkImageView attachments[] = {color_image.view, depth_image.view};
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = render_pass;
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.width = width;
        info.height = height;
        info.layers = 1;
        check(vkCreateFramebuffer(device, &info, nullptr, &framebuffer), "create framebuffer");
        target_width = width;
        target_height = height;
        static unsigned reported_width, reported_height;
        if (state.width == 640 && state.height == 480 && (reported_width != width || reported_height != height))
        {
            std::fprintf(stderr, "[GPU-VULKAN-TARGET] guest=%ux%u raster=%ux%u\n", state.width, state.height, width, height);
            reported_width = width;
            reported_height = height;
        }
    }
    begin_commands();
    if (recreate)
    {
        initialize_layout(color_image, VK_IMAGE_ASPECT_COLOR_BIT);
        initialize_layout(depth_image, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
    bool restore_present = width == present_width && height == present_height &&
                           state.height == 480 && present_matches(state.color, state.pitch);
    bool restore_depth = !state.depth_generation && depth_snapshot_matches(state, width, height);
    bool scaled = width != state.width || height != state.height;
    bool expand_color = scaled && native_resolve_supported && !restore_present;
    bool expand_depth = scaled && native_resolve_supported && !restore_depth;
    unsigned color_width = expand_color ? state.width : width;
    unsigned color_height = expand_color ? state.height : height;
    unsigned depth_width = expand_depth ? state.width : width;
    unsigned depth_height = expand_depth ? state.height : height;
    VkDeviceSize color_offset = upload_color && !color_source.handle ?
        allocate_upload((VkDeviceSize)color_width * color_height * 4) : 0;
    VkDeviceSize depth_offset = upload_depth && !restore_depth && !depth_source.handle ?
        allocate_upload((VkDeviceSize)depth_width * depth_height * 4) : 0;
    if (upload_color && restore_present && !color_source.handle)
    {
        std::memcpy(mapped + color_offset, present_pixels.data(), (size_t)width * height * 4);
        physical_color_restore_bytes += (uint64_t)width * height * 4;
    }
    else if (upload_color && !color_source.handle)
    {
        for (unsigned y = 0; y < color_height; y++)
        {
            unsigned guest_y = color_height == state.height ? y : (unsigned)((uint64_t)y * state.height / color_height);
            uint8_t *row = mapped + color_offset + (size_t)y * color_width * 4;
            if (color_width == state.width && !state.surface_offset)
            {
                std::memcpy(row, state.color + (size_t)guest_y * state.pitch, color_width * 4);
            }
            else
            {
                for (unsigned x = 0; x < color_width; x++)
                {
                    unsigned guest_x = color_width == state.width ? x : (unsigned)((uint64_t)x * state.width / color_width);
                    size_t offset = state.surface_offset ? state.surface_offset(state.surface_format, guest_x, guest_y)
                                                       : (size_t)guest_y * state.pitch + guest_x * 4;
                    std::memcpy(row + x * 4, state.color + offset, 4);
                }
            }
        }
    }
    if (upload_depth && !restore_depth && !depth_source.handle)
    {
        for (unsigned y = 0; y < depth_height; y++)
        {
            unsigned guest_y = depth_height == state.height ? y : (unsigned)((uint64_t)y * state.height / depth_height);
            for (unsigned x = 0; x < depth_width; x++)
            {
                unsigned guest_x = depth_width == state.width ? x : (unsigned)((uint64_t)x * state.width / depth_width);
                float depth = state.depth ? state.depth[(size_t)guest_y * state.depth_stride + guest_x] / state.zscale : 1.f;
                depth = std::max(0.f, std::min(1.f, depth));
                std::memcpy(mapped + depth_offset + ((size_t)y * depth_width + x) * 4, &depth, 4);
            }
        }
    }
    target_state = state;
    if ((upload_color && expand_color) || (upload_depth && expand_depth))
    {
        prepare_native_resolve();
    }
    memory_barrier();
    if (upload_color && color_source.handle)
    {
        VkImageCopy copy = {};
        copy.srcSubresource = copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.extent = {width, height, 1};
        vkCmdCopyImage(command, color_source.handle, VK_IMAGE_LAYOUT_GENERAL, color_image.handle,
                       VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
        color_revision = source_color_revision;
    }
    else if (upload_color)
    {
        copy_image(expand_color ? resolve_color : color_image, VK_IMAGE_ASPECT_COLOR_BIT,
                   color_width, color_height, color_offset, true);
        color_upload_bytes += (uint64_t)color_width * color_height * 4;
    }
    if (upload_depth && depth_source.handle)
    {
        copy_depth_history(depth_source, depth_image, width, height);
        depth_revision = source_revision;
        depth_dirty = true;
        deferred_depth_copies++;
    }
    else if (upload_depth && restore_depth)
    {
        copy_depth_history(depth_history_image, depth_image, width, height);
        gpu_depth_history_bytes += (uint64_t)width * height * 4;
    }
    else if (upload_depth)
    {
        copy_image(expand_depth ? resolve_depth : depth_image, VK_IMAGE_ASPECT_DEPTH_BIT,
                   depth_width, depth_height, depth_offset, true);
        depth_upload_bytes += (uint64_t)depth_width * depth_height * 4;
    }
    memory_barrier();
    if (upload_color && expand_color && !color_source.handle)
    {
        expand_to_physical(resolve_color, color_image, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    if (upload_depth && expand_depth && !depth_source.handle)
    {
        expand_to_physical(resolve_depth, depth_image, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
    memory_barrier();
    target_state = state;
    loaded_target = true;
    snapshot_native_target(state);
    if (upload_color && !color_source.handle)
    {
        color_revision = ++color_revision_clock;
    }
    if (upload_depth && !depth_source.handle)
    {
        depth_revision = ++depth_revision_clock;
        depth_dirty = false;
    }
}

VkBlendFactor blend_factor(unsigned value, bool alpha)
{
    switch (value)
    {
    case 0:
        return VK_BLEND_FACTOR_ZERO;
    case 1:
        return VK_BLEND_FACTOR_ONE;
    case 0x300:
        return alpha ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_SRC_COLOR;
    case 0x301:
        return alpha ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case 0x302:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case 0x303:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 0x304:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case 0x305:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case 0x306:
        return alpha ? VK_BLEND_FACTOR_DST_ALPHA : VK_BLEND_FACTOR_DST_COLOR;
    case 0x307:
        return alpha ? VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA : VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    default:
        return VK_BLEND_FACTOR_ONE;
    }
}

VkPipeline pipeline_for(const NvGpuDraw &state)
{
    PipelineKey key = {state.blend,      state.src,         state.dst,        state.equation, state.mask,
                       state.depth_test, state.depth_write, state.depth_func, state.mode};
    for (const Pipeline &pipeline : pipelines)
    {
        if (!std::memcmp(&pipeline.key, &key, sizeof key))
        {
            return pipeline.handle;
        }
    }
    /* Retiring a pipeline requires its queued uses to finish. */
    if (pipelines.size() >= 256)
    {
        nv_vk_sync_color_reason("pipeline cache budget");
        for (const Pipeline &pipeline : pipelines)
        {
            vkDestroyPipeline(device, pipeline.handle, nullptr);
        }
        pipelines.clear();
    }
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex_shader;
    stages[0].pName = "vertex";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = pixel_shader;
    stages[1].pName = "pixel";
    VkSpecializationMapEntry mode_entry = {0, 0, sizeof(state.mode)};
    VkSpecializationInfo specialization = {1, &mode_entry, sizeof(state.mode), &state.mode};
    stages[1].pSpecializationInfo = &specialization;
    VkVertexInputBindingDescription binding = {0, sizeof(NvGpuVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[8] = {};
    for (unsigned i = 0; i < 8; i++)
    {
        attributes[i] = {i, 0, i == 7 ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT, i * 16};
    }
    VkPipelineVertexInputStateCreateInfo input = {};
    input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input.vertexBindingDescriptionCount = 1;
    input.pVertexBindingDescriptions = &binding;
    input.vertexAttributeDescriptionCount = 8;
    input.pVertexAttributeDescriptions = attributes;
    VkPipelineInputAssemblyStateCreateInfo assembly = {};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport = {};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable = VK_TRUE;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.lineWidth = 1;
    VkPipelineMultisampleStateCreateInfo samples = {};
    samples.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth = {};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = state.depth_test || state.depth_write;
    depth.depthWriteEnable = state.depth_write;
    depth.depthCompareOp = state.depth_test ? (VkCompareOp)(state.depth_func - 0x200) : VK_COMPARE_OP_ALWAYS;
    VkPipelineColorBlendAttachmentState attachment = {};
    attachment.blendEnable = state.blend;
    attachment.srcColorBlendFactor = blend_factor(state.src, false);
    attachment.dstColorBlendFactor = blend_factor(state.dst, false);
    attachment.srcAlphaBlendFactor = blend_factor(state.src, true);
    attachment.dstAlphaBlendFactor = blend_factor(state.dst, true);
    attachment.colorBlendOp = state.equation == 0x800A   ? VK_BLEND_OP_SUBTRACT
                              : state.equation == 0x800B ? VK_BLEND_OP_REVERSE_SUBTRACT
                              : state.equation == 0x8007 ? VK_BLEND_OP_MIN
                              : state.equation == 0x8008 ? VK_BLEND_OP_MAX
                                                         : VK_BLEND_OP_ADD;
    attachment.alphaBlendOp = attachment.colorBlendOp;
    attachment.colorWriteMask = ((state.mask & 0x10000) ? VK_COLOR_COMPONENT_R_BIT : 0) |
                                ((state.mask & 0x100) ? VK_COLOR_COMPONENT_G_BIT : 0) |
                                ((state.mask & 1) ? VK_COLOR_COMPONENT_B_BIT : 0) |
                                ((state.mask & 0x1000000) ? VK_COLOR_COMPONENT_A_BIT : 0);
    VkPipelineColorBlendStateCreateInfo blend = {};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &attachment;
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic = {};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;
    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &samples;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = pipeline_layout;
    info.renderPass = render_pass;
    VkPipeline result;
    check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &result),
          "create graphics pipeline");
    pipelines.push_back({key, result});
    return result;
}

struct Constants
{
    float viewport[4], fog[4];
    uint32_t flags[4], alpha[4], program[8][4];
    float factor0[8][4], factor1[8][4], final0[4], final1[4];
    uint32_t control[4];
};
static_assert(sizeof(Constants) == 496, "shared HLSL constant layout");

Constants constants_for(const NvGpuDraw &state)
{
    Constants c = {};
    c.viewport[0] = (float)state.width;
    c.viewport[1] = (float)state.height;
    c.viewport[2] = state.zscale;
    std::memcpy(c.fog, state.fog_color, 16);
    c.flags[0] = state.mode;
    c.flags[1] = state.textured;
    for (unsigned i = 0; i < 3; i++)
    {
        if (state.extra[i].valid)
        {
            c.flags[1] |= 1u << (i + 1);
        }
    }
    c.flags[2] = state.clip_plane_mode;
    c.alpha[0] = state.alpha_test;
    c.alpha[1] = state.alpha_func;
    c.alpha[2] = state.alpha_ref;
    for (unsigned i = 0; i < 8; i++)
    {
        c.program[i][0] = state.combiner.color_icw[i];
        c.program[i][1] = state.combiner.alpha_icw[i];
        c.program[i][2] = state.combiner.color_ocw[i];
        c.program[i][3] = state.combiner.alpha_ocw[i];
        nv2a_rc_unpack_fast(state.combiner.factor0[i], c.factor0[i]);
        nv2a_rc_unpack_fast(state.combiner.factor1[i], c.factor1[i]);
    }
    nv2a_rc_unpack_fast(state.combiner.final_c0, c.final0);
    nv2a_rc_unpack_fast(state.combiner.final_c1, c.final1);
    c.control[0] = state.combiner.control;
    c.control[1] = state.combiner.final0;
    c.control[2] = state.combiner.final1;
    c.control[3] = state.combiner.stage_program;
    return c;
}

unsigned initialize_device()
{
    loader = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!loader)
    {
        throw std::runtime_error("Vulkan loader unavailable. Install a graphics driver with Vulkan support.");
    }
    get_instance_proc = (PFN_vkGetInstanceProcAddr)GetProcAddress(loader, "vkGetInstanceProcAddr");
    if (!get_instance_proc)
    {
        throw std::runtime_error("Vulkan loader is missing vkGetInstanceProcAddr");
    }
    vkCreateInstance = (PFN_vkCreateInstance)get_instance_proc(VK_NULL_HANDLE, "vkCreateInstance");
    if (!vkCreateInstance)
    {
        throw std::runtime_error("Vulkan loader cannot create an instance");
    }
    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "DogGame experimental renderer";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app;
    check(vkCreateInstance(&info, nullptr, &instance), "create Vulkan instance");
#define LOAD_INSTANCE(name) vk##name = (PFN_vk##name)get_instance_proc(instance, "vk" #name);
    VULKAN_FUNCTIONS(LOAD_INSTANCE)
#undef LOAD_INSTANCE
    get_device_proc = (PFN_vkGetDeviceProcAddr)get_instance_proc(instance, "vkGetDeviceProcAddr");
    uint32_t count = 0;
    check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "enumerate devices");
    std::vector<VkPhysicalDevice> devices(count);
    check(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "enumerate devices");
    unsigned queue_family = 0;
    int best_score = -1;
    for (VkPhysicalDevice candidate : devices)
    {
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties candidate_properties;
        vkGetPhysicalDeviceFeatures(candidate, &features);
        vkGetPhysicalDeviceProperties(candidate, &candidate_properties);
        if (!features.depthClamp)
        {
            continue;
        }
        VkFormatProperties color_format, depth_format;
        vkGetPhysicalDeviceFormatProperties(candidate, VK_FORMAT_B8G8R8A8_UNORM, &color_format);
        vkGetPhysicalDeviceFormatProperties(candidate, VK_FORMAT_D32_SFLOAT, &depth_format);
        if (!(color_format.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) ||
            !(depth_format.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
        {
            continue;
        }
        uint32_t families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, nullptr);
        std::vector<VkQueueFamilyProperties> queues(families);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, queues.data());
        for (unsigned i = 0; i < families; i++)
        {
            if (!queues[i].queueCount || !(queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                continue;
            }
            int score = candidate_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2 : 1;
            if (score > best_score)
            {
                physical = candidate;
                properties = candidate_properties;
                VkFormatFeatureFlags blit = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
                native_resolve_supported = (color_format.optimalTilingFeatures & blit) == blit &&
                                          (depth_format.optimalTilingFeatures & blit) == blit;
                queue_family = i;
                best_score = score;
            }
            break;
        }
    }
    if (!physical)
    {
        throw std::runtime_error(
            "No compatible Vulkan graphics device (BGRA8, D32 and depth clamp required)");
    }
    float priority = 1;
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures features = {};
    features.depthClamp = VK_TRUE;
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.pEnabledFeatures = &features;
    check(vkCreateDevice(physical, &device_info, nullptr, &device), "create Vulkan device");
#define LOAD_DEVICE(name)                                                                                    \
    if (auto proc = get_device_proc(device, "vk" #name))                                                     \
        vk##name = (PFN_vk##name)proc;
    VULKAN_FUNCTIONS(LOAD_DEVICE)
#undef LOAD_DEVICE
    vkGetDeviceQueue(device, queue_family, 0, &queue);
    vkGetPhysicalDeviceMemoryProperties(physical, &memory_properties);
    return queue_family;
}

void initialize_command_storage(unsigned queue_family)
{
    VkCommandPoolCreateInfo pool = {};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.queueFamilyIndex = queue_family;
    check(vkCreateCommandPool(device, &pool, nullptr, &command_pool), "create command pool");
    VkCommandBufferAllocateInfo commands = {};
    commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commands.commandPool = command_pool;
    commands.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commands.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(device, &commands, &command), "allocate commands");
    command_segments.push_back(command);
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    check(vkCreateFence(device, &fence_info, nullptr, &fence), "create fence");
    VkBufferCreateInfo buffer = {};
    buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer.size = arena_bytes;
    buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(device, &buffer, nullptr, &arena), "create upload arena");
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, arena, &requirements);
    VkMemoryAllocateInfo allocation = {};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex =
        memory_type(requirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::fprintf(stderr, "[GPU-VULKAN-MEMORY] upload type=%u flags=0x%X\n", allocation.memoryTypeIndex,
                 memory_properties.memoryTypes[allocation.memoryTypeIndex].propertyFlags);
    check(vkAllocateMemory(device, &allocation, nullptr, &arena_memory), "allocate upload arena");
    check(vkBindBufferMemory(device, arena, arena_memory, 0), "bind upload arena");
    check(vkMapMemory(device, arena_memory, 0, VK_WHOLE_SIZE, 0, (void **)&mapped), "map upload arena");
    buffer.size = attachment_bytes * 2;
    buffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    check(vkCreateBuffer(device, &buffer, nullptr, &readback_buffer), "create readback buffer");
    vkGetBufferMemoryRequirements(device, readback_buffer, &requirements);
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                             VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    readback_memory_flags = memory_properties.memoryTypes[allocation.memoryTypeIndex].propertyFlags;
    std::fprintf(stderr, "[GPU-VULKAN-MEMORY] readback type=%u flags=0x%X\n", allocation.memoryTypeIndex,
                 readback_memory_flags);
    check(vkAllocateMemory(device, &allocation, nullptr, &readback_memory), "allocate readback buffer");
    check(vkBindBufferMemory(device, readback_buffer, readback_memory, 0), "bind readback buffer");
    check(vkMapMemory(device, readback_memory, 0, VK_WHOLE_SIZE, 0, (void **)&readback_mapped),
          "map readback buffer");
}

void initialize_render_resources()
{
    VkAttachmentDescription attachments[2] = {};
    for (unsigned i = 0; i < 2; i++)
    {
        attachments[i].format = i ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_B8G8R8A8_UNORM;
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = attachments[i].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    VkAttachmentReference color_reference = {0, VK_IMAGE_LAYOUT_GENERAL};
    VkAttachmentReference depth_reference = {1, VK_IMAGE_LAYOUT_GENERAL};
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;
    VkRenderPassCreateInfo pass = {};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass.attachmentCount = 2;
    pass.pAttachments = attachments;
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    check(vkCreateRenderPass(device, &pass, nullptr, &render_pass), "create render pass");
    VkDescriptorSetLayoutBinding bindings[9] = {};
    for (unsigned i = 0; i < 9; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = i == 0  ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                     : i < 5 ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                                             : VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[i].stageFlags =
            i == 0 ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo descriptors = {};
    descriptors.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptors.bindingCount = 9;
    descriptors.pBindings = bindings;
    check(vkCreateDescriptorSetLayout(device, &descriptors, nullptr, &descriptor_layout),
          "create descriptor layout");
    VkPipelineLayoutCreateInfo layout = {};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &descriptor_layout;
    check(vkCreatePipelineLayout(device, &layout, nullptr, &pipeline_layout), "create pipeline layout");
    VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
                                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096},
                                    {VK_DESCRIPTOR_TYPE_SAMPLER, 4096}};
    VkDescriptorPoolCreateInfo descriptor_info = {};
    descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_info.maxSets = 1024;
    descriptor_info.poolSizeCount = 3;
    descriptor_info.pPoolSizes = sizes;
    check(vkCreateDescriptorPool(device, &descriptor_info, nullptr, &descriptor_pool),
          "create descriptor pool");
    VkShaderModuleCreateInfo shader = {};
    shader.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader.codeSize = sizeof nv_vk_vert_spirv;
    shader.pCode = nv_vk_vert_spirv;
    check(vkCreateShaderModule(device, &shader, nullptr, &vertex_shader), "create vertex shader");
    shader.codeSize = sizeof nv_vk_frag_spirv;
    shader.pCode = nv_vk_frag_spirv;
    check(vkCreateShaderModule(device, &shader, nullptr, &pixel_shader), "create fragment shader");
}

/* Publish only after the submission fence. Noncoherent memory is invalidated
 * over the complete mapped allocation, satisfying atom alignment even when the
 * selected device has no cached/coherent combination. */
void publish_attachments()
{
    CpuTimer timer(publish_ticks);
    const unsigned read_width = color_read_width, read_height = color_read_height;
    if (!(color_dirty || depth_dirty))
    {
        return;
    }
    if (!(readback_memory_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        VkMappedMemoryRange range = {};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = readback_memory;
        range.size = VK_WHOLE_SIZE;
        check(vkInvalidateMappedMemoryRanges(device, 1, &range), "invalidate readback memory");
    }
    const uint8_t *color_source = readback_mapped;
    const uint8_t *depth_source = readback_mapped + attachment_bytes;
    /* A driver may offer only uncached host-visible memory. Read it in bulk
     * before scalar depth/layout processing, never one PCIe/cache-line fetch
     * per guest pixel. These snapshots are title-thread owned. */
    static std::vector<uint8_t> color_snapshot, depth_snapshot;
    size_t bytes = (size_t)read_width * read_height * 4;
    if (!(readback_memory_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
    {
        if (color_dirty)
        {
            color_snapshot.resize(bytes);
            std::memcpy(color_snapshot.data(), color_source, bytes);
            color_source = color_snapshot.data();
        }
        if (depth_dirty)
        {
            size_t depth_bytes = (size_t)depth_read_width * depth_read_height * 4;
            depth_snapshot.resize(depth_bytes);
            std::memcpy(depth_snapshot.data(), depth_source, depth_bytes);
            depth_source = depth_snapshot.data();
        }
    }
    if (color_dirty)
    {
        for (unsigned y = 0; y < target_state.height; y++)
        {
            unsigned source_y = read_height == target_state.height ? y : std::min(read_height - 1,
                (unsigned)(((uint64_t)y * 2 + 1) * read_height / (target_state.height * 2)));
            const uint8_t *row = color_source + (size_t)source_y * read_width * 4;
            if (!target_state.surface_offset && read_width == target_state.width)
            {
                std::memcpy(target_state.color + (size_t)y * target_state.pitch, row, target_state.width * 4);
            }
            else
            {
                for (unsigned x = 0; x < target_state.width; x++)
                {
                    unsigned source_x = read_width == target_state.width ? x : std::min(read_width - 1,
                        (unsigned)(((uint64_t)x * 2 + 1) * read_width / (target_state.width * 2)));
                    size_t destination = target_state.surface_offset
                        ? target_state.surface_offset(target_state.surface_format, x, y)
                        : (size_t)y * target_state.pitch + x * 4;
                    std::memcpy(target_state.color + destination, row + source_x * 4, 4);
                }
            }
        }
    }
    if (depth_dirty && target_state.depth)
    {
        for (unsigned y = 0; y < target_state.height; y++)
        {
            for (unsigned x = 0; x < target_state.width; x++)
            {
                float z;
                unsigned source_x = depth_read_width == target_state.width ? x : std::min(depth_read_width - 1,
                    (unsigned)(((uint64_t)x * 2 + 1) * depth_read_width / (target_state.width * 2)));
                unsigned source_y = depth_read_height == target_state.height ? y : std::min(depth_read_height - 1,
                    (unsigned)(((uint64_t)y * 2 + 1) * depth_read_height / (target_state.height * 2)));
                std::memcpy(&z, depth_source + ((size_t)source_y * depth_read_width + source_x) * 4, 4);
                float &old = target_state.depth[(size_t)y * target_state.depth_stride + x];
                /* Match the maintained D3D readback sentinel behavior. */
                if (z != 1.f || old <= target_state.zscale)
                {
                    old = z * target_state.zscale;
                }
            }
        }
    }
    if (color_dirty && target_state.width == 640 && target_state.height == 480 &&
        !target_state.surface_offset && (read_width != 640 || read_height != 480))
    {
        /* Preserve the physical raster before the copied window publication.
         * Its native image validates CPU ownership at both reload and present.
         * No mapped Vulkan storage or guest pointer escapes to the UI thread. */
        present_pixels.resize((size_t)read_width * read_height);
        std::memcpy(present_pixels.data(), color_source, bytes);
        present_native.resize(640 * 480);
        for (unsigned y = 0; y < 480; y++)
        {
            std::memcpy(present_native.data() + y * 640,
                        target_state.color + (size_t)y * target_state.pitch, 640 * 4);
        }
        present_guest = target_state.color;
        present_pitch = target_state.pitch;
        present_width = read_width;
        present_height = read_height;
    }
    else if (color_dirty && target_state.color == present_guest)
    {
        present_guest = nullptr;
    }
    if (depth_dirty && target_state.depth &&
        (target_width != target_state.width || target_height != target_state.height))
    {
        preserved_native_depth.resize((size_t)target_state.width * target_state.height);
        for (unsigned y = 0; y < target_state.height; y++)
        {
            std::memcpy(preserved_native_depth.data() + (size_t)y * target_state.width,
                        target_state.depth + (size_t)y * target_state.depth_stride,
                        target_state.width * sizeof(float));
        }
        preserved_depth_guest = target_state.depth;
        preserved_depth_stride = target_state.depth_stride;
        preserved_depth_zscale = target_state.zscale;
        preserved_depth_width = target_width;
        preserved_depth_height = target_height;
        preserved_depth_guest_width = target_state.width;
        preserved_depth_guest_height = target_state.height;
    }
    else if (depth_dirty && target_state.depth == preserved_depth_guest)
    {
        preserved_depth_guest = nullptr;
    }
}

int reject_draw()
{
    fallbacks++;
    return 0;
}

void validate_mirrored_clears()
{
    bool edited = false;
    if (loaded_target && mirrored_color_clear)
    {
        for (unsigned y = 0; y < target_state.height && !edited; y++)
        {
            for (unsigned x = 0; x < target_state.width; x++)
            {
                size_t offset = target_state.surface_offset
                                    ? target_state.surface_offset(target_state.surface_format, x, y)
                                    : (size_t)y * target_state.pitch + x * 4;
                uint32_t value;
                std::memcpy(&value, target_state.color + offset, 4);
                if (value != mirrored_color_value)
                {
                    edited = true;
                    break;
                }
            }
        }
    }
    if (loaded_target && mirrored_depth_clear && target_state.depth)
    {
        for (unsigned y = 0; y < target_state.height && !edited; y++)
        {
            for (unsigned x = 0; x < target_state.width; x++)
            {
                if (target_state.depth[(size_t)y * target_state.depth_stride + x] != mirrored_depth_value)
                {
                    edited = true;
                    break;
                }
            }
        }
    }
    mirrored_color_clear = mirrored_depth_clear = false;
    if (edited)
    {
        /* The cleared attachment is already marked clean. Publishing the
         * other attachment cannot overwrite the CPU edit; force a reload. */
        nv_vk_sync_reason("CPU edit after mirrored clear");
        loaded_target = false;
        target_native_color.clear();
        target_native_depth.clear();
    }
}

void fatal(const char *message)
{
    std::fprintf(stderr, "[GPU-VULKAN] fatal: %s\n", message);
    MessageBoxA(nullptr, message, "DogGame: experimental Vulkan renderer", MB_OK | MB_ICONERROR);
    ExitProcess(4);
}
} // namespace

int nv_vk_initialize(char *error, unsigned capacity)
{
    if (device)
    {
        return 1;
    }
    try
    {
        unsigned queue_family = initialize_device();
        initialize_command_storage(queue_family);
        initialize_render_resources();
        std::fprintf(stderr,
                     "[GPU-VULKAN] experimental backend initialized: %s; selectable-resolution offscreen raster, "
                     "existing window presentation\n",
                     properties.deviceName);
        return 1;
    }
    catch (const std::exception &failure)
    {
        std::snprintf(error, capacity, "%s", failure.what());
        nv_vk_shutdown();
        return 0;
    }
}

int nv_vk_begin(const NvGpuDraw *state)
{
    CpuTimer timer(prepare_ticks);
    const NvGpuDraw &s = *state;
    if (!device || !s.color || !s.width || !s.height || s.width > 1024 || s.height > 1024 ||
        !std::isfinite(s.zscale) || s.zscale <= 0 || s.pitch < s.width * 4 ||
        (s.depth && s.depth_stride < s.width) || s.clip_x > s.width || s.clip_y > s.height ||
        s.clip_w > s.width - s.clip_x || s.clip_h > s.height - s.clip_y ||
        (s.depth_test && (s.depth_func < 0x200 || s.depth_func > 0x207)))
    {
        fallbacks++;
        return 0;
    }
    try
    {
        /* Decode before accepting the batch; failure still permits software
         * fallback. Keep the arena bounded by flushing between batches. */
        validate_mirrored_clears();
        if (cached_textures.size() >= 256 || cached_texture_bytes >= 128ull * 1024 * 1024)
        {
            nv_vk_sync_color_reason("texture cache budget");
            clear_texture_cache();
        }
        NvGpuTexture textures[4] = {};
        textures[0].valid = s.textured;
        textures[0].width = s.tw;
        textures[0].height = s.th;
        textures[0].decode = s.decode;
        textures[0].arg = s.decode_arg;
        textures[0].addr_u = s.addr_u;
        textures[0].addr_v = s.addr_v;
        textures[0].linear = s.linear;
        textures[0].key = s.texture_key;
        textures[0].hash = s.texture_hash;
        for (unsigned i = 0; i < 3; i++)
        {
            textures[i + 1] = s.extra[i];
        }
        std::vector<uint32_t> decoded[4];
        size_t cached[4];
        Image cached_images[4];
        VkDeviceSize texture_bytes = 0;
        for (unsigned i = 0; i < 4; i++)
        {
            NvGpuTexture &t = textures[i];
            cached[i] = find_texture(t);
            if (cached[i] != SIZE_MAX)
            {
                /* A later stage can replace the same cache identity with a
                 * new hash. Borrow this exact version for the whole batch. */
                cached_images[i] = cached_textures[cached[i]].image;
                texture_cache_hits++;
                continue;
            }
            if (!t.valid)
            {
                t.width = t.height = 1;
                decoded[i].push_back(0xFFFFFFFFu);
            }
            else
            {
                if (!t.decode || !t.width || !t.height || t.width > 2048 || t.height > 2048)
                {
                    return reject_draw();
                }
                decoded[i].resize((size_t)t.width * t.height);
                for (unsigned y = 0; y < t.height; y++)
                {
                    for (unsigned x = 0; x < t.width; x++)
                    {
                        if (!t.decode(t.arg, x, y, &decoded[i][(size_t)y * t.width + x]))
                        {
                            return reject_draw();
                        }
                    }
                }
            }
            texture_bytes += decoded[i].size() * 4;
        }
        if (texture_bytes > arena_bytes / 2)
        {
            return reject_draw();
        }
        if (cursor + texture_bytes + 8 * 1024 * 1024 > arena_bytes || draw_sets >= 512)
        {
            nv_vk_sync_color_reason("upload or descriptor budget");
        }
        draw_pipeline = pipeline_for(s);
        ensure_target(s);
        if (texture_bytes)
        {
            /* Transfers cannot run inside this render pass. Cached textures
             * need no transfer or pass break; their contents are immutable. */
            end_render_pass();
        }
        draw_state = s;
        vertices.clear();
        indices.clear();
        VkDescriptorSetAllocateInfo allocation = {};
        allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocation.descriptorPool = descriptor_pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &descriptor_layout;
        check(vkAllocateDescriptorSets(device, &allocation, &draw_descriptor), "allocate draw descriptors");
        draw_sets++;
        Constants constants = constants_for(s);
        VkDeviceSize offset = allocate_upload(
            sizeof constants, std::max<VkDeviceSize>(16, properties.limits.minUniformBufferOffsetAlignment));
        std::memcpy(mapped + offset, &constants, sizeof constants);
        VkDescriptorBufferInfo constant_info = {arena, offset, sizeof constants};
        VkWriteDescriptorSet writes[9] = {};
        for (unsigned i = 0; i < 9; i++)
        {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = draw_descriptor;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = i == 0  ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                       : i < 5 ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                                               : VK_DESCRIPTOR_TYPE_SAMPLER;
        }
        writes[0].pBufferInfo = &constant_info;
        VkDescriptorImageInfo image_info[4] = {};
        for (unsigned i = 0; i < 4; i++)
        {
            const NvGpuTexture &t = textures[i];
            if (cached[i] != SIZE_MAX)
            {
                const Image &image = cached_images[i];
                image_info[i] = {image.sampler, image.view, VK_IMAGE_LAYOUT_GENERAL};
                writes[i + 1].pImageInfo = writes[i + 5].pImageInfo = &image_info[i];
                continue;
            }
            Image image;
            make_image(image, t.width, t.height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
                       VK_IMAGE_ASPECT_COLOR_BIT);
            draw_textures.push_back(image);
            Image &owned = draw_textures.back();
            VkSamplerCreateInfo sampler = {};
            sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler.magFilter = sampler.minFilter = t.linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
            sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            sampler.addressModeU =
                t.addr_u == 1 ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler.addressModeV =
                t.addr_v == 1 ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            check(vkCreateSampler(device, &sampler, nullptr, &owned.sampler), "create sampler");
            offset = allocate_upload(decoded[i].size() * 4);
            std::memcpy(mapped + offset, decoded[i].data(), decoded[i].size() * 4);
            initialize_layout(owned, VK_IMAGE_ASPECT_COLOR_BIT);
            copy_image(owned, VK_IMAGE_ASPECT_COLOR_BIT, t.width, t.height, offset, true);
            image_info[i] = {owned.sampler, owned.view, VK_IMAGE_LAYOUT_GENERAL};
            writes[i + 1].pImageInfo = writes[i + 5].pImageInfo = &image_info[i];
            texture_uploads++;
            if (!t.valid || t.key)
            {
                /* Only executor-identified textures participate in reuse. */
                cache_texture(t, owned);
                draw_textures.pop_back();
            }
        }
        if (texture_bytes)
        {
            memory_barrier();
        }
        vkUpdateDescriptorSets(device, 9, writes, 0, nullptr);
        return 1;
    }
    catch (const std::exception &failure)
    {
        std::fprintf(stderr, "[GPU-VULKAN] rejected batch: %s\n", failure.what());
        fallbacks++;
        return 0;
    }
}

unsigned nv_vk_append_vertex(const NvGpuVertex *vertex)
{
    unsigned index = (unsigned)vertices.size();
    vertices.push_back(*vertex);
    return index;
}

void nv_vk_indexed_triangle(const unsigned triangle[3])
{
    indices.insert(indices.end(), triangle, triangle + 3);
}

void nv_vk_triangle(const NvGpuVertex *triangle)
{
    unsigned first = (unsigned)vertices.size();
    vertices.insert(vertices.end(), triangle, triangle + 3);
    for (unsigned i = 0; i < 3; i++)
    {
        indices.push_back(first + i);
    }
}

void nv_vk_end(void)
{
    CpuTimer timer(record_ticks);
    if (indices.empty())
    {
        return;
    }
    try
    {
        VkDeviceSize size = indices.size() * sizeof(NvGpuVertex);
        /* Geometry is expanded in this initial implementation. Flush/rebuild
         * descriptors if a large accepted batch crosses the upload budget. */
        if (cursor + size > arena_bytes)
        {
            std::vector<NvGpuVertex> saved_vertices = std::move(vertices);
            std::vector<unsigned> saved_indices = std::move(indices);
            NvGpuDraw saved_draw = draw_state;
            nv_vk_sync_color_reason("geometry upload budget");
            if (!nv_vk_begin(&saved_draw))
            {
                throw std::runtime_error("Cannot resume Vulkan geometry after arena flush");
            }
            vertices = std::move(saved_vertices);
            indices = std::move(saved_indices);
        }
        VkDeviceSize offset = allocate_upload(size);
        for (size_t i = 0; i < indices.size(); i++)
        {
            if (indices[i] >= vertices.size())
            {
                throw std::runtime_error("Invalid Vulkan batch vertex index");
            }
            std::memcpy(mapped + offset + i * sizeof(NvGpuVertex), &vertices[indices[i]],
                        sizeof(NvGpuVertex));
        }
        /* Fixed-function color/depth ordering is maintained inside a pass.
         * Consecutive draws need no whole-pipeline barrier between them. */
        begin_render_pass();
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, draw_pipeline);
        VkViewport viewport = {0, 0, (float)target_width, (float)target_height, 0, 1};
        unsigned left = (unsigned)((uint64_t)draw_state.clip_x * target_width / draw_state.width);
        unsigned top = (unsigned)((uint64_t)draw_state.clip_y * target_height / draw_state.height);
        unsigned right = (unsigned)((uint64_t)(draw_state.clip_x + draw_state.clip_w) * target_width / draw_state.width);
        unsigned bottom = (unsigned)((uint64_t)(draw_state.clip_y + draw_state.clip_h) * target_height / draw_state.height);
        VkRect2D scissor = {{(int32_t)left, (int32_t)top}, {right - left, bottom - top}};
        vkCmdSetViewport(command, 0, 1, &viewport);
        vkCmdSetScissor(command, 0, 1, &scissor);
        vkCmdBindVertexBuffers(command, 0, 1, &arena, &offset);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                                &draw_descriptor, 0, nullptr);
        vkCmdDraw(command, (uint32_t)indices.size(), 1, 0, 0);
        color_dirty = color_dirty || (draw_state.mask & 0x01010101u) != 0;
        if (draw_state.mask & 0x01010101u)
        {
            color_revision = ++color_revision_clock;
        }
        depth_dirty = depth_dirty || draw_state.depth_write;
        if (draw_state.depth_write)
        {
            depth_revision = ++depth_revision_clock;
        }
        batches++;
        triangles += indices.size() / 3;
        if (++segment_draws >= draws_per_submission)
        {
            submit_completed_segment();
        }
    }
    catch (const std::exception &failure)
    {
        /* Once begin accepted geometry there is no software replay contract.
         * Surface/device failures must be visible rather than dropping draws. */
        fatal(failure.what());
    }
}

void nv_vk_sync(void)
{
    nv_vk_sync_reason("explicit boundary");
}

const uint32_t *nv_vk_present_image(const uint8_t *guest, unsigned pitch, unsigned *width, unsigned *height)
{
    PresentInterval interval;
    if (!xbox_DisplayInternalScaling())
    {
        return nullptr;
    }
    unsigned selected_width, selected_height;
    xbox_DisplayResolutionSize(xbox_DisplayResolutionSelected(), &selected_width, &selected_height);
    if (selected_width == 640 && selected_height == 480)
    {
        return nullptr;
    }
    if (!present_matches(guest, pitch) || present_width != selected_width || present_height != selected_height)
    {
        /* Intermediate guest readers need only native pixels. Materialize the
         * physical color once, at presentation, from a validated resident
         * image even if another target is currently bound. */
        NvGpuDraw state = {};
        if (loaded_target && target_state.color == guest && target_state.pitch == pitch &&
            target_state.width == 640 && target_state.height == 480 && native_color_matches(target_state))
        {
            state = target_state;
        }
        else
        {
            for (const ResidentTarget &entry : resident_targets)
            {
                if (entry.state.color != guest || entry.state.pitch != pitch || entry.state.width != 640 ||
                    entry.state.height != 480 || entry.state.surface_offset || entry.native_color.size() != 640 * 480)
                {
                    continue;
                }
                bool matches = true;
                for (unsigned y = 0; y < 480 && matches; y++)
                {
                    matches = !std::memcmp(guest + (size_t)y * pitch, entry.native_color.data() + y * 640, 640 * 4);
                }
                if (matches)
                {
                    state = entry.state;
                    break;
                }
            }
        }
        if (!state.color)
        {
            return nullptr;
        }
        ensure_target(state);
        color_dirty = true;
        nv_vk_sync_color_reason("presentation");
    }
    *width = present_width;
    *height = present_height;
    return present_pixels.data();
}

void nv_vk_discard_snapshots(const uint8_t *color, const float *depth)
{
    if ((color && color == target_state.color) || (depth && depth == target_state.depth))
    {
        loaded_target = false;
        target_native_color.clear();
        target_native_depth.clear();
    }
    for (ResidentTarget &entry : resident_targets)
    {
        if (color && color == entry.state.color)
        {
            entry.native_color.clear();
        }
        if (depth && depth == entry.state.depth)
        {
            entry.native_depth.clear();
            entry.pending_depth = false;
        }
    }
    if (color && color == present_guest)
    {
        present_guest = nullptr;
    }
    if (depth && depth == preserved_depth_guest)
    {
        preserved_depth_guest = nullptr;
    }
}

void nv_vk_prepare_cpu_clear(const uint8_t *color, unsigned pitch, unsigned format,
                             unsigned x, unsigned y, unsigned width, unsigned height,
                             const float *depth)
{
    /* Texture sources have already been copied into backend-owned uploads.
     * A validated linear color clear and an unrelated host depth buffer cannot
     * consume or overwrite this active target. Pooled targets are already
     * fence-complete. Keep recording instead of flushing unrelated rendering.
     * A null color is deliberately conservative: partial-channel clears can
     * reach the dispatcher without a validated color pointer. */
    if (loaded_target && color && !format && !target_state.surface_format &&
        depth != target_state.depth && pitch && height && target_state.pitch)
    {
        uint64_t clear_begin = (uintptr_t)color;
        uint64_t clear_end = clear_begin + ((uint64_t)y + height) * pitch +
            ((uint64_t)x + width) * 4;
        uint64_t target_begin = (uintptr_t)target_state.color;
        uint64_t target_end = target_begin + (uint64_t)target_state.height * target_state.pitch;
        if (clear_end >= clear_begin && target_end >= target_begin &&
            (clear_end <= target_begin || target_end <= clear_begin))
        {
            nv_vk_discard_snapshots(color, depth);
            return;
        }
    }
    /* The executor's zbuf_clear overwrites its complete 1024x1024 host buffer.
     * clear_surface overwrites the supplied color rectangle. Reading the old
     * values back first serves no consumer, but unrelated attachment writes
     * must still be published before the CPU runs. This does not remove the
     * fence or permit reuse of resources while GPU commands are in flight. */
    if (loaded_target)
    {
        if (depth && depth == target_state.depth)
        {
            depth_dirty = false;
        }
        if (color && color == target_state.color && pitch == target_state.pitch &&
            format == target_state.surface_format && !x && !y &&
            width >= target_state.width && height >= target_state.height)
        {
            color_dirty = false;
        }
    }
    nv_vk_sync_reason("unsupported CPU clear");
    nv_vk_discard_snapshots(color, depth);
}

int nv_vk_clear_attachments(const NvGpuClear *clear)
{
    if (clear && clear->color && (!loaded_target || clear->color != target_state.color) &&
        !clear->x && !clear->y)
    {
        auto found = std::find_if(resident_targets.begin(), resident_targets.end(),
            [&](const ResidentTarget &entry)
            {
                return entry.state.color == clear->color && entry.state.pitch == clear->pitch &&
                    entry.state.surface_format == clear->format && clear->width >= entry.state.width &&
                    clear->height >= entry.state.height && (!clear->depth || clear->depth == entry.state.depth);
            });
        if (found != resident_targets.end())
        {
            NvGpuDraw state = found->state;
            ensure_target(state);
        }
    }
    if (!clear || !loaded_target || (!clear->color && !clear->depth))
    {
        return 0;
    }
    if (clear->color && (clear->color != target_state.color || clear->pitch != target_state.pitch ||
                         clear->format != target_state.surface_format || clear->x || clear->y ||
                         clear->width < target_state.width || clear->height < target_state.height))
    {
        return 0;
    }
    if (clear->depth && (clear->depth != target_state.depth || !std::isfinite(clear->depth_value)))
    {
        return 0;
    }
    /* The executor immediately performs the matching CPU clear after success.
     * Discard only the overwritten dirty attachment; retain unrelated writes.
     * All recording happens outside a render pass on the title thread. */
    end_render_pass();
    begin_commands();
    memory_barrier();
    if (clear->color)
    {
        if (present_guest == clear->color)
        {
            present_guest = nullptr;
        }
        VkClearColorValue value = {};
        nv2a_rc_unpack_fast(clear->color_value, value.float32);
        VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(command, color_image.handle, VK_IMAGE_LAYOUT_GENERAL, &value, 1, &range);
        color_dirty = false;
        color_revision = ++color_revision_clock;
        mirrored_color_clear = true;
        for (ResidentTarget &entry : resident_targets)
        {
            if (entry.state.color == clear->color)
            {
                entry.native_color.clear();
            }
        }
        mirrored_color_value = clear->color_value;
        gpu_color_clears++;
    }
    if (clear->depth)
    {
        if (preserved_depth_guest == clear->depth)
        {
            preserved_depth_guest = nullptr;
        }
        VkClearDepthStencilValue value = {};
        value.depth = std::max(0.f, std::min(1.f, clear->depth_value / target_state.zscale));
        VkImageSubresourceRange range = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        vkCmdClearDepthStencilImage(command, depth_image.handle, VK_IMAGE_LAYOUT_GENERAL, &value, 1, &range);
        depth_dirty = false;
        depth_revision = ++depth_revision_clock;
        for (ResidentTarget &entry : resident_targets)
        {
            if (entry.state.depth == clear->depth)
            {
                entry.pending_depth = false;
                entry.native_depth.clear();
            }
        }
        mirrored_depth_clear = true;
        mirrored_depth_value = clear->depth_value;
        gpu_depth_clears++;
    }
    memory_barrier();
    return 1;
}

namespace
{
void sync_active_target(const char *reason, bool publish_depth)
{
    if (!recording)
    {
        if (!publish_depth || !depth_dirty)
        {
            return;
        }
        begin_commands();
    }
    try
    {
        bool retained_depth = depth_dirty && !publish_depth;
        if (retained_depth)
        {
            depth_dirty = false;
            deferred_depth_boundaries++;
        }
        uint64_t wait_before = wait_ticks;
        end_render_pass();
        memory_barrier();
        bool scaled = target_width != target_state.width || target_height != target_state.height;
        if (depth_dirty && target_state.depth && scaled)
        {
            retain_depth_history();
            memory_barrier();
        }
        bool resolve_native = native_resolve_supported &&
            scaled &&
            std::strcmp(reason, "presentation") && std::strcmp(reason, "diagnostic screenshot") &&
            std::strcmp(reason, "explicit boundary");
        color_read_width = resolve_native ? target_state.width : target_width;
        color_read_height = resolve_native ? target_state.height : target_height;
        depth_read_width = target_width;
        depth_read_height = target_state.height;
        if (resolve_native && color_dirty)
        {
            prepare_native_resolve();
            if (resolve_native && color_dirty)
            {
                resolve_to_native(color_image, resolve_color, VK_IMAGE_ASPECT_COLOR_BIT);
            }
            memory_barrier();
        }
        if (color_dirty)
        {
            copy_image(resolve_native ? resolve_color : color_image, VK_IMAGE_ASPECT_COLOR_BIT, color_read_width, color_read_height, 0,
                       false);
        }
        if (depth_dirty)
        {
            if (target_height != target_state.height)
            {
                copy_depth_sample_rows();
            }
            else
            {
                copy_image(depth_image, VK_IMAGE_ASPECT_DEPTH_BIT, depth_read_width, depth_read_height,
                           attachment_bytes, false);
            }
        }
        VkMemoryBarrier host = {};
        host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host,
                             0, nullptr, 0, nullptr);
        check(vkEndCommandBuffer(command), "end commands");
        VkSubmitInfo submit = {};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        {
            CpuTimer timer(submit_ticks);
            check(vkQueueSubmit(queue, 1, &submit, fence), "submit commands");
        }
        {
            CpuTimer timer(wait_ticks);
            check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "wait for rendering");
        }
        auto row = std::find_if(sync_audit.begin(), sync_audit.end(),
                                [reason](const SyncAudit &entry) { return entry.reason == reason; });
        if (row == sync_audit.end())
        {
            SyncAudit entry;
            entry.reason = reason;
            sync_audit.push_back(entry);
            row = sync_audit.end() - 1;
        }
        row->calls++;
        row->wait += wait_ticks - wait_before;
        uint64_t bytes = (uint64_t)color_read_width * color_read_height * 4;
        if (color_dirty)
        {
            row->color_bytes += bytes;
        }
        if (depth_dirty)
        {
            row->depth_bytes += (uint64_t)depth_read_width * depth_read_height * 4;
        }
        publish_attachments();
        if (color_dirty)
        {
            for (ResidentTarget &entry : resident_targets)
            {
                if (entry.state.color == target_state.color && entry.color_revision < color_revision)
                {
                    entry.native_color.clear();
                }
            }
        }
        if (depth_dirty)
        {
            for (ResidentTarget &entry : resident_targets)
            {
                if (same_depth_allocation(entry.state, target_state) && entry.depth_revision < depth_revision)
                {
                    entry.pending_depth = false;
                    entry.native_depth.clear();
                }
            }
        }
        if (color_dirty || depth_dirty)
        {
            readbacks++;
        }
        check(vkResetFences(device, 1, &fence), "reset fence");
        check(vkResetCommandPool(device, command_pool, 0), "reset commands");
        command_segment = segment_draws = 0;
        command = command_segments[0];
        check(vkResetDescriptorPool(device, descriptor_pool, 0), "reset descriptors");
        for (Image &image : draw_textures)
        {
            destroy_image(image);
        }
        draw_textures.clear();
        for (Image &image : retired_textures)
        {
            destroy_image(image);
        }
        retired_textures.clear();
        cursor = 0;
        draw_sets = 0;
        snapshot_native_target(target_state, color_dirty || mirrored_color_clear, depth_dirty || mirrored_depth_clear);
        recording = color_dirty = depth_dirty = false;
        depth_dirty = retained_depth;
        mirrored_color_clear = mirrored_depth_clear = false;
    }
    catch (const std::exception &failure)
    {
        fatal(failure.what());
    }
}

void publish_resident_depth(ResidentTarget &entry)
{
    if (!entry.pending_depth)
    {
        return;
    }
    if (same_depth_allocation(target_state, entry.state) && depth_revision > entry.depth_revision)
    {
        entry.pending_depth = false;
        entry.native_depth.clear();
        return;
    }
    for (const ResidentTarget &other : resident_targets)
    {
        if (same_depth_allocation(other.state, entry.state) && other.depth_revision > entry.depth_revision)
        {
            entry.pending_depth = false;
            entry.native_depth.clear();
            return;
        }
    }
    /* Called only after active color work has reached its fence. Resource
     * handles remain owned by their original entry across this temporary bind. */
    auto exchange = [&]()
    {
        std::swap(target_state, entry.state);
        std::swap(color_image, entry.color);
        std::swap(depth_image, entry.depth);
        std::swap(framebuffer, entry.framebuffer);
        std::swap(target_width, entry.width);
        std::swap(target_height, entry.height);
        target_native_color.swap(entry.native_color);
        target_native_depth.swap(entry.native_depth);
        std::swap(depth_dirty, entry.pending_depth);
        std::swap(depth_revision, entry.depth_revision);
        std::swap(color_revision, entry.color_revision);
    };
    bool was_loaded = loaded_target;
    exchange();
    loaded_target = true;
    sync_active_target("CPU depth consumer", true);
    exchange();
    loaded_target = was_loaded;
}
} // namespace

void nv_vk_sync_reason(const char *reason)
{
    for (const ResidentTarget &entry : resident_targets)
    {
        if (same_depth_allocation(target_state, entry.state) && entry.depth_revision > depth_revision)
        {
            depth_dirty = false;
        }
    }
    sync_active_target(reason, true);
    for (ResidentTarget &entry : resident_targets)
    {
        publish_resident_depth(entry);
    }
}

void nv_vk_sync_color_reason(const char *reason)
{
    sync_active_target(reason, false);
}

void nv_vk_depth_slot_reused(const float *depth)
{
    /* The old guest allocation is no longer represented by this CPU slot.
     * Recorded GPU commands own images, never the float buffer. Keep those
     * images alive through their existing fences, but never publish them into
     * the replacement allocation. The new generation prevents cache aliasing. */
    if (target_state.depth == depth)
    {
        depth_dirty = false;
        target_native_depth.clear();
    }
    for (ResidentTarget &entry : resident_targets)
    {
        if (entry.state.depth == depth)
        {
            entry.pending_depth = false;
            entry.native_depth.clear();
        }
    }
    if (preserved_depth_guest == depth)
    {
        preserved_depth_guest = nullptr;
    }
}

void nv_vk_report(void)
{
    std::fprintf(stderr, "[GPU-VULKAN-DEPTH-OWNERSHIP] deferred=%llu gpu-copies=%llu\n",
                 (unsigned long long)deferred_depth_boundaries, (unsigned long long)deferred_depth_copies);
    if (!present_intervals.empty())
    {
        std::sort(present_intervals.begin(), present_intervals.end());
        double sum = 0;
        unsigned late = 0;
        for (double ms : present_intervals)
        {
            sum += ms;
            if (ms > 36.667)
            {
                late++;
            }
        }
        size_t n = present_intervals.size();
        std::fprintf(stderr, "[GPU-VULKAN-PACING] recent=%zu mean=%.3f ms p95=%.3f ms p99=%.3f ms max=%.3f ms over36.667=%u; title publication, not scanout\n",
            n, sum / n, present_intervals[(n - 1) * 95 / 100], present_intervals[(n - 1) * 99 / 100],
            present_intervals.back(), late);
        present_intervals.clear();
        present_interval_cursor = 0;
    }
    std::fprintf(stderr, "[GPU-VULKAN-RESIDENT] hits=%llu pooled=%zu color-upload=%.2f MiB depth-upload=%.2f MiB\n",
        (unsigned long long)resident_hits, resident_targets.size(), color_upload_bytes / 1048576.0, depth_upload_bytes / 1048576.0);
    std::fprintf(stderr, "[GPU-VULKAN-RESOLVE] supported=%d attachments=%llu upload-expansions=%llu\n", native_resolve_supported,
        (unsigned long long)native_resolves, (unsigned long long)native_upload_expansions);
    std::fprintf(stderr, "[GPU-VULKAN-HISTORY] physical-color-restored=%.2f MiB physical-depth-restored=%.2f MiB gpu-depth-restored=%.2f MiB\n",
        physical_color_restore_bytes / 1048576.0, physical_depth_restore_bytes / 1048576.0,
        gpu_depth_history_bytes / 1048576.0);
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double milliseconds = 1000.0 / frequency.QuadPart;
    for (const SyncAudit &entry : sync_audit)
    {
        std::fprintf(stderr, "[GPU-VULKAN-SYNC] %s calls=%llu wait=%.1f ms color=%.2f MiB depth=%.2f MiB\n",
                     entry.reason.c_str(), (unsigned long long)entry.calls, entry.wait * milliseconds,
                     entry.color_bytes / 1048576.0, entry.depth_bytes / 1048576.0);
    }
    std::fprintf(stderr, "[GPU-VULKAN-CLEAR] color=%llu depth=%llu\n", (unsigned long long)gpu_color_clears,
                 (unsigned long long)gpu_depth_clears);
    std::fprintf(stderr, "[GPU-VULKAN-SUBMIT] early=%llu retained-command-buffers=%zu\n",
                 (unsigned long long)early_submissions, command_segments.size());
    std::fprintf(stderr, "[GPU-VULKAN-PASSES] passes=%llu draws=%llu\n",
        (unsigned long long)render_passes, (unsigned long long)batches);
    std::fprintf(stderr,
                 "[GPU-VULKAN-CPU] cumulative prepare=%.1f ms record=%.1f ms submit=%.1f ms wait=%.1f ms "
                 "publish=%.1f ms; prepare/record include nested flushes\n",
                 prepare_ticks * milliseconds, record_ticks * milliseconds, submit_ticks * milliseconds,
                 wait_ticks * milliseconds, publish_ticks * milliseconds);
    std::fprintf(stderr,
                 "[GPU-VULKAN] batches=%llu triangles=%llu readback-boundaries=%llu rejected=%llu "
                 "pipelines=%zu texture-uploads=%llu texture-hits=%llu texture-replacements=%llu\n",
                 (unsigned long long)batches, (unsigned long long)triangles, (unsigned long long)readbacks,
                 (unsigned long long)fallbacks, pipelines.size(), (unsigned long long)texture_uploads,
                 (unsigned long long)texture_cache_hits, (unsigned long long)texture_replacements);
}

void nv_vk_texture_barrier(const uint8_t *data, uint64_t bytes)
{
    if (!loaded_target || !color_dirty)
    {
        return;
    }
    /* Texture decoding reads guest RAM. Publish only when its range overlaps
     * the pending color surface; ordinary static textures need no readback.
     * Nonlinear surface spans have not been recovered here: synchronize them
     * conservatively rather than infer an allocation size from row pitch. */
    if (target_state.surface_offset)
    {
        nv_vk_sync_color_reason("nonlinear texture read");
        return;
    }
    uintptr_t source = (uintptr_t)data;
    uintptr_t target = (uintptr_t)target_state.color;
    uint64_t span = (uint64_t)target_state.pitch * target_state.height;
    bool overlaps = source >= target ? source - target < span : target - source < bytes;
    if (overlaps)
    {
        nv_vk_sync_color_reason("render-target texture read");
    }
}

void nv_vk_shutdown(void)
{
    if (device)
    {
        /* Also supports partial initialization failure. Unsubmitted commands
         * are discarded; normal shutdown publishes them first. */
        if (recording)
        {
            nv_vk_sync_reason("shutdown");
        }
        vkDeviceWaitIdle(device);
        for (ResidentTarget &entry : resident_targets)
        {
            vkDestroyFramebuffer(device, entry.framebuffer, nullptr);
            destroy_image(entry.color);
            destroy_image(entry.depth);
        }
        resident_targets.clear();
        for (Image &image : retired_textures)
        {
            destroy_image(image);
        }
        retired_textures.clear();
        clear_texture_cache();
        if (command_pool)
        {
            vkDestroyCommandPool(device, command_pool, nullptr);
        }
        for (Image &image : draw_textures)
        {
            destroy_image(image);
        }
        draw_textures.clear();
        if (framebuffer)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        destroy_image(color_image);
        destroy_image(depth_image);
        destroy_image(resolve_color);
        destroy_image(resolve_depth);
        destroy_image(depth_history_image);
        for (const Pipeline &pipeline : pipelines)
        {
            vkDestroyPipeline(device, pipeline.handle, nullptr);
        }
        pipelines.clear();
        if (descriptor_pool)
        {
            vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
        }
        if (pipeline_layout)
        {
            vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        }
        if (descriptor_layout)
        {
            vkDestroyDescriptorSetLayout(device, descriptor_layout, nullptr);
        }
        if (vertex_shader)
        {
            vkDestroyShaderModule(device, vertex_shader, nullptr);
        }
        if (pixel_shader)
        {
            vkDestroyShaderModule(device, pixel_shader, nullptr);
        }
        if (render_pass)
        {
            vkDestroyRenderPass(device, render_pass, nullptr);
        }
        if (readback_mapped)
        {
            vkUnmapMemory(device, readback_memory);
        }
        if (readback_buffer)
        {
            vkDestroyBuffer(device, readback_buffer, nullptr);
        }
        if (readback_memory)
        {
            vkFreeMemory(device, readback_memory, nullptr);
        }
        if (mapped)
        {
            vkUnmapMemory(device, arena_memory);
        }
        if (arena)
        {
            vkDestroyBuffer(device, arena, nullptr);
        }
        if (arena_memory)
        {
            vkFreeMemory(device, arena_memory, nullptr);
        }
        if (fence)
        {
            vkDestroyFence(device, fence, nullptr);
        }
        vkDestroyDevice(device, nullptr);
    }
    if (instance)
    {
        vkDestroyInstance(instance, nullptr);
    }
    if (loader)
    {
        FreeLibrary(loader);
    }
    loader = nullptr;
    instance = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    physical = VK_NULL_HANDLE;
    framebuffer = VK_NULL_HANDLE;
    command_pool = VK_NULL_HANDLE;
    command_segments.clear();
    command_segment = segment_draws = 0;
    command = VK_NULL_HANDLE;
    descriptor_pool = VK_NULL_HANDLE;
    pipeline_layout = VK_NULL_HANDLE;
    descriptor_layout = VK_NULL_HANDLE;
    vertex_shader = pixel_shader = VK_NULL_HANDLE;
    render_pass = VK_NULL_HANDLE;
    arena = VK_NULL_HANDLE;
    arena_memory = VK_NULL_HANDLE;
    mapped = nullptr;
    readback_buffer = VK_NULL_HANDLE;
    readback_memory = VK_NULL_HANDLE;
    readback_mapped = nullptr;
    readback_memory_flags = 0;
    fence = VK_NULL_HANDLE;
    recording = loaded_target = color_dirty = depth_dirty = false;
    depth_revision = 0;
    color_revision = 0;
    render_pass_open = false;
    mirrored_color_clear = mirrored_depth_clear = false;
    target_width = target_height = 0;
    previous_present_tick = 0;
    present_intervals.clear();
    present_interval_cursor = 0;
    resolve_width = resolve_height = 0;
    depth_history_width = depth_history_height = 0;
    target_native_color.clear();
    target_native_depth.clear();
    present_guest = nullptr;
    present_pitch = present_width = present_height = 0;
    present_pixels.clear();
    present_native.clear();
    preserved_depth_guest = nullptr;
    preserved_native_depth.clear();
    cursor = 0;
    draw_sets = 0;
}
