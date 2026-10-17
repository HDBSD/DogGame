/* Experimental hardware raster path. Guest RAM remains authoritative at
 * software draws, clears, render-target switches and presentation boundaries. */
#include "nv2a_gpu.h"
extern "C" {
#include "../../include/xbox/display_options.h"
}
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <utility>
#include <cmath>
#include <xmmintrin.h>
#include <emmintrin.h>

template<class T> static void release(T *&p){if(p)p->Release();p=nullptr;}
static ID3D11Device *device;
static ID3D11DeviceContext *context;
static ID3D11VertexShader *vs;
static ID3D11PixelShader *ps;
static ID3D11PixelShader *simple_ps[8];
static uint64_t simple_shader_draws;
static ID3D11InputLayout *layout;
static ID3D11Buffer *vb,*cb;
static ID3D11Texture2D *color,*depth,*color_read,*depth_read;
static ID3D11RenderTargetView *rtv;
static ID3D11DepthStencilView *dsv;
static ID3D11ShaderResourceView *texture;
static ID3D11SamplerState *sampler;
static ID3D11ShaderResourceView *extra_texture[3];
static ID3D11SamplerState *extra_sampler[3];
static ID3D11BlendState *blend;
static ID3D11DepthStencilState *ds;
static ID3D11RasterizerState *raster;
static NvGpuDraw active,draw;
static bool initialized,failed,dirty,depth_dirty,color_dirty;
static uint64_t depth_readbacks, skipped_depth_readbacks;
static unsigned capacity;
static unsigned vertex_cursor;
static unsigned target_width,target_height;
static float render_scale_x=1,render_scale_y=1;
static std::vector<uint32_t> scaled_color;
static ID3D11Texture2D *preserved_color;
static uint64_t preserved_uploads;
static ID3D11Texture2D *preserved_depth;
static float *preserved_guest_depth;
static std::vector<float> preserved_native_depth;
static unsigned preserved_depth_width,preserved_depth_height,preserved_depth_stride;
static float preserved_depth_scale;
static uint64_t preserved_depth_uploads;

/* Only the title thread owns this immediate context. These borrowed pointers
 * describe bindings held alive by the context, not application ownership.
 * Readback unbinds render targets and invalidates this snapshot before any
 * later draw. Compute resolve uses CS bindings only. Any new graphics-context
 * mutation outside bind_draw_pipeline must invalidate the snapshot too. */
struct DrawBindings
{
    bool valid;
    ID3D11Buffer *vertices;
    ID3D11PixelShader *pixel_shader;
    ID3D11ShaderResourceView *textures[4];
    ID3D11SamplerState *samplers[4];
    ID3D11RenderTargetView *color;
    ID3D11DepthStencilView *depth;
    ID3D11BlendState *blend;
    ID3D11DepthStencilState *depth_state;
    D3D11_VIEWPORT viewport;
    D3D11_RECT scissor;
};
static DrawBindings bound_draw;
static unsigned char uploaded_constants[496];
static bool constants_current;
static uint64_t constant_uploads, skipped_constant_uploads;
static uint64_t pipeline_binds, skipped_pipeline_binds;

static bool submission_cache_enabled()
{
    static const bool enabled = []()
    {
        const char *setting = std::getenv("RECOMP_GPU_STATE_CACHE");
        return !setting || std::strcmp(setting, "0") != 0;
    }();
    return enabled;
}

static void bind_draw_pipeline(ID3D11PixelShader *pixel_shader)
{
    DrawBindings next = {};
    next.valid = true;
    next.vertices = vb;
    next.pixel_shader = pixel_shader;
    next.textures[0] = texture;
    next.samplers[0] = sampler;
    for (unsigned i = 0; i < 3; i++)
    {
        next.textures[i + 1] = extra_texture[i];
        next.samplers[i + 1] = extra_sampler[i];
    }
    next.color = rtv;
    next.depth = dsv;
    next.blend = blend;
    next.depth_state = ds;
    next.viewport = {0, 0, draw.width * render_scale_x, draw.height * render_scale_y, 0, 1};
    next.scissor = {(LONG)(draw.clip_x * render_scale_x), (LONG)(draw.clip_y * render_scale_y),
        (LONG)((draw.clip_x + draw.clip_w) * render_scale_x),
        (LONG)((draw.clip_y + draw.clip_h) * render_scale_y)};

    bool reset = !bound_draw.valid || !submission_cache_enabled();
    if (reset)
    {
        context->IASetInputLayout(layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &cb);
        context->PSSetConstantBuffers(0, 1, &cb);
        context->RSSetState(raster);
        pipeline_binds += 6;
    }
    else
    {
        skipped_pipeline_binds += 6;
    }
    if (reset || next.vertices != bound_draw.vertices)
    {
        UINT stride = sizeof(NvGpuVertex), offset = 0;
        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || next.pixel_shader != bound_draw.pixel_shader)
    {
        context->PSSetShader(pixel_shader, nullptr, 0);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || std::memcmp(next.textures, bound_draw.textures, sizeof(next.textures)))
    {
        context->PSSetShaderResources(0, 4, next.textures);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || std::memcmp(next.samplers, bound_draw.samplers, sizeof(next.samplers)))
    {
        context->PSSetSamplers(0, 4, next.samplers);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || next.color != bound_draw.color || next.depth != bound_draw.depth)
    {
        context->OMSetRenderTargets(1, &rtv, dsv);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || next.blend != bound_draw.blend)
    {
        context->OMSetBlendState(blend, nullptr, ~0u);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || next.depth_state != bound_draw.depth_state)
    {
        context->OMSetDepthStencilState(ds, 0);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || std::memcmp(&next.viewport, &bound_draw.viewport, sizeof(next.viewport)))
    {
        context->RSSetViewports(1, &next.viewport);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    if (reset || std::memcmp(&next.scissor, &bound_draw.scissor, sizeof(next.scissor)))
    {
        context->RSSetScissorRects(1, &next.scissor);
        pipeline_binds++;
    }
    else skipped_pipeline_binds++;
    bound_draw = next;
}

/* Resident pairs own their GPU attachments and staging resources. Guest RAM
 * still becomes current at existing sync boundaries. Snapshots certify only
 * that the bytes used by this layout have not changed since upload/readback;
 * they are not guest write tracking and do not authorize deferred CPU reads.
 * All ownership changes and snapshot access run on the title thread. */
struct TargetResources
{
    unsigned width, height;
    ID3D11Texture2D *color, *depth, *color_read, *depth_read;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    ID3D11ShaderResourceView *color_view;
    NvGpuDraw guest;
    std::vector<uint8_t> color_snapshot, depth_snapshot;
    std::vector<uint64_t> color_offsets;
};
static std::vector<TargetResources> target_pool;
static constexpr uint64_t surface_pool_budget = 128ull * 1024 * 1024;
static uint64_t surface_resource_bytes(unsigned width, unsigned height)
{
    // Color/depth attachments and their staging images are all four bytes/pixel.
    return (uint64_t)width * height * 16;
}
static uint64_t surface_pool_bytes()
{
    uint64_t bytes = 0;
    for (const auto &entry : target_pool) bytes += surface_resource_bytes(entry.width, entry.height);
    return bytes;
}
static NvGpuDraw resident_guest;
static ID3D11ShaderResourceView *resident_color_view;
static std::vector<uint8_t> resident_color_snapshot, resident_depth_snapshot;
static std::vector<uint64_t> resident_color_offsets;
static uint64_t resident_color_hits, resident_depth_hits, resident_texture_hits;
static uint64_t resident_color_clears, resident_depth_clears;
static uint64_t surface_color_upload_bytes, surface_depth_upload_bytes;

static bool resident_surfaces_enabled()
{
    static const bool enabled = []()
    {
        const char *setting = std::getenv("RECOMP_GPU_RESIDENT_SURFACES");
        return !setting || std::strcmp(setting, "0") != 0;
    }();
    return enabled;
}

static bool same_surface_layout(const NvGpuDraw &a, const NvGpuDraw &b)
{
    return a.color == b.color && a.depth == b.depth &&
        a.width == b.width && a.height == b.height && a.pitch == b.pitch &&
        a.depth_stride == b.depth_stride && a.zscale == b.zscale &&
        a.surface_format == b.surface_format && a.surface_offset == b.surface_offset;
}

static void capture_surface_snapshot(std::vector<uint8_t> &snapshot, bool depth_image)
{
    if (!resident_surfaces_enabled()) return;
    const NvGpuDraw &guest = resident_guest;
    const void *source = depth_image ? (const void *)guest.depth : guest.color;
    if (!source) { snapshot.clear(); return; }
    size_t row_bytes = (size_t)guest.width * 4;
    snapshot.resize(row_bytes * guest.height);
    for (unsigned y = 0; y < guest.height; y++)
    {
        uint8_t *row = snapshot.data() + y * row_bytes;
        if (!depth_image && guest.surface_offset)
        {
            for (unsigned x = 0; x < guest.width; x++)
            {
                std::memcpy(row + x * 4, guest.color +
                    resident_color_offsets[(size_t)y * guest.width + x], 4);
            }
        }
        else
        {
            size_t stride = depth_image ? (size_t)guest.depth_stride * 4 : guest.pitch;
            std::memcpy(row, (const uint8_t *)source + y * stride, row_bytes);
        }
    }
}

static void set_resident_layout(const NvGpuDraw &guest)
{
    if (resident_surfaces_enabled() && guest.surface_offset &&
        (resident_color_offsets.size() != (size_t)guest.width * guest.height ||
         resident_guest.width != guest.width || resident_guest.height != guest.height ||
         resident_guest.surface_format != guest.surface_format ||
         resident_guest.surface_offset != guest.surface_offset))
    {
        resident_color_offsets.resize((size_t)guest.width * guest.height);
        for (unsigned y = 0; y < guest.height; y++)
            for (unsigned x = 0; x < guest.width; x++)
                resident_color_offsets[(size_t)y * guest.width + x] =
                    guest.surface_offset(guest.surface_format, x, y);
    }
    if (!guest.surface_offset) resident_color_offsets.clear();
    resident_guest = guest;
}

static bool surface_snapshot_matches(const std::vector<uint8_t> &snapshot,
    const NvGpuDraw &guest, bool depth_image, const std::vector<uint64_t> *offsets = nullptr)
{
    const void *source = depth_image ? (const void *)guest.depth : guest.color;
    size_t row_bytes = (size_t)guest.width * 4;
    if (!source || snapshot.size() != row_bytes * guest.height ||
        (!depth_image && guest.surface_offset && (!offsets || offsets->size() != (size_t)guest.width * guest.height))) return false;
    for (unsigned y = 0; y < guest.height; y++)
    {
        const uint8_t *row = snapshot.data() + y * row_bytes;
        if (!depth_image && guest.surface_offset)
        {
            for (unsigned x = 0; x < guest.width; x++)
            {
                if (std::memcmp(row + x * 4, guest.color +
                    (*offsets)[(size_t)y * guest.width + x], 4)) return false;
            }
        }
        else
        {
            size_t stride = depth_image ? (size_t)guest.depth_stride * 4 : guest.pitch;
            if (std::memcmp(row, (const uint8_t *)source + y * stride, row_bytes)) return false;
        }
    }
    return true;
}

/* Uniform CPU contents can be reproduced exactly with an attachment clear,
 * avoiding scaled image expansion and a full upload. Inspect every used pixel;
 * this is a data check, not a guess that the last guest operation was a clear. */
static bool uniform_surface_word(const NvGpuDraw &guest, bool depth_image, uint32_t *word)
{
    const uint8_t *source = depth_image ? (const uint8_t *)guest.depth : guest.color;
    if (!source) return false;
    uint64_t first = !depth_image && guest.surface_offset
        ? guest.surface_offset(guest.surface_format, 0, 0) : 0;
    std::memcpy(word, source + first, 4);
    __m128i expected = _mm_set1_epi32((int)*word);
    for (unsigned y = 0; y < guest.height; y++)
    {
        if (!depth_image && guest.surface_offset)
        {
            for (unsigned x = 0; x < guest.width; x++)
            {
                uint32_t value;
                std::memcpy(&value, source + guest.surface_offset(guest.surface_format, x, y), 4);
                if (value != *word) return false;
            }
        }
        else
        {
            size_t stride = depth_image ? (size_t)guest.depth_stride * 4 : guest.pitch;
            const uint8_t *row = source + y * stride;
            unsigned x = 0;
            for (; x + 4 <= guest.width; x += 4)
            {
                __m128i values = _mm_loadu_si128((const __m128i *)(row + x * 4));
                if (_mm_movemask_epi8(_mm_cmpeq_epi32(values, expected)) != 0xFFFF) return false;
            }
            for (; x < guest.width; x++)
            {
                uint32_t value; std::memcpy(&value, row + x * 4, 4);
                if (value != *word) return false;
            }
        }
    }
    return true;
}

static void release_target(TargetResources &target)
{
    if (target.color == preserved_color) preserved_color = nullptr;
    if (target.depth == preserved_depth) preserved_depth = nullptr;
    release(target.color_view);
    release(target.rtv); release(target.dsv);
    release(target.color); release(target.depth);
    release(target.color_read); release(target.depth_read);
}

static void select_surface_resources(const NvGpuDraw &guest,
    unsigned width, unsigned height, bool depth_change)
{
    bool resident = resident_surfaces_enabled();
    bool same = same_surface_layout(resident_guest, guest);
    if (color && depth && rtv && dsv && color_read && depth_read &&
        (target_width != width || target_height != height ||
         (resident && !same && !depth_change)))
    {
        TargetResources old = {};
        old.width = target_width; old.height = target_height;
        old.color = color; old.depth = depth;
        old.color_read = color_read; old.depth_read = depth_read;
        old.rtv = rtv; old.dsv = dsv; old.guest = resident_guest;
        old.color_view = resident_color_view; resident_color_view = nullptr;
        old.color_snapshot = std::move(resident_color_snapshot);
        old.depth_snapshot = std::move(resident_depth_snapshot);
        old.color_offsets = std::move(resident_color_offsets);
        color = depth = color_read = depth_read = nullptr;
        rtv = nullptr; dsv = nullptr;
        resident_guest = {};
        uint64_t bytes = surface_resource_bytes(old.width, old.height);
        while (!target_pool.empty() && (target_pool.size() >= 8 ||
            surface_pool_bytes() + bytes > surface_pool_budget))
        {
            release_target(target_pool.front());
            target_pool.erase(target_pool.begin());
        }
        if (bytes <= surface_pool_budget)
        {
            target_pool.push_back(std::move(old));
        }
        else
        {
            release_target(old);
        }
    }
    if (!color)
    {
        for (size_t i = 0; i < target_pool.size(); i++)
        {
            TargetResources &candidate = target_pool[i];
            if (candidate.width != width || candidate.height != height ||
                (resident && !same_surface_layout(candidate.guest, guest))) continue;
            target_width = width; target_height = height;
            color = candidate.color; depth = candidate.depth;
            color_read = candidate.color_read; depth_read = candidate.depth_read;
            rtv = candidate.rtv; dsv = candidate.dsv;
            resident_color_view = candidate.color_view;
            resident_guest = candidate.guest;
            resident_color_snapshot = std::move(candidate.color_snapshot);
            resident_depth_snapshot = std::move(candidate.depth_snapshot);
            resident_color_offsets = std::move(candidate.color_offsets);
            target_pool.erase(target_pool.begin() + i);
            break;
        }
    }
}
static uint64_t batches,triangles,readbacks,uploads;
static uint64_t prepare_ticks,submit_ticks,sync_ticks;
struct GpuTimer {
 uint64_t &total;const char *operation;LARGE_INTEGER start;
 GpuTimer(uint64_t &counter,const char *name):total(counter),operation(name){QueryPerformanceCounter(&start);}
 ~GpuTimer(){
  LARGE_INTEGER end;QueryPerformanceCounter(&end);
  uint64_t ticks=(uint64_t)(end.QuadPart-start.QuadPart);total+=ticks;
  static const double milliseconds_per_tick=[](){LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);return 1000.0/frequency.QuadPart;}();
  double ms=ticks*milliseconds_per_tick;
  if(ms>=50.0)std::fprintf(stderr,"[GPU-STALL] %s %.1f ms at %llu ms; batches=%llu readbacks=%llu uploads=%llu\n",operation,ms,(unsigned long long)GetTickCount64(),(unsigned long long)batches,(unsigned long long)readbacks,(unsigned long long)uploads);
 }
};
/* Sample GPU command duration without forcing completion. CPU Map timing
 * includes waits; timestamp queries distinguish shader work from those waits. */
struct GpuSample {ID3D11Query *disjoint,*begin,*end;unsigned operation;bool pending;};
struct GpuDuration {uint64_t samples;double total_ms,max_ms;};
static GpuSample gpu_samples[32];
static GpuDuration gpu_duration[3];
static uint64_t gpu_sample_calls[3];
static int gpu_timing_enabled=-1;
static void collect_gpu_samples(){
 if(!context)return;
 for(auto &sample:gpu_samples)if(sample.pending){
  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT clock={};UINT64 begin=0,end=0;
  if(context->GetData(sample.disjoint,&clock,sizeof(clock),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK ||
     context->GetData(sample.begin,&begin,sizeof(begin),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK ||
     context->GetData(sample.end,&end,sizeof(end),D3D11_ASYNC_GETDATA_DONOTFLUSH)!=S_OK)continue;
  if(!clock.Disjoint && clock.Frequency && end>=begin){
   double ms=(end-begin)*1000.0/clock.Frequency;auto &duration=gpu_duration[sample.operation];
   duration.samples++;duration.total_ms+=ms;duration.max_ms=std::max(duration.max_ms,ms);
  }
  sample.pending=false;
 }
}
static GpuSample *begin_gpu_sample(unsigned operation){
 if(gpu_timing_enabled<0)gpu_timing_enabled=getenv("RECOMP_PROFILE_GPU")!=nullptr;
 if(!gpu_timing_enabled || gpu_sample_calls[operation]++%128)return nullptr;
 collect_gpu_samples();
 for(auto &sample:gpu_samples)if(!sample.pending){
  if(!sample.disjoint){
   D3D11_QUERY_DESC desc={D3D11_QUERY_TIMESTAMP_DISJOINT,0};
   if(FAILED(device->CreateQuery(&desc,&sample.disjoint)))return nullptr;
   desc.Query=D3D11_QUERY_TIMESTAMP;
   if(FAILED(device->CreateQuery(&desc,&sample.begin)) || FAILED(device->CreateQuery(&desc,&sample.end))){release(sample.disjoint);release(sample.begin);release(sample.end);return nullptr;}
  }
  sample.operation=operation;sample.pending=true;
  context->Begin(sample.disjoint);context->End(sample.begin);return &sample;
 }
 return nullptr;
}
static void end_gpu_sample(GpuSample *sample){if(sample){context->End(sample->end);context->End(sample->disjoint);}}
static std::vector<NvGpuVertex> vertices;
static ID3D11Buffer *index_buffer;
static unsigned index_capacity, index_cursor;
static std::vector<NvGpuVertex> fallback_vertices;
static std::vector<uint32_t> vertex_indices;
static uint64_t expanded_vertex_bytes, uploaded_vertex_bytes, uploaded_index_bytes;
static uint64_t indexed_batches, reused_vertices;

static bool indexed_geometry;
static unsigned emitted_vertex_count;
int nv_gpu_indexed_upload_enabled(void)
{
    static const bool enabled = []()
    {
        const char *setting = std::getenv("RECOMP_GPU_INDEXED_UPLOAD");
        return !setting || std::strcmp(setting, "0") != 0;
    }();
    return enabled;
}

static bool check(HRESULT hr, const char *what);

static bool upload_indices(unsigned *first_index)
{
    if (!index_buffer || vertex_indices.size() > index_capacity)
    {
        release(index_buffer);
        index_capacity = std::max(32768u, (unsigned)vertex_indices.size());
        index_cursor = 0;
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = index_capacity * sizeof(uint32_t);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (!check(device->CreateBuffer(&desc, nullptr, &index_buffer), "indices"))
        {
            return false;
        }
    }
    if (index_cursor + vertex_indices.size() > index_capacity)
    {
        index_cursor = 0;
    }
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    D3D11_MAP mode = index_cursor ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
    if (!check(context->Map(index_buffer, 0, mode, 0, &mapped), "index upload"))
    {
        return false;
    }
    std::memcpy((uint32_t *)mapped.pData + index_cursor,
        vertex_indices.data(), vertex_indices.size() * sizeof(uint32_t));
    context->Unmap(index_buffer, 0);
    *first_index = index_cursor;
    index_cursor += (unsigned)vertex_indices.size();
    uploaded_index_bytes += vertex_indices.size() * sizeof(uint32_t);
    return true;
}

struct CachedTexture {uint64_t key,hash;unsigned w,h;ID3D11ShaderResourceView *srv;};
static std::vector<CachedTexture> textures;
static unsigned replace_texture;
template<class D,class T> struct CachedState {D desc;T *object;};
static std::vector<CachedState<D3D11_SAMPLER_DESC,ID3D11SamplerState>> sampler_states;
static std::vector<CachedState<D3D11_BLEND_DESC,ID3D11BlendState>> blend_states;
static std::vector<CachedState<D3D11_DEPTH_STENCIL_DESC,ID3D11DepthStencilState>> depth_states;
template<class D,class T,class Create>
static HRESULT state_cached(const D &desc,std::vector<CachedState<D,T>> &cache,T **output,Create create){
 for(auto &entry:cache)if(std::memcmp(&entry.desc,&desc,sizeof(D))==0){*output=entry.object;(*output)->AddRef();return S_OK;}
 T *object=nullptr;HRESULT hr=create(&desc,&object);if(FAILED(hr))return hr;
 if(cache.size()==128){release(cache.front().object);cache.erase(cache.begin());}
 CachedState<D,T> entry={desc,object};cache.push_back(entry);object->AddRef();*output=object;return S_OK;
}
template<class D,class T> static void clear_states(std::vector<CachedState<D,T>> &cache){for(auto &entry:cache)release(entry.object);cache.clear();}
static void cache_texture(const CachedTexture &entry){
 /* Animated render-target textures change their hash every frame. Replace
  * their previous contents instead of evicting unrelated static textures.
  * Draw bindings own separate references, so replacing a cache entry is safe. */
 for(auto &old:textures)if(old.key==entry.key){release(old.srv);old=entry;return;}
 if(textures.size()<128)textures.push_back(entry);
 else{auto &old=textures[replace_texture++%textures.size()];release(old.srv);old=entry;}
}
static const char shader[]=R"(
cbuffer Params:register(b0) {float4 viewport;float4 fogColor;uint4 flags;uint4 alpha;uint4 program[8];float4 factor0[8];float4 factor1[8];float4 finalC0;float4 finalC1;uint4 control;};
Texture2D image:register(t0);SamplerState sampleState:register(s0);
Texture2D image1:register(t1);SamplerState sample1:register(s1);
Texture2D image2:register(t2);SamplerState sample2:register(s2);
Texture2D image3:register(t3);SamplerState sample3:register(s3);
struct In {float4 p:POSITION;float4 c:COLOR0;float4 d:COLOR1;float4 t:TEXCOORD0;float4 t1:TEXCOORD1;float4 t2:TEXCOORD2;float4 t3:TEXCOORD3;float f:FOG;};
struct Out {float4 p:SV_POSITION;noperspective float4 c:COLOR0;noperspective float4 d:COLOR1;float4 t:TEXCOORD0;float4 t1:TEXCOORD1;float4 t2:TEXCOORD2;float4 t3:TEXCOORD3;noperspective float f:FOG;};
Out vertex(In v) {Out o;float w=v.p.w;o.p=float4((v.p.x*2/viewport.x-1)*w,(1-v.p.y*2/viewport.y)*w,v.p.z/viewport.z*w,w);o.c=v.c;o.d=v.d;o.t=v.t;o.t1=v.t1;o.t2=v.t2;o.t3=v.t3;o.f=v.f;return o;}
float4 stage(uint s,float4 t){uint mode=(control.w>>(s*5))&31;
 if(!mode)return float4(0,0,0,1);if(mode==4)return saturate(t);
 if(mode==5){[unroll]for(uint k=0;k<4;k++){bool ge=((flags.z>>(s*4+k))&1)!=0;if(ge?t[k]>=0:t[k]<0)discard;}return 0;}
 if(mode>2)return 0;if(!(flags.y&(1<<s)))return 1;
 float2 uv=t.xy/(t.w!=0?t.w:1);
 if(s==0)return image.Sample(sampleState,uv);if(s==1)return image1.Sample(sample1,uv);if(s==2)return image2.Sample(sample2,uv);return image3.Sample(sample3,uv);
}
float inputMap(float x,uint code){float p=max(x,0);switch((code>>5)&7){case 0:return p;case 1:return 1-saturate(x);case 2:return 2*p-1;case 3:return -2*p+1;case 4:return p-.5;case 5:return -p+.5;case 6:return x;default:return -x;}}
float outputMap(float x,uint code){switch(code){case 8:return x-.5;case 16:return x*2;case 24:return (x-.5)*2;case 32:return x*4;case 48:return x*.5;default:return x;}}
float3 rgb(float4 R[16],uint code){float4 v=R[code&15];return float3(inputMap((code&16)?v.a:v.r,code),inputMap((code&16)?v.a:v.g,code),inputMap((code&16)?v.a:v.b,code));}
float ainput(float4 R[16],uint code){float4 v=R[code&15];return inputMap((code&16)?v.a:v.b,code);}
float3 mapped(float3 x,uint f){return clamp(float3(outputMap(x.x,f&56),outputMap(x.y,f&56),outputMap(x.z,f&56)),-1,1);}
float4 combine(Out v,float4 texel,float4 t1,float4 t2,float4 t3){
 float4 R[16];[unroll]for(uint j=0;j<16;j++)R[j]=0;
 R[3]=float4(fogColor.rgb,v.f);R[4]=v.c;R[5]=v.d;R[8]=texel;R[9]=t1;R[10]=t2;R[11]=t3;R[12].a=(control.w&31)?texel.a:1;
 uint count=min(control.x&255,8),opts=control.x>>8;
 [loop]for(uint s=0;s<count;s++){
  uint icw=program[s].x,aicw=program[s].y,ocw=program[s].z,aocw=program[s].w,fl=ocw>>12,afl=aocw>>12;
  R[1]=factor0[(opts&16)?s:0];R[2]=factor1[(opts&256)?s:0];
  bool mux=(opts&1)?R[12].a>=.5:(((int)(R[12].a*255)&1)!=0);
  float3 A=rgb(R,icw>>24),B=rgb(R,icw>>16),C=rgb(R,icw>>8),D=rgb(R,icw);
  float3 pab=(fl&2)?dot(A,B).xxx:A*B,pcd=(fl&1)?dot(C,D).xxx:C*D;
  float3 sum=(fl&4)?(mux?pcd:pab):pab+pcd;
  float3 ab=mapped(pab,fl),cd=mapped(pcd,fl),ms=mapped(sum,fl);
  float ap=ainput(R,aicw>>24)*ainput(R,aicw>>16),cp=ainput(R,aicw>>8)*ainput(R,aicw),asum=(afl&4)?(mux?cp:ap):ap+cp;
  float aa=clamp(outputMap(ap,afl&56),-1,1),ca=clamp(outputMap(cp,afl&56),-1,1),ma=clamp(outputMap(asum,afl&56),-1,1);
  uint da=(ocw>>4)&15,dc=ocw&15,dm=(ocw>>8)&15;
  if(da){R[da].rgb=ab;if(fl&128)R[da].a=ab.b;}if(dc){R[dc].rgb=cd;if(fl&64)R[dc].a=cd.b;}if(dm)R[dm].rgb=ms;
  da=(aocw>>4)&15;dc=aocw&15;dm=(aocw>>8)&15;if(da)R[da].a=aa;if(dc)R[dc].a=ca;if(dm)R[dm].a=ma;R[0]=0;
 }
 if(control.y||control.z){uint f=control.z&255;R[1]=finalC0;R[2]=finalC1;R[14].rgb=((f&64)?1-R[5].rgb:R[5].rgb)+((f&32)?1-R[12].rgb:R[12].rgb);if(f&128)R[14].rgb=saturate(R[14].rgb);R[15].rgb=rgb(R,control.z>>24)*rgb(R,control.z>>16);float3 A=rgb(R,control.y>>24),B=rgb(R,control.y>>16),C=rgb(R,control.y>>8),D=rgb(R,control.y);return saturate(float4(D+C*(1-A)+B*A,ainput(R,control.z>>8)));}
 return saturate(R[12]);
}
float4 pixel(Out v):SV_TARGET {
 float4 texel=stage(0,v.t),t1=stage(1,v.t1),t2=stage(2,v.t2),t3=stage(3,v.t3);
 float4 c;
 if(!flags.x)c=combine(v,texel,t1,t2,t3);
 else {c=max(v.c,0);if(!(flags.x&2))c=clamp(c*max(texel,0),-1,1);else c=clamp(c,-1,1);
 if(flags.x&4)c.rgb=max(fogColor.rgb,0)*(1-max(v.f,0))+c.rgb*max(v.f,0);}
 c=saturate(c);
 if(alpha.x) {uint a=(uint)(c.a*255+0.5),r=alpha.z;bool accepted=true;
 switch(alpha.y){case 512:accepted=false;break;case 513:accepted=a<r;break;case 514:accepted=a==r;break;case 515:accepted=a<=r;break;case 516:accepted=a>r;break;case 517:accepted=a!=r;break;case 518:accepted=a>=r;break;}
 if(!accepted)discard;}
 return saturate(c);
})";
static bool check(HRESULT hr,const char *what){if(SUCCEEDED(hr))return true;std::fprintf(stderr,"[GPU-D3D11] %s failed: %08lX\n",what,(unsigned long)hr);return false;}
int nv_gpu_enabled(void){const char *e=std::getenv("RECOMP_GPU");return e&&std::strcmp(e,"0")!=0;}
static bool init(){
 if(initialized)return !failed;initialized=true;
 D3D_FEATURE_LEVEL level;
 if(!check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"hardware device"))return !(failed=true);
 IDXGIDevice *dxgi=nullptr;IDXGIAdapter *adapter=nullptr;
 if(SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice),(void**)&dxgi)) && SUCCEEDED(dxgi->GetAdapter(&adapter))){
  DXGI_ADAPTER_DESC desc={};if(SUCCEEDED(adapter->GetDesc(&desc)))std::fprintf(stderr,"[GPU-ADAPTER] %ls vendor=%04X device=%04X dedicated=%.0f MiB\n",desc.Description,desc.VendorId,desc.DeviceId,desc.DedicatedVideoMemory/1048576.0);
 }
 release(adapter);release(dxgi);
 ID3DBlob *v=nullptr,*p=nullptr,*error=nullptr;
 HRESULT hr=D3DCompile(shader,sizeof(shader)-1,"nv2a_gpu",nullptr,nullptr,"vertex","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&v,&error);
 if(error){std::fprintf(stderr,"[GPU-D3D11] %s\n",(char*)error->GetBufferPointer());release(error);}
 if(!check(hr,"vertex compile"))return !(failed=true);
 hr=D3DCompile(shader,sizeof(shader)-1,"nv2a_gpu",nullptr,nullptr,"pixel","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&p,&error);
 if(error){std::fprintf(stderr,"[GPU-D3D11] %s\n",(char*)error->GetBufferPointer());release(error);}
 if(!check(hr,"pixel compile")){release(v);return !(failed=true);}
 bool ok=check(device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs),"vertex shader")&&check(device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps),"pixel shader");
 ID3D11ShaderReflection *general_reflection=nullptr;
 /* MinGW lacks the __uuidof specialization for shader reflection. The SDK
  * IID names the same interface and also works with the MSVC headers. */
 if(SUCCEEDED(D3DReflect(p->GetBufferPointer(),p->GetBufferSize(),IID_ID3D11ShaderReflection,(void**)&general_reflection))){
  D3D11_SHADER_DESC stats={};general_reflection->GetDesc(&stats);
  std::fprintf(stderr,"[GPU-SHADER] general instructions=%u temporary-registers=%u\n",stats.InstructionCount,stats.TempRegisterCount);release(general_reflection);
 }
 /* A runtime uniform branch retains the general combiner interpreter in
  * every shader binary. Constant modes let the compiler remove it entirely. */
 const char *simple_option=getenv("RECOMP_GPU_SIMPLE_SHADERS");
 if(!simple_option || strcmp(simple_option,"0"))for(unsigned mode:{1u,2u,5u,6u}){
  std::string code(shader);size_t at=0;std::string literal=std::to_string(mode)+"u";
  while((at=code.find("flags.x",at))!=std::string::npos){code.replace(at,7,literal);at+=literal.size();}
  ID3DBlob *compiled=nullptr,*errors=nullptr;
  HRESULT result=D3DCompile(code.data(),code.size(),"simple_pixel",nullptr,nullptr,"pixel","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&compiled,&errors);
  if(errors){std::fprintf(stderr,"[GPU-D3D11] %s\n",(char*)errors->GetBufferPointer());release(errors);}
  if(SUCCEEDED(result) && SUCCEEDED(device->CreatePixelShader(compiled->GetBufferPointer(),compiled->GetBufferSize(),nullptr,&simple_ps[mode]))){
   ID3D11ShaderReflection *reflection=nullptr;
   if(SUCCEEDED(D3DReflect(compiled->GetBufferPointer(),compiled->GetBufferSize(),IID_ID3D11ShaderReflection,(void**)&reflection))){
    D3D11_SHADER_DESC stats={};reflection->GetDesc(&stats);
    std::fprintf(stderr,"[GPU-SHADER] simple mode=%u instructions=%u temporary-registers=%u\n",mode,stats.InstructionCount,stats.TempRegisterCount);release(reflection);
   }
  }
  release(compiled);release(errors);
 }
 D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,48,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,64,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",2,DXGI_FORMAT_R32G32B32A32_FLOAT,0,80,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",3,DXGI_FORMAT_R32G32B32A32_FLOAT,0,96,D3D11_INPUT_PER_VERTEX_DATA,0},{"FOG",0,DXGI_FORMAT_R32_FLOAT,0,112,D3D11_INPUT_PER_VERTEX_DATA,0}};
 ok=ok&&check(device->CreateInputLayout(elements,8,v->GetBufferPointer(),v->GetBufferSize(),&layout),"input layout");release(v);release(p);
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=496;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
 ok=ok&&check(device->CreateBuffer(&bd,nullptr,&cb),"constants");
 D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.ScissorEnable=TRUE;rd.DepthClipEnable=FALSE;
 ok=ok&&check(device->CreateRasterizerState(&rd,&raster),"raster state");
 failed=!ok;std::fprintf(stderr,"[GPU-D3D11] hardware raster initialized (feature level %X)\n",level);return ok;
}
static void read_depth_rows(const float *source,unsigned stride,const NvGpuDraw &s){
 const __m128 one=_mm_set1_ps(1),scale=_mm_set1_ps(s.zscale);
 for(unsigned y=0;y<s.height;y++){
  const float *row=source+(size_t)y*stride;
  float *out=s.depth+(size_t)y*s.depth_stride;unsigned x=0;
  for(;x+4<=s.width;x+=4){
   __m128 z=_mm_loadu_ps(row+x),old=_mm_loadu_ps(out+x);
   __m128 replace=_mm_or_ps(_mm_cmpneq_ps(z,one),_mm_cmple_ps(old,scale));
   _mm_storeu_ps(out+x,_mm_or_ps(_mm_and_ps(replace,_mm_mul_ps(z,scale)),_mm_andnot_ps(replace,old)));
  }
  for(;x<s.width;x++)if(row[x]!=1.0f||out[x]<=s.zscale)out[x]=row[x]*s.zscale;
 }
}
/* Preserve the physical raster before compatibility resolution. Validate the
 * resolved guest pixels at flip so later CPU/FMV writes never display stale GPU content. */
static std::vector<uint32_t> present_pixels,present_native;
static uint8_t *present_guest;
static unsigned present_width,present_height,present_pitch;
static ID3D11Texture2D *present_gpu,*present_gpu_read;
static bool present_pixels_current;
static uint64_t deferred_color_readbacks,materialized_color_readbacks,materialized_color_bytes,materialized_color_ticks;
static bool preserve_color_gpu(){
 D3D11_TEXTURE2D_DESC existing={};if(present_gpu)present_gpu->GetDesc(&existing);
 if(!present_gpu || existing.Width!=target_width || existing.Height!=target_height){
  release(present_gpu);release(present_gpu_read);
  D3D11_TEXTURE2D_DESC desc={};color->GetDesc(&desc);
  desc.BindFlags=0;desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=0;
  if(!check(device->CreateTexture2D(&desc,nullptr,&present_gpu),"preserved GPU color"))return false;
  desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  if(!check(device->CreateTexture2D(&desc,nullptr,&present_gpu_read),"preserved color staging")){release(present_gpu);return false;}
 }
 context->CopyResource(present_gpu,color);present_pixels_current=false;
 return true;
}
static bool materialize_present_pixels(){
 if(present_pixels_current)return true;
 if(!present_gpu || !present_gpu_read)return false;
 LARGE_INTEGER begin,end;QueryPerformanceCounter(&begin);
 context->CopyResource(present_gpu_read,present_gpu);
 D3D11_MAPPED_SUBRESOURCE map={};
 if(!check(context->Map(present_gpu_read,0,D3D11_MAP_READ,0,&map),"preserved color readback"))return false;
 present_pixels.resize((size_t)present_width*present_height);
 for(unsigned y=0;y<present_height;y++)std::memcpy(present_pixels.data()+(size_t)y*present_width,(char*)map.pData+(size_t)y*map.RowPitch,present_width*4);
 context->Unmap(present_gpu_read,0);present_pixels_current=true;
 QueryPerformanceCounter(&end);materialized_color_readbacks++;materialized_color_bytes+=(uint64_t)present_width*present_height*4;materialized_color_ticks+=end.QuadPart-begin.QuadPart;return true;
}
static void preserve_native_snapshot(){
 present_native.resize(640*480);
 for(unsigned y=0;y<480;y++)std::memcpy(present_native.data()+y*640,active.color+(size_t)y*active.pitch,640*4);
 present_width=target_width;present_height=target_height;present_pitch=active.pitch;present_guest=active.color;preserved_color=color;
}

const uint32_t *nv_gpu_present_image(const uint8_t *guest,unsigned pitch,unsigned *width,unsigned *height){
 if(!xbox_DisplayInternalScaling() || guest!=present_guest || pitch!=present_pitch || present_native.size()!=640*480)return nullptr;
 for(unsigned y=0;y<480;y++)if(std::memcmp(guest+(size_t)y*pitch,present_native.data()+y*640,640*4))return nullptr;
 if(!materialize_present_pixels())return nullptr;
 *width=present_width;*height=present_height;return present_pixels.data();
}
static ID3D11ComputeShader *resolve_shader;
static ID3D11Buffer *resolve_constants;
static ID3D11Texture2D *resolve_output,*resolve_read,*resolve_source;
static ID3D11ShaderResourceView *resolve_srv;
static ID3D11UnorderedAccessView *resolve_uav;
static unsigned resolve_width,resolve_height;
static uint64_t gpu_resolves;
static const char resolve_code[]=R"(
cbuffer Resolve:register(b0){uint4 dimensions;float4 scale;};
Texture2D<float4> source:register(t0);
RWTexture2D<uint> output:register(u0);
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=dimensions.x || id.y>=dimensions.y)return;
 uint2 lo=(uint2)(float2(id.xy)*scale.xy);
 uint2 hi=min(dimensions.zw,max(lo+1,(uint2)(float2(id.xy+1)*scale.xy)));
 uint4 sum=0;
 for(uint y=lo.y;y<hi.y;y++)for(uint x=lo.x;x<hi.x;x++)
  sum+=(uint4)(source.Load(int3(x,y,0))*255.0+0.5);
 uint count=(hi.x-lo.x)*(hi.y-lo.y);
 uint4 color=(sum+count/2)/count;
 output[id.xy]=color.b|(color.g<<8)|(color.r<<16)|(color.a<<24);
})";
static bool dispatch_resolve(){
 if(active.surface_offset || active.width!=640 || active.height!=480 ||
    (render_scale_x==1 && render_scale_y==1))return false;
 const char *option=getenv("RECOMP_GPU_RESOLVE");if(option && !strcmp(option,"0"))return false;
 if(!resolve_shader){
  ID3DBlob *code=nullptr,*errors=nullptr;
  HRESULT result=D3DCompile(resolve_code,sizeof(resolve_code)-1,"compatibility_resolve",nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors);
  if(errors){std::fprintf(stderr,"[GPU-D3D11] %s\n",(char*)errors->GetBufferPointer());release(errors);}
  if(!check(result,"resolve compile"))return false;
  bool ok=check(device->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&resolve_shader),"resolve shader");release(code);if(!ok)return false;
 }
 if(!resolve_constants){
  D3D11_BUFFER_DESC desc={};desc.ByteWidth=32;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(!check(device->CreateBuffer(&desc,nullptr,&resolve_constants),"resolve constants"))return false;
 }
 if(resolve_source!=color){
  release(resolve_srv);resolve_source=nullptr;
  if(!check(device->CreateShaderResourceView(color,nullptr,&resolve_srv),"resolve source"))return false;
  resolve_source=color;
 }
 if(!resolve_output || resolve_width!=active.width || resolve_height!=active.height){
  release(resolve_uav);release(resolve_output);release(resolve_read);
  D3D11_TEXTURE2D_DESC desc={};desc.Width=active.width;desc.Height=active.height;desc.MipLevels=desc.ArraySize=1;
  desc.Format=DXGI_FORMAT_R32_UINT;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
  if(!check(device->CreateTexture2D(&desc,nullptr,&resolve_output),"resolve output"))return false;
  if(!check(device->CreateUnorderedAccessView(resolve_output,nullptr,&resolve_uav),"resolve output view"))return false;
  desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  if(!check(device->CreateTexture2D(&desc,nullptr,&resolve_read),"resolve staging"))return false;
  resolve_width=active.width;resolve_height=active.height;
 }
 struct {unsigned dimensions[4];float scale[4];} constants={{active.width,active.height,target_width,target_height},{render_scale_x,render_scale_y,0,0}};
 context->UpdateSubresource(resolve_constants,0,nullptr,&constants,0,0);
 context->CSSetShader(resolve_shader,nullptr,0);context->CSSetConstantBuffers(0,1,&resolve_constants);
 context->CSSetShaderResources(0,1,&resolve_srv);context->CSSetUnorderedAccessViews(0,1,&resolve_uav,nullptr);
 GpuSample *gpu_sample=begin_gpu_sample(1);
 context->Dispatch((active.width+7)/8,(active.height+7)/8,1);end_gpu_sample(gpu_sample);
 ID3D11ShaderResourceView *no_srv=nullptr;ID3D11UnorderedAccessView *no_uav=nullptr;
 context->CSSetShaderResources(0,1,&no_srv);context->CSSetUnorderedAccessViews(0,1,&no_uav,nullptr);
 context->CSSetShader(nullptr,nullptr,0);context->CopyResource(resolve_read,resolve_output);gpu_resolves++;
 return true;
}
static ID3D11ComputeShader *depth_resolve_shader;
static ID3D11Buffer *depth_resolve_constants;
static ID3D11Texture2D *depth_resolve_output,*depth_resolve_read,*depth_resolve_source;
static ID3D11ShaderResourceView *depth_resolve_srv;
static ID3D11UnorderedAccessView *depth_resolve_uav;
static uint64_t gpu_depth_resolves;
static const char depth_resolve_code[]=R"(
cbuffer Resolve:register(b0){uint4 dimensions;float4 scale;};
Texture2D<float> source:register(t0);
RWTexture2D<float> output:register(u0);
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=dimensions.x || id.y>=dimensions.y)return;
 uint2 position=min(dimensions.zw-1,(uint2)((float2(id.xy)+0.5)*scale.xy));
 output[id.xy]=source.Load(int3(position,0));
})";
static bool dispatch_depth_resolve(){
 if(!active.depth || !depth_dirty || active.width!=640 || active.height!=480 ||
    (render_scale_x==1 && render_scale_y==1))return false;
 const char *option=getenv("RECOMP_GPU_DEPTH_RESOLVE");if(option && !strcmp(option,"0"))return false;
 if(!depth_resolve_shader){
  ID3DBlob *code=nullptr,*errors=nullptr;
  HRESULT result=D3DCompile(depth_resolve_code,sizeof(depth_resolve_code)-1,"depth_resolve",nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors);
  if(errors){std::fprintf(stderr,"[GPU-D3D11] %s\n",(char*)errors->GetBufferPointer());release(errors);}
  if(!check(result,"depth resolve compile"))return false;
  bool ok=check(device->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&depth_resolve_shader),"depth resolve shader");release(code);if(!ok)return false;
 }
 if(!depth_resolve_constants){
  D3D11_BUFFER_DESC desc={};desc.ByteWidth=32;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(!check(device->CreateBuffer(&desc,nullptr,&depth_resolve_constants),"depth resolve constants"))return false;
 }
 if(depth_resolve_source!=depth){
  release(depth_resolve_srv);depth_resolve_source=nullptr;
  D3D11_SHADER_RESOURCE_VIEW_DESC desc={};desc.Format=DXGI_FORMAT_R32_FLOAT;desc.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;desc.Texture2D.MipLevels=1;
  if(!check(device->CreateShaderResourceView(depth,&desc,&depth_resolve_srv),"depth resolve source"))return false;
  depth_resolve_source=depth;
 }
 if(!depth_resolve_output){
  D3D11_TEXTURE2D_DESC desc={};desc.Width=640;desc.Height=480;desc.MipLevels=desc.ArraySize=1;
  desc.Format=DXGI_FORMAT_R32_FLOAT;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
  if(!check(device->CreateTexture2D(&desc,nullptr,&depth_resolve_output),"depth resolve output"))return false;
  if(!check(device->CreateUnorderedAccessView(depth_resolve_output,nullptr,&depth_resolve_uav),"depth resolve output view"))return false;
  desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  if(!check(device->CreateTexture2D(&desc,nullptr,&depth_resolve_read),"depth resolve staging"))return false;
 }
 struct {unsigned dimensions[4];float scale[4];} constants={{640,480,target_width,target_height},{render_scale_x,render_scale_y,0,0}};
 context->UpdateSubresource(depth_resolve_constants,0,nullptr,&constants,0,0);
 context->CSSetShader(depth_resolve_shader,nullptr,0);context->CSSetConstantBuffers(0,1,&depth_resolve_constants);
 context->CSSetShaderResources(0,1,&depth_resolve_srv);context->CSSetUnorderedAccessViews(0,1,&depth_resolve_uav,nullptr);
 GpuSample *gpu_sample=begin_gpu_sample(2);
 context->Dispatch(80,60,1);end_gpu_sample(gpu_sample);
 ID3D11ShaderResourceView *no_srv=nullptr;ID3D11UnorderedAccessView *no_uav=nullptr;
 context->CSSetShaderResources(0,1,&no_srv);context->CSSetUnorderedAccessViews(0,1,&no_uav,nullptr);
 context->CSSetShader(nullptr,nullptr,0);context->CopyResource(depth_resolve_read,depth_resolve_output);gpu_depth_resolves++;
 return true;
}
static uint32_t average_channels(__m128i sums,unsigned count){
 __m128i values=_mm_unpacklo_epi16(sums,_mm_setzero_si128());
 values=_mm_add_epi32(values,_mm_set1_epi32(count/2));
 /* Inputs are small nonnegative integers: float division is exact enough
  * to preserve the scalar integer quotient, including exact boundaries. */
 __m128i average=_mm_cvttps_epi32(_mm_div_ps(_mm_cvtepi32_ps(values),_mm_set1_ps((float)count)));
 __m128i shorts=_mm_packs_epi32(average,_mm_setzero_si128());
 return (uint32_t)_mm_cvtsi128_si32(_mm_packus_epi16(shorts,_mm_setzero_si128()));
}
static void read_scaled_depth(const float *source,unsigned stride,const NvGpuDraw &guest,unsigned width,unsigned height,float sx,float sy){
 static std::vector<unsigned> columns;
 static unsigned cached_width,cached_guest_width;static float cached_scale;
 if(cached_width!=width || cached_guest_width!=guest.width || cached_scale!=sx){
  columns.resize(guest.width);
  for(unsigned x=0;x<guest.width;x++)columns[x]=std::min(width-1,(unsigned)((x+.5f)*sx));
  cached_width=width;cached_guest_width=guest.width;cached_scale=sx;
 }
 const __m128 one=_mm_set1_ps(1),scale=_mm_set1_ps(guest.zscale);
 for(unsigned y=0;y<guest.height;y++){
  unsigned yy=std::min(height-1,(unsigned)((y+.5f)*sy));
  const float *row=source+(size_t)yy*stride;float *out=guest.depth+(size_t)y*guest.depth_stride;
  unsigned x=0;
  for(;x+4<=guest.width;x+=4){
   __m128 z=_mm_set_ps(row[columns[x+3]],row[columns[x+2]],row[columns[x+1]],row[columns[x]]);
   __m128 old=_mm_loadu_ps(out+x);
   __m128 replace=_mm_or_ps(_mm_cmpneq_ps(z,one),_mm_cmple_ps(old,scale));
   _mm_storeu_ps(out+x,_mm_or_ps(_mm_and_ps(replace,_mm_mul_ps(z,scale)),_mm_andnot_ps(replace,old)));
  }
  for(;x<guest.width;x++){float value=row[columns[x]];if(value!=1 || out[x]<=guest.zscale)out[x]=value*guest.zscale;}
 }
}
/* The caller will fully overwrite these guest buffers after synchronization.
 * Preserve unrelated attachments and never discard a partial color clear. */
static uint64_t discarded_color,discarded_depth;
static uint64_t gpu_color_clears, gpu_depth_clears;
static const uint8_t *mirrored_clear_color;
static uint32_t mirrored_clear_value;

int nv_gpu_clear_attachments(const NvGpuClear *clear)
{
    if (!clear || !context || !active.color || (!clear->color && !clear->depth))
    {
        return 0;
    }
    if (clear->color && (clear->color != active.color || clear->pitch != active.pitch ||
        clear->format != active.surface_format || clear->x || clear->y ||
        clear->width < active.width || clear->height < active.height))
    {
        return 0;
    }
    if (clear->depth && (clear->depth != active.depth || !dsv || !(active.zscale > 0) ||
        !std::isfinite(clear->depth_value)))
    {
        return 0;
    }
    if (clear->color && !rtv)
    {
        return 0;
    }

    /* The CPU clear immediately following this call makes both representations
     * agree. Reading the discarded attachment back first would only copy bytes
     * that the caller is about to overwrite. Keep the other attachment dirty. */
    if (clear->color)
    {
        const uint32_t value = clear->color_value;
        const float rgba[4] = {((value >> 16) & 255) / 255.f, ((value >> 8) & 255) / 255.f,
                                (value & 255) / 255.f, (value >> 24) / 255.f};
        context->ClearRenderTargetView(rtv, rgba);
        resident_color_snapshot.clear();
        mirrored_clear_color = clear->color;
        mirrored_clear_value = value;
        color_dirty = false;
        if (present_guest == clear->color)
        {
            present_guest = nullptr;
        }
        if (preserved_color == color)
        {
            preserved_color = nullptr;
        }
        gpu_color_clears++;
    }
    if (clear->depth)
    {
        float value = std::min(1.f, std::max(0.f, clear->depth_value / active.zscale));
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, value, 0);
        resident_depth_snapshot.clear();
        depth_dirty = false;
        if (preserved_depth == depth)
        {
            preserved_depth = nullptr;
        }
        gpu_depth_clears++;
    }
    dirty = color_dirty || depth_dirty;
    return 1;
}

static void validate_mirrored_clear(const NvGpuDraw &next)
{
    if (!mirrored_clear_color)
    {
        return;
    }
    const uint8_t *guest = mirrored_clear_color;
    mirrored_clear_color = nullptr;
    if (next.color != guest || active.color != guest)
    {
        return; /* A target switch already reloads the next CPU image. */
    }
    for (unsigned y = 0; y < active.height; y++)
    {
        for (unsigned x = 0; x < active.width; x++)
        {
            uint64_t offset = active.surface_offset
                ? active.surface_offset(active.surface_format, x, y)
                : (uint64_t)y * active.pitch + x * 4;
            uint32_t value;
            std::memcpy(&value, guest + offset, sizeof value);
            if (value != mirrored_clear_value)
            {
                /* The title changed pixels after its CPU clear. Retain any
                 * pending depth, then let target() reload the edited color. */
                nv_gpu_sync_depth("CPU edit after mirrored color clear");
                active.color = nullptr;
                return;
            }
        }
    }
}

void nv_gpu_discard_overwritten(const uint8_t *guest_color,unsigned pitch,unsigned format,unsigned x,unsigned y,unsigned width,unsigned height,const float *guest_depth){
 if(!dirty)return;
 if(guest_color && active.color==guest_color && active.pitch==pitch &&
    active.surface_format==format && x==0 && y==0 && width>=active.width && height>=active.height){
  if(color_dirty)discarded_color++;
  color_dirty=false;
  if(present_guest==guest_color)present_guest=nullptr;
  if(preserved_color==color)preserved_color=nullptr;
 }
 if(guest_depth && active.depth==guest_depth){
  if(depth_dirty)discarded_depth++;
  depth_dirty=false;
  if(preserved_depth==depth)preserved_depth=nullptr;
 }
 dirty=color_dirty || depth_dirty;
}
/* Keep native-sized offscreen results as GPU textures. Guest-byte snapshots
 * validate every reuse so CPU writes and recycled guest addresses invalidate it.
 * Larger/scaled targets and incompatible formats retain ordinary decoding. */
struct RenderedTexture {
 const uint8_t *guest;unsigned width,height,pitch;bool swizzled;
 ID3D11Texture2D *image;ID3D11ShaderResourceView *view;
 std::vector<uint8_t> native;
};
static std::vector<RenderedTexture> rendered_textures;
static unsigned rendered_replacement;
static uint64_t rendered_texture_copies,rendered_texture_hits;
static int rendered_textures_enabled=-1;
static bool use_rendered_textures(){
 if(rendered_textures_enabled<0){const char *flag=getenv("RECOMP_GPU_RENDERED_TEXTURES");rendered_textures_enabled=!flag || strcmp(flag,"0");}
 return rendered_textures_enabled!=0;
}
static void preserve_rendered_texture(){
 /* Resident resources can be sampled directly after the destination is bound.
  * Keep legacy GPU copies only in the dimension-pool comparison path. */
 if(resident_surfaces_enabled())return;
 if(!use_rendered_textures() || active.width>512 || active.height>512 ||
    target_width!=active.width || target_height!=active.height)return;
 bool swizzled=active.surface_offset!=nullptr;
 size_t bytes=swizzled?(size_t)active.width*active.height*4:(size_t)active.pitch*active.height;
 if(!bytes || bytes>2*1024*1024)return;
 RenderedTexture *entry=nullptr;
 for(auto &candidate:rendered_textures)if(candidate.guest==active.color && candidate.width==active.width && candidate.height==active.height && candidate.pitch==active.pitch && candidate.swizzled==swizzled){entry=&candidate;break;}
 if(!entry){
  if(rendered_textures.size()<8){rendered_textures.push_back({});entry=&rendered_textures.back();}
  else {entry=&rendered_textures[rendered_replacement++%8];release(entry->view);release(entry->image);entry->native.clear();}
  entry->guest=active.color;entry->width=active.width;entry->height=active.height;entry->pitch=active.pitch;entry->swizzled=swizzled;
 }
 if(!entry->image){
  D3D11_TEXTURE2D_DESC desc={};color->GetDesc(&desc);desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=0;
  if(FAILED(device->CreateTexture2D(&desc,nullptr,&entry->image)))return;
  if(FAILED(device->CreateShaderResourceView(entry->image,nullptr,&entry->view))){release(entry->image);return;}
 }
 context->CopyResource(entry->image,color);
 entry->native.assign(active.color,active.color+bytes);rendered_texture_copies++;
}
static ID3D11ShaderResourceView *rendered_texture_view(const uint8_t *guest,unsigned width,unsigned height,unsigned format,unsigned pitch){
 if(!guest || !use_rendered_textures() || (format!=0x06 && format!=0x12))return nullptr;
 bool swizzled=format==0x06;
 if (resident_surfaces_enabled())
 {
     /* Only inactive attachments may be sampled. The destination switch has
      * already synchronized/retired the source; the context is still owned
      * by this title thread. Scaled images require a native texture resolve
      * and intentionally retain decoding until that API exists. */
     for (auto &candidate : target_pool)
     {
         const NvGpuDraw &source = candidate.guest;
         if (source.color != guest || source.width != width || source.height != height ||
             candidate.width != width || candidate.height != height ||
             (source.surface_offset != nullptr) != swizzled || (!swizzled && source.pitch != pitch) ||
             !surface_snapshot_matches(candidate.color_snapshot, source, false, &candidate.color_offsets)) continue;
         if (!candidate.color_view &&
             !check(device->CreateShaderResourceView(candidate.color, nullptr, &candidate.color_view),
                 "resident surface texture view")) return nullptr;
         candidate.color_view->AddRef();
         resident_texture_hits++;
         rendered_texture_hits++;
         return candidate.color_view;
     }
 }
 for(auto &entry:rendered_textures)if(entry.guest==guest && entry.width==width && entry.height==height && entry.swizzled==swizzled && (swizzled || entry.pitch==pitch) && entry.view){
  if(entry.native.empty() || memcmp(entry.native.data(),guest,entry.native.size()))return nullptr;
  entry.view->AddRef();rendered_texture_hits++;return entry.view;
 }
 return nullptr;
}
struct SyncAudit {
 const char *reason;
 unsigned width,height,scaled_width,scaled_height;
 uint64_t calls,color_bytes,depth_bytes,ticks;
};
static std::vector<SyncAudit> sync_audit;
static int sync_audit_enabled=-1;
static bool sync_depth_only;
void nv_gpu_sync(void){nv_gpu_sync_reason("explicit boundary");}
void nv_gpu_prepare_depth_change(const uint8_t *next_color){
 if(active.color==next_color)nv_gpu_sync_depth("depth surface change (depth only)");
 else nv_gpu_sync_reason("color and depth surface change");
}
void nv_gpu_sync_depth(const char *reason){
 if(!dirty || !depth_dirty || !active.depth)return;
 sync_depth_only=true;nv_gpu_sync_reason(reason);sync_depth_only=false;
}
void nv_gpu_sync_reason(const char *reason){
 if(!dirty||!context){active.color=nullptr;return;}
 GpuTimer timer(sync_ticks,"readback");
 if(sync_audit_enabled<0)sync_audit_enabled=getenv("RECOMP_PROFILE_GPU")!=nullptr;
 LARGE_INTEGER audit_start={};
 unsigned audit_width=active.width,audit_height=active.height;
 uint64_t audit_color=0,audit_depth=0;
 if(sync_audit_enabled)QueryPerformanceCounter(&audit_start);
 readbacks++;
 bound_draw.valid = false;
 context->OMSetRenderTargets(0,nullptr,nullptr);
 bool resolved=color_dirty && !sync_depth_only && dispatch_resolve();
 bool depth_resolved=dispatch_depth_resolve();
 bool deferred_high=resolved && preserve_color_gpu();
 if(deferred_high){
  D3D11_MAPPED_SUBRESOURCE native_map={};
  if(check(context->Map(resolve_read,0,D3D11_MAP_READ,0,&native_map),"native color readback")){
   for(unsigned y=0;y<active.height;y++)std::memcpy(active.color+(size_t)y*active.pitch,(char*)native_map.pData+(size_t)y*native_map.RowPitch,active.width*4);
   context->Unmap(resolve_read,0);preserve_native_snapshot();capture_surface_snapshot(resident_color_snapshot, false);deferred_color_readbacks++;
  }else deferred_high=false;
 }
 if(color_dirty && !sync_depth_only && !deferred_high){context->CopyResource(color_read,color);
  audit_color=(uint64_t)target_width*target_height*4;
  if(resolved)audit_color+=(uint64_t)active.width*active.height*4;
 }
 if(deferred_high)audit_color=(uint64_t)active.width*active.height*4;
 if(active.depth && depth_dirty)audit_depth=(uint64_t)(depth_resolved?active.width:target_width)*(depth_resolved?active.height:target_height)*4;
 if(active.depth && depth_dirty && !depth_resolved)context->CopyResource(depth_read,depth);
 D3D11_MAPPED_SUBRESOURCE map;
 if(color_dirty && !sync_depth_only && !deferred_high && check(context->Map(color_read,0,D3D11_MAP_READ,0,&map),"color readback")){
  D3D11_MAPPED_SUBRESOURCE native_map={};
  bool native_ready=resolved && check(context->Map(resolve_read,0,D3D11_MAP_READ,0,&native_map),"resolved color readback");
  if(native_ready){
   for(unsigned y=0;y<active.height;y++)std::memcpy(active.color+(size_t)y*active.pitch,(char*)native_map.pData+(size_t)y*native_map.RowPitch,active.width*4);
   context->Unmap(resolve_read,0);
  }else if(render_scale_x!=1 || render_scale_y!=1) {
   for(unsigned y=0;y<active.height;y++)for(unsigned x=0;x<active.width;x++){
    unsigned x0=(unsigned)(x*render_scale_x),y0=(unsigned)(y*render_scale_y);
    unsigned x1=std::min(target_width,std::max(x0+1,(unsigned)((x+1)*render_scale_x)));
    unsigned y1=std::min(target_height,std::max(y0+1,(unsigned)((y+1)*render_scale_y)));
    __m128i sums=_mm_setzero_si128();unsigned n=(x1-x0)*(y1-y0);
    for(unsigned yy=y0;yy<y1;yy++){
     const uint32_t *row=(const uint32_t*)((const char*)map.pData+(size_t)yy*map.RowPitch);
     if(x1-x0==3){
      uint64_t pair;std::memcpy(&pair,row+x0,8);
      __m128i pixels=_mm_unpacklo_epi8(_mm_cvtsi64_si128((long long)pair),_mm_setzero_si128());
      __m128i third=_mm_unpacklo_epi8(_mm_cvtsi32_si128((int)row[x0+2]),_mm_setzero_si128());
      sums=_mm_add_epi16(sums,_mm_add_epi16(_mm_add_epi16(pixels,_mm_srli_si128(pixels,8)),third));
     }else for(unsigned xx=x0;xx<x1;xx++){
      __m128i pixel=_mm_cvtsi32_si128((int)row[xx]);
      sums=_mm_add_epi16(sums,_mm_unpacklo_epi8(pixel,_mm_setzero_si128()));
     }
    }
    uint32_t pixel=average_channels(sums,n);
    uint64_t offset=active.surface_offset?active.surface_offset(active.surface_format,x,y):(uint64_t)y*active.pitch+x*4;
    std::memcpy(active.color+offset,&pixel,4);
   }
  } else for(unsigned y=0;y<active.height;y++){
   if(active.surface_offset)for(unsigned x=0;x<active.width;x++)
    std::memcpy(active.color+active.surface_offset(active.surface_format,x,y),(char*)map.pData+y*map.RowPitch+x*4,4);
   else std::memcpy(active.color+y*active.pitch,(char*)map.pData+y*map.RowPitch,active.width*4);
  }
  if(active.width==640 && active.height==480 && !active.surface_offset){
   present_guest=nullptr;
   if(render_scale_x!=1 || render_scale_y!=1){
    present_pixels.resize((size_t)target_width*target_height);
    present_native.resize(640*480);
    for(unsigned y=0;y<target_height;y++)std::memcpy(present_pixels.data()+(size_t)y*target_width,(char*)map.pData+(size_t)y*map.RowPitch,target_width*4);
    for(unsigned y=0;y<480;y++)std::memcpy(present_native.data()+y*640,active.color+(size_t)y*active.pitch,640*4);
    present_width=target_width;present_height=target_height;present_pitch=active.pitch;present_guest=active.color;preserved_color=color;present_pixels_current=true;
   }
  }
  context->Unmap(color_read,0);
  capture_surface_snapshot(resident_color_snapshot, false);
  preserve_rendered_texture();
 }
 if(active.depth && depth_dirty && check(context->Map(depth_resolved?depth_resolve_read:depth_read,0,D3D11_MAP_READ,0,&map),"depth readback")){
  depth_readbacks++;
  if(depth_resolved || (render_scale_x==1 && render_scale_y==1))read_depth_rows((const float*)map.pData,map.RowPitch/sizeof(float),active);
  else read_scaled_depth((const float*)map.pData,map.RowPitch/sizeof(float),active,target_width,target_height,render_scale_x,render_scale_y);
  if(active.width==640 && active.height==480 && !active.surface_offset && (render_scale_x!=1 || render_scale_y!=1)){
   preserved_native_depth.resize(640*480);
   for(unsigned y=0;y<480;y++)std::memcpy(preserved_native_depth.data()+y*640,active.depth+(size_t)y*active.depth_stride,640*sizeof(float));
   preserved_depth=depth;preserved_guest_depth=active.depth;preserved_depth_stride=active.depth_stride;
   preserved_depth_scale=active.zscale;preserved_depth_width=target_width;preserved_depth_height=target_height;
  }
  context->Unmap(depth_resolved?depth_resolve_read:depth_read,0);
  capture_surface_snapshot(resident_depth_snapshot, true);
 }
 if(active.depth && !depth_dirty)skipped_depth_readbacks++;
 if(sync_audit_enabled){
  LARGE_INTEGER end;QueryPerformanceCounter(&end);
  SyncAudit *entry=nullptr;
  for(auto &row:sync_audit)if(!strcmp(row.reason,reason) && row.width==audit_width && row.height==audit_height && row.scaled_width==target_width && row.scaled_height==target_height){entry=&row;break;}
  if(!entry && sync_audit.size()<64){sync_audit.push_back({reason,audit_width,audit_height,target_width,target_height,0,0,0,0});entry=&sync_audit.back();}
  if(entry){entry->calls++;entry->color_bytes+=audit_color;entry->depth_bytes+=audit_depth;entry->ticks+=end.QuadPart-audit_start.QuadPart;}
 }
 depth_dirty=false;
 if(sync_depth_only)dirty=color_dirty;
 else {color_dirty=false;dirty=false;active.color=nullptr;}
}
/* Convert directly into the staging rows; retain the original divide and
 * clamping semantics, including NaNs, without a temporary copy per upload. */
static void upload_depth_rows(float *output,unsigned stride,const NvGpuDraw &s){
 const __m128 zero=_mm_setzero_ps(),one=_mm_set1_ps(1),scale=_mm_set1_ps(s.zscale);
 for(unsigned y=0;y<s.height;y++){
  float *row=output+(size_t)y*stride;unsigned x=0;
  if(s.depth){
   const float *source=s.depth+(size_t)y*s.depth_stride;
   for(;x+4<=s.width;x+=4){
    __m128 z=_mm_div_ps(_mm_loadu_ps(source+x),scale);
    _mm_storeu_ps(row+x,_mm_min_ps(one,_mm_max_ps(z,zero)));
   }
   for(;x<s.width;x++)row[x]=std::min(1.0f,std::max(0.0f,source[x]/s.zscale));
  }else{
   for(;x+4<=s.width;x+=4)_mm_storeu_ps(row+x,one);
   for(;x<s.width;x++)row[x]=1;
  }
 }
}
/* Normalize at guest resolution before expanding. Repeated physical rows
 * can be copied, avoiding millions of divisions and an intermediate depth image. */
static void upload_scaled_depth(float *output,unsigned stride,const NvGpuDraw &guest,unsigned width,unsigned height){
 static std::vector<float> normalized;
 static std::vector<unsigned> columns;
 static unsigned cached_width,cached_guest_width;
 normalized.resize((size_t)guest.width*guest.height);
 upload_depth_rows(normalized.data(),guest.width,guest);
 if(cached_width!=width || cached_guest_width!=guest.width){
  columns.resize(width);for(unsigned x=0;x<width;x++)columns[x]=(unsigned)((uint64_t)x*guest.width/width);
  cached_width=width;cached_guest_width=guest.width;
 }
 unsigned previous=~0u;
 for(unsigned y=0;y<height;y++){
  unsigned gy=(unsigned)((uint64_t)y*guest.height/height);
  float *row=output+(size_t)y*stride;
  if(y && gy==previous)std::memcpy(row,output+(size_t)(y-1)*stride,width*sizeof(float));
  else {
   const float *source=normalized.data()+(size_t)gy*guest.width;
   for(unsigned x=0;x<width;x++)row[x]=source[columns[x]];
  }
  previous=gy;
 }
}
static std::vector<float> depth_upload;
static bool target(const NvGpuDraw &guest){
 unsigned width=guest.width,height=guest.height;
 if(xbox_DisplayInternalScaling() && guest.width==640 && guest.height<=480){
  unsigned w,h;xbox_DisplayResolutionSize(xbox_DisplayResolutionSelected(),&w,&h);
  width=w;height=std::max(1u,(unsigned)((uint64_t)guest.height*h/480));
 }
 float sx=(float)width/guest.width,sy=(float)height/guest.height;
 if(active.color==guest.color&&active.width==guest.width&&active.height>=guest.height&&active.depth==guest.depth&&active.pitch==guest.pitch&&active.zscale==guest.zscale&&active.surface_format==guest.surface_format&&active.surface_offset==guest.surface_offset&&render_scale_x==sx&&render_scale_y==sy)return true;
 bool depth_change=active.color==guest.color && active.width==guest.width && active.height==guest.height &&
    active.pitch==guest.pitch && active.surface_format==guest.surface_format && active.surface_offset==guest.surface_offset &&
    target_width==width && target_height==height && render_scale_x==sx && render_scale_y==sy;
 if(depth_change)nv_gpu_sync_depth("depth attachment change");
 else nv_gpu_sync_reason("render target switch");
 select_surface_resources(guest, width, height, depth_change);
 bool resident_match = resident_surfaces_enabled() && color && depth &&
     target_width == width && target_height == height && same_surface_layout(resident_guest, guest);
 bool resident_color = resident_match && surface_snapshot_matches(resident_color_snapshot, guest, false, &resident_color_offsets);
 bool resident_depth = resident_match && surface_snapshot_matches(resident_depth_snapshot, guest, true);
 bool keep_color=depth_change || resident_color;
 if (resident_color) resident_color_hits++;
 if (resident_depth) resident_depth_hits++;
 if(!keep_color && color==preserved_color && color && guest.color==present_guest && guest.pitch==present_pitch &&
    !guest.surface_offset && guest.width==640 && guest.height==480 && width==present_width && height==present_height){
  keep_color=true;
  for(unsigned y=0;y<480;y++)if(std::memcmp(guest.color+(size_t)y*guest.pitch,present_native.data()+y*640,640*4)){keep_color=false;break;}
 }
 bool keep_depth=resident_depth;
 if(depth==preserved_depth && depth && guest.depth && guest.depth==preserved_guest_depth &&
    guest.depth_stride==preserved_depth_stride && guest.zscale==preserved_depth_scale &&
    guest.width==640 && guest.height==480 && width==preserved_depth_width && height==preserved_depth_height){
  keep_depth=true;
  for(unsigned y=0;y<480;y++)if(std::memcmp(guest.depth+(size_t)y*guest.depth_stride,preserved_native_depth.data()+y*640,640*sizeof(float))){keep_depth=false;break;}
 }
 bool uniform_color = false, uniform_depth = false;
 if (resident_surfaces_enabled() && color && depth && rtv && dsv && color_read && depth_read &&
     target_width == width && target_height == height)
 {
     uint32_t word = 0;
     if (!keep_color && uniform_surface_word(guest, false, &word))
     {
         const float rgba[4] = {((word >> 16) & 255) / 255.f, ((word >> 8) & 255) / 255.f,
             (word & 255) / 255.f, (word >> 24) / 255.f};
         context->ClearRenderTargetView(rtv, rgba);
         if (preserved_color == color) preserved_color = nullptr;
         if (present_guest == guest.color) present_guest = nullptr;
         keep_color = uniform_color = true;
         resident_color_clears++;
     }
     if (!keep_depth && (!guest.depth || uniform_surface_word(guest, true, &word)))
     {
         float value = 1.f;
         if (guest.depth) std::memcpy(&value, &word, 4);
         if ((!guest.depth || (guest.zscale > 0 && std::isfinite(guest.zscale))) && std::isfinite(value))
         {
             float normalized = guest.depth ? std::min(1.f, std::max(0.f, value / guest.zscale)) : 1.f;
             context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, normalized, 0);
             if (preserved_depth == depth) preserved_depth = nullptr;
             keep_depth = uniform_depth = true;
             resident_depth_clears++;
         }
     }
 }
 NvGpuDraw s=guest;
 if(!keep_color && (width!=guest.width || height!=guest.height)) {
  scaled_color.resize((size_t)width*height);
  bool retained=guest.color==present_guest && guest.pitch==present_pitch &&
      !guest.surface_offset && guest.width==640 && guest.height==480 &&
      width==present_width && height==present_height && present_native.size()==640*480;
  if(retained && !keep_color && !materialize_present_pixels())retained=false;
  if(retained && !keep_color){
   static std::vector<unsigned> patch_x,patch_y;
   static unsigned patch_width,patch_height;
   if(patch_width!=width || patch_height!=height){
    patch_x.resize(641);patch_y.resize(481);
    for(unsigned x=0;x<=640;x++)patch_x[x]=(unsigned)(((uint64_t)x*width+639)/640);
    for(unsigned y=0;y<=480;y++)patch_y[y]=(unsigned)(((uint64_t)y*height+479)/480);
    patch_width=width;patch_height=height;
   }
   std::memcpy(scaled_color.data(),present_pixels.data(),(size_t)width*height*4);
   /* Most readbacks are unchanged. Compare at native resolution and patch
    * only CPU-modified pixels instead of rebuilding every physical pixel. */
   for(unsigned gy=0;gy<guest.height;gy++)for(unsigned gx=0;gx<guest.width;gx++){
    uint32_t pixel;std::memcpy(&pixel,guest.color+(size_t)gy*guest.pitch+gx*4,4);
    if(pixel==present_native[(size_t)gy*640+gx])continue;
    unsigned x0=patch_x[gx];
    unsigned x1=patch_x[gx+1];
    unsigned y0=patch_y[gy];
    unsigned y1=patch_y[gy+1];
    for(unsigned y=y0;y<y1;y++)std::fill(scaled_color.data()+(size_t)y*width+x0,scaled_color.data()+(size_t)y*width+x1,pixel);
   }
  }
  /* Compute nearest-source coordinates once per column, rather than doing
   * two integer divisions for every physical pixel on every upload. */
  static std::vector<unsigned> source_x;
  static unsigned map_width,map_guest_width;
  if(map_width!=width || map_guest_width!=guest.width){
   source_x.resize(width);for(unsigned x=0;x<width;x++)source_x[x]=(unsigned)((uint64_t)x*guest.width/width);
   map_width=width;map_guest_width=guest.width;
  }
  for(unsigned y=0;y<height;y++){
   unsigned gy=(unsigned)((uint64_t)y*guest.height/height);
   uint32_t *out=scaled_color.data()+(size_t)y*width;

   if(!retained)for(unsigned x=0;x<width;x++){
    unsigned gx=source_x[x];
    uint64_t offset=guest.surface_offset?guest.surface_offset(guest.surface_format,gx,gy):(uint64_t)gy*guest.pitch+gx*4;
    std::memcpy(out+x,guest.color+offset,4);
   }

  }
  s.width=width;s.height=height;s.pitch=width*4;s.color=(uint8_t*)scaled_color.data();
  s.surface_offset=nullptr;
 }
 if(keep_color){s.width=width;s.height=height;s.pitch=width*4;s.surface_offset=nullptr;}
 std::vector<uint32_t> linear;
 const void *source=s.color;unsigned source_pitch=s.pitch;
 if(s.surface_offset){
  linear.resize(s.width*s.height);
  for(unsigned y=0;y<s.height;y++)for(unsigned x=0;x<s.width;x++)
   std::memcpy(&linear[y*s.width+x],s.color+s.surface_offset(s.surface_format,x,y),4);
  source=linear.data();source_pitch=s.width*4;
 }
 if(color&&depth&&rtv&&dsv&&color_read&&depth_read&&target_width==s.width&&target_height==s.height){
  if(keep_color)preserved_uploads++;
  else {context->UpdateSubresource(color,0,nullptr,source,source_pitch,0);if(color==preserved_color)preserved_color=nullptr;}
  /* D3D11 forbids UpdateSubresource on depth-stencil resources. Upload
   * through the reusable staging texture, then copy to the depth target. */
  if(keep_depth)preserved_depth_uploads++;
  else {
  D3D11_MAPPED_SUBRESOURCE map;
  if(!check(context->Map(depth_read,0,D3D11_MAP_WRITE,0,&map),"depth upload"))return false;
  if(width!=guest.width || height!=guest.height)upload_scaled_depth((float*)map.pData,map.RowPitch/sizeof(float),guest,width,height);
  else upload_depth_rows((float*)map.pData,map.RowPitch/sizeof(float),s);
  context->Unmap(depth_read,0);context->CopyResource(depth,depth_read);
   if(depth==preserved_depth)preserved_depth=nullptr;
  }
  set_resident_layout(guest);
  if (uniform_color) capture_surface_snapshot(resident_color_snapshot, false);
  if (uniform_depth) capture_surface_snapshot(resident_depth_snapshot, true);
  if (!keep_color)
  {
      surface_color_upload_bytes += (uint64_t)width * height * 4;
      capture_surface_snapshot(resident_color_snapshot, false);
  }
  if (!keep_depth)
  {
      surface_depth_upload_bytes += (uint64_t)width * height * 4;
      capture_surface_snapshot(resident_depth_snapshot, true);
  }
  active=guest;render_scale_x=sx;render_scale_y=sy;return true;
 }
 release(resident_color_view);release(rtv);release(dsv);release(color);release(depth);release(color_read);release(depth_read);
 D3D11_TEXTURE2D_DESC td={};td.Width=s.width;td.Height=s.height;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA input={source,source_pitch,0};
 if(!check(device->CreateTexture2D(&td,&input,&color),"color target")||!check(device->CreateRenderTargetView(color,nullptr,&rtv),"color view"))return false;
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 if(!check(device->CreateTexture2D(&td,nullptr,&color_read),"color staging"))return false;
 depth_upload.resize((size_t)s.width*s.height);
 if(width!=guest.width || height!=guest.height)upload_scaled_depth(depth_upload.data(),s.width,guest,width,height);
 else upload_depth_rows(depth_upload.data(),s.width,s);
 td.Format=DXGI_FORMAT_R32_TYPELESS;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;td.CPUAccessFlags=0;
 input.pSysMem=depth_upload.data();input.SysMemPitch=s.width*4;
 D3D11_DEPTH_STENCIL_VIEW_DESC depth_view={};depth_view.Format=DXGI_FORMAT_D32_FLOAT;depth_view.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;
 if(!check(device->CreateTexture2D(&td,&input,&depth),"depth target")||!check(device->CreateDepthStencilView(depth,&depth_view,&dsv),"depth view"))return false;
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ|D3D11_CPU_ACCESS_WRITE;
 if(!check(device->CreateTexture2D(&td,nullptr,&depth_read),"depth staging"))return false;
 target_width=s.width;target_height=s.height;
 set_resident_layout(guest);
 surface_color_upload_bytes += (uint64_t)width * height * 4;
 surface_depth_upload_bytes += (uint64_t)width * height * 4;
 capture_surface_snapshot(resident_color_snapshot, false);
 capture_surface_snapshot(resident_depth_snapshot, true);
 active=guest;render_scale_x=sx;render_scale_y=sy;return true;
}
static D3D11_BLEND factor(unsigned f){switch(f){case 0:return D3D11_BLEND_ZERO;case 1:return D3D11_BLEND_ONE;case 0x300:return D3D11_BLEND_SRC_COLOR;case 0x301:return D3D11_BLEND_INV_SRC_COLOR;case 0x302:return D3D11_BLEND_SRC_ALPHA;case 0x303:return D3D11_BLEND_INV_SRC_ALPHA;case 0x304:return D3D11_BLEND_DEST_ALPHA;case 0x305:return D3D11_BLEND_INV_DEST_ALPHA;case 0x306:return D3D11_BLEND_DEST_COLOR;case 0x307:return D3D11_BLEND_INV_DEST_COLOR;default:return D3D11_BLEND_ONE;}}
static D3D11_BLEND alpha_factor(D3D11_BLEND f){if(f==D3D11_BLEND_SRC_COLOR)return D3D11_BLEND_SRC_ALPHA;if(f==D3D11_BLEND_INV_SRC_COLOR)return D3D11_BLEND_INV_SRC_ALPHA;if(f==D3D11_BLEND_DEST_COLOR)return D3D11_BLEND_DEST_ALPHA;if(f==D3D11_BLEND_INV_DEST_COLOR)return D3D11_BLEND_INV_DEST_ALPHA;return f;}
/* Match the currently verified software address modes. */
static D3D11_TEXTURE_ADDRESS_MODE address(unsigned a){return a==1?D3D11_TEXTURE_ADDRESS_WRAP:D3D11_TEXTURE_ADDRESS_CLAMP;}
void nv_gpu_texture_barrier(const uint8_t *data,uint64_t bytes){
 if(active.color&&dirty){uintptr_t a=(uintptr_t)active.color,b=(uintptr_t)data;
  uint64_t size=active.surface_offset?(uint64_t)active.width*active.height*4:(uint64_t)active.pitch*active.height;
  if(b<a+size&&a<b+bytes)nv_gpu_sync_reason("render target sampled as texture");}
}
static ID3D11ShaderResourceView *extra_view(const NvGpuTexture &s){
 if(auto *resident=rendered_texture_view(s.guest,s.width,s.height,s.format,s.pitch))return resident;
 for(auto &t:textures)if(t.key==s.key&&t.hash==s.hash&&t.w==s.width&&t.h==s.height){t.srv->AddRef();return t.srv;}
 if(!s.width||!s.height||s.width>4096||s.height>4096||!s.decode)return nullptr;
 std::vector<uint32_t> pixels(s.width*s.height);
 for(unsigned y=0;y<s.height;y++)for(unsigned x=0;x<s.width;x++)if(!s.decode(s.arg,x,y,&pixels[y*s.width+x]))return nullptr;
 D3D11_TEXTURE2D_DESC td={};td.Width=s.width;td.Height=s.height;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA in={pixels.data(),s.width*4,0};ID3D11Texture2D *tex=nullptr;ID3D11ShaderResourceView *view=nullptr;
 if(!check(device->CreateTexture2D(&td,&in,&tex),"extra texture"))return nullptr;
 bool ok=check(device->CreateShaderResourceView(tex,nullptr,&view),"extra texture view");release(tex);if(!ok)return nullptr;
 CachedTexture entry={s.key,s.hash,s.width,s.height,view};uploads++;
 cache_texture(entry);
 view->AddRef();return view;
}
int nv_gpu_begin(const NvGpuDraw *state){
 if(!nv_gpu_enabled()||!init())return 0;
 GpuTimer timer(prepare_ticks,"prepare");
 const NvGpuDraw &s=*state;
 if(!s.width||!s.height||s.width>1024||s.height>1024||s.src>0x307||s.dst>0x307)return 0;
 validate_mirrored_clear(s);
 if(!target(s))return 0;draw=s;vertices.clear();vertex_indices.clear();indexed_geometry=false;emitted_vertex_count=0;release(texture);
 for(unsigned i=0;i<3;i++){release(extra_texture[i]);release(extra_sampler[i]);}
 if(s.textured){
  texture=rendered_texture_view(s.texture_guest,s.tw,s.th,s.texture_format,s.texture_pitch);
  bool resident=texture!=nullptr;
  if(!resident)for(auto &t:textures)if(t.key==s.texture_key&&t.hash==s.texture_hash&&t.w==s.tw&&t.h==s.th){texture=t.srv;break;}
  if(!texture){
   std::vector<uint32_t> pixels(s.tw*s.th);
   for(unsigned y=0;y<s.th;y++)for(unsigned x=0;x<s.tw;x++)if(!s.decode(s.decode_arg,x,y,&pixels[y*s.tw+x]))return 0;
   D3D11_TEXTURE2D_DESC td={};td.Width=s.tw;td.Height=s.th;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
   D3D11_SUBRESOURCE_DATA in={pixels.data(),s.tw*4,0};ID3D11Texture2D *tex=nullptr;
   if(!check(device->CreateTexture2D(&td,&in,&tex),"texture"))return 0;
   bool ok=check(device->CreateShaderResourceView(tex,nullptr,&texture),"texture view");release(tex);if(!ok)return 0;
   CachedTexture entry={s.texture_key,s.texture_hash,s.tw,s.th,texture};
   uploads++;
   cache_texture(entry);
  }
  if(!resident)texture->AddRef();
 }
 release(sampler);D3D11_SAMPLER_DESC sd={};sd.Filter=s.linear?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=address(s.addr_u);sd.AddressV=address(s.addr_v);sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=0;
 auto make_sampler=[](const D3D11_SAMPLER_DESC *desc,ID3D11SamplerState **out){return device->CreateSamplerState(desc,out);};
 if(!check(state_cached(sd,sampler_states,&sampler,make_sampler),"sampler"))return 0;
 for(unsigned i=0;i<3;i++)if(s.extra[i].valid){
  extra_texture[i]=extra_view(s.extra[i]);if(!extra_texture[i])return 0;
  D3D11_SAMPLER_DESC extra=sd;extra.Filter=s.extra[i].linear?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;extra.AddressU=address(s.extra[i].addr_u);extra.AddressV=address(s.extra[i].addr_v);
  if(!check(state_cached(extra,sampler_states,&extra_sampler[i],make_sampler),"extra sampler"))return 0;
 }
 release(blend);D3D11_BLEND_DESC bd={};auto &b=bd.RenderTarget[0];b.BlendEnable=s.blend;b.SrcBlend=factor(s.src);b.DestBlend=factor(s.dst);b.SrcBlendAlpha=alpha_factor(b.SrcBlend);b.DestBlendAlpha=alpha_factor(b.DestBlend);
 b.BlendOp=s.equation==0x800A?D3D11_BLEND_OP_SUBTRACT:s.equation==0x800B?D3D11_BLEND_OP_REV_SUBTRACT:s.equation==0x8007?D3D11_BLEND_OP_MIN:s.equation==0x8008?D3D11_BLEND_OP_MAX:D3D11_BLEND_OP_ADD;b.BlendOpAlpha=b.BlendOp;
 b.RenderTargetWriteMask=((s.mask&0x10000)?1:0)|((s.mask&0x100)?2:0)|((s.mask&1)?4:0)|((s.mask&0x1000000)?8:0);
 if(!check(state_cached(bd,blend_states,&blend,[](const D3D11_BLEND_DESC *desc,ID3D11BlendState **out){return device->CreateBlendState(desc,out);}),"blend"))return 0;
 release(ds);D3D11_DEPTH_STENCIL_DESC dd={};dd.DepthEnable=s.depth_test||s.depth_write;dd.DepthWriteMask=s.depth_write?D3D11_DEPTH_WRITE_MASK_ALL:D3D11_DEPTH_WRITE_MASK_ZERO;dd.DepthFunc=s.depth_test?(D3D11_COMPARISON_FUNC)(s.depth_func-0x200+1):D3D11_COMPARISON_ALWAYS;
 if(!check(state_cached(dd,depth_states,&ds,[](const D3D11_DEPTH_STENCIL_DESC *desc,ID3D11DepthStencilState **out){return device->CreateDepthStencilState(desc,out);}),"depth state"))return 0;
 return 1;
}
unsigned nv_gpu_append_vertex(const NvGpuVertex *vertex)
{
    unsigned index = (unsigned)vertices.size();
    vertices.push_back(*vertex);
    return index;
}

void nv_gpu_indexed_triangle(const unsigned indices[3])
{
    vertex_indices.insert(vertex_indices.end(), indices, indices + 3);
    emitted_vertex_count += 3;
    indexed_geometry = true;
}

void nv_gpu_triangle(const NvGpuVertex *v)
{
    unsigned first = (unsigned)vertices.size();
    vertices.insert(vertices.end(), v, v + 3);
    emitted_vertex_count += 3;
    if (nv_gpu_indexed_upload_enabled())
    {
        for (unsigned i = 0; i < 3; i++) vertex_indices.push_back(first + i);
    }
}
void nv_gpu_end(void){
 if(vertices.empty())return;
 GpuTimer timer(submit_ticks,"submit");
 bool indexed = indexed_geometry;
 unsigned first_index = 0;
 if (indexed && !upload_indices(&first_index))
 {
     /* Preserve the draw if the optional index-buffer allocation/map fails. */
     fallback_vertices.clear();
     for (uint32_t index : vertex_indices) fallback_vertices.push_back(vertices[index]);
     indexed = false;
 }
 const std::vector<NvGpuVertex> &upload_vertices = indexed_geometry && !indexed ? fallback_vertices : vertices;
 if(upload_vertices.size()>capacity){release(vb);capacity=std::max(32768u,(unsigned)upload_vertices.size());vertex_cursor=0;D3D11_BUFFER_DESC bd={};bd.ByteWidth=capacity*sizeof(NvGpuVertex);bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;if(!check(device->CreateBuffer(&bd,nullptr,&vb),"vertices"))return;}
 if(vertex_cursor+upload_vertices.size()>capacity)vertex_cursor=0;
 unsigned first_vertex=vertex_cursor;
 D3D11_MAP upload=first_vertex?D3D11_MAP_WRITE_NO_OVERWRITE:D3D11_MAP_WRITE_DISCARD;
 D3D11_MAPPED_SUBRESOURCE map;if(!check(context->Map(vb,0,upload,0,&map),"vertex upload"))return;
 std::memcpy((char*)map.pData+first_vertex*sizeof(NvGpuVertex),upload_vertices.data(),upload_vertices.size()*sizeof(NvGpuVertex));context->Unmap(vb,0);
 vertex_cursor+=(unsigned)upload_vertices.size();
 expanded_vertex_bytes += emitted_vertex_count * sizeof(NvGpuVertex);
 uploaded_vertex_bytes += upload_vertices.size() * sizeof(NvGpuVertex);
 if (indexed)
 {
     indexed_batches++;
     reused_vertices += emitted_vertex_count - vertices.size();
 }
 struct Constants{float viewport[4],fog[4];unsigned flags[4],alpha[4],program[8][4];float factor0[8][4],factor1[8][4],final0[4],final1[4];unsigned control[4];} c={};
 static_assert(sizeof(c)==496,"HLSL constant layout");
 c.viewport[0]=(float)draw.width;c.viewport[1]=(float)draw.height;c.viewport[2]=draw.zscale;std::memcpy(c.fog,draw.fog_color,16);c.flags[0]=draw.mode;c.flags[1]=draw.textured;c.alpha[0]=draw.alpha_test;c.alpha[1]=draw.alpha_func;c.alpha[2]=draw.alpha_ref;
 for(unsigned i=0;i<3;i++)if(draw.extra[i].valid)c.flags[1]|=1u<<(i+1);c.flags[2]=draw.clip_plane_mode;
 for(unsigned i=0;i<8;i++){c.program[i][0]=draw.combiner.color_icw[i];c.program[i][1]=draw.combiner.alpha_icw[i];c.program[i][2]=draw.combiner.color_ocw[i];c.program[i][3]=draw.combiner.alpha_ocw[i];nv2a_rc_unpack_fast(draw.combiner.factor0[i],c.factor0[i]);nv2a_rc_unpack_fast(draw.combiner.factor1[i],c.factor1[i]);}
 nv2a_rc_unpack_fast(draw.combiner.final_c0,c.final0);nv2a_rc_unpack_fast(draw.combiner.final_c1,c.final1);c.control[0]=draw.combiner.control;c.control[1]=draw.combiner.final0;c.control[2]=draw.combiner.final1;c.control[3]=draw.combiner.stage_program;
 if (!submission_cache_enabled() || !constants_current || std::memcmp(uploaded_constants, &c, sizeof(c)))
 {
     context->UpdateSubresource(cb, 0, nullptr, &c, 0, 0);
     std::memcpy(uploaded_constants, &c, sizeof(c));
     constants_current = true;
     constant_uploads++;
 }
 else
 {
     skipped_constant_uploads++;
 }
 ID3D11PixelShader *pixel_shader = draw.mode < 8 && simple_ps[draw.mode] ? simple_ps[draw.mode] : ps;
 if (pixel_shader != ps) simple_shader_draws++;
 bind_draw_pipeline(pixel_shader);
 GpuSample *gpu_sample=begin_gpu_sample(0);if (indexed)
 {
     context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);
     context->DrawIndexed((UINT)vertex_indices.size(), first_index, (INT)first_vertex);
 }
 else
 {
     context->Draw((UINT)upload_vertices.size(), first_vertex);
 }end_gpu_sample(gpu_sample);if (draw.mask & 0x01010101u) resident_color_snapshot.clear();
 if (draw.depth_write) resident_depth_snapshot.clear();
 color_dirty=color_dirty || (draw.mask&0x01010101u)!=0;depth_dirty=depth_dirty || draw.depth_write;dirty=color_dirty || depth_dirty; batches++;triangles+=emitted_vertex_count/3;
}
void nv_gpu_report(void){if(nv_gpu_enabled()){
 std::fprintf(stderr, "[GPU-RESIDENT-SURFACES] color hits=%llu depth hits=%llu texture hits=%llu GPU clears color=%llu depth=%llu uploads color=%.2f MiB depth=%.2f MiB inactive pairs=%zu\n", (unsigned long long)resident_color_hits, (unsigned long long)resident_depth_hits, (unsigned long long)resident_texture_hits, (unsigned long long)resident_color_clears, (unsigned long long)resident_depth_clears, surface_color_upload_bytes/1048576.0, surface_depth_upload_bytes/1048576.0, target_pool.size());
 std::fprintf(stderr, "[GPU-GEOMETRY-TRANSFER] expanded=%.2f MiB vertices=%.2f MiB indices=%.2f MiB indexed=%llu reused=%llu\n", expanded_vertex_bytes/1048576.0, uploaded_vertex_bytes/1048576.0, uploaded_index_bytes/1048576.0, (unsigned long long)indexed_batches, (unsigned long long)reused_vertices);
 std::fprintf(stderr, "[GPU-SUBMISSION] pipeline calls=%llu skipped=%llu constant uploads=%llu skipped=%llu\n", (unsigned long long)pipeline_binds, (unsigned long long)skipped_pipeline_binds, (unsigned long long)constant_uploads, (unsigned long long)skipped_constant_uploads);
 std::fprintf(stderr,"[GPU-CLEARS] %llu color, %llu depth attachment clears without readback\n",(unsigned long long)gpu_color_clears,(unsigned long long)gpu_depth_clears);
 std::fprintf(stderr,"[GPU-D3D11] %llu batches, %llu triangles, %llu readbacks, %llu texture uploads\n",(unsigned long long)batches,(unsigned long long)triangles,(unsigned long long)readbacks,(unsigned long long)uploads);
 std::fprintf(stderr,"[GPU-RENDERED-TEXTURES] %llu GPU snapshots, %llu direct bindings, %zu snapshot-cache entries\n",(unsigned long long)rendered_texture_copies,(unsigned long long)rendered_texture_hits,rendered_textures.size());
 std::fprintf(stderr,"[GPU-SIMPLE-SHADERS] %llu draws used dedicated simple shaders\n",(unsigned long long)simple_shader_draws);
 std::fprintf(stderr,"[GPU-DEFERRED-COLOR] %llu intermediate full-resolution readbacks deferred\n",(unsigned long long)deferred_color_readbacks);
 std::fprintf(stderr,"[GPU-CLEAR-DISCARD] color=%llu depth=%llu overwritten transfers skipped\n",(unsigned long long)discarded_color,(unsigned long long)discarded_depth);
 std::fprintf(stderr,"[GPU-DEPTH-RESOLVE] %llu reduced transfers\n",(unsigned long long)gpu_depth_resolves);
 std::fprintf(stderr,"[GPU-DEPTH] %llu readbacks, %llu unchanged passes skipped\n",(unsigned long long)depth_readbacks,(unsigned long long)skipped_depth_readbacks);
 LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);double ms=1000.0/frequency.QuadPart;
 collect_gpu_samples();
 const char *operations[]={"draw","color reduction","depth reduction"};
 for(unsigned i=0;i<3;i++)if(gpu_duration[i].samples)std::fprintf(stderr,"[GPU-EXECUTION] %s samples=%llu mean=%.4f ms max=%.4f ms\n",operations[i],(unsigned long long)gpu_duration[i].samples,gpu_duration[i].total_ms/gpu_duration[i].samples,gpu_duration[i].max_ms);
 std::fprintf(stderr,"[GPU-HIGH-COLOR] %llu full-resolution CPU fetches, %.2f MiB, %.2f ms\n",(unsigned long long)materialized_color_readbacks,materialized_color_bytes/1048576.0,materialized_color_ticks*ms);
 for(const auto &row:sync_audit)std::fprintf(stderr,"[GPU-SYNC-AUDIT] %s guest=%ux%u physical=%ux%u calls=%llu color=%.2f MiB depth=%.2f MiB wait=%.2f ms\n",row.reason,row.width,row.height,row.scaled_width,row.scaled_height,(unsigned long long)row.calls,row.color_bytes/1048576.0,row.depth_bytes/1048576.0,row.ticks*ms);
 std::fprintf(stderr,"[GPU-TIMING] cumulative prepare %.1f ms, submit %.1f ms, sync %.1f ms (sync may overlap prepare); cached states %zu/%zu/%zu\n",prepare_ticks*ms,submit_ticks*ms,sync_ticks*ms,sampler_states.size(),blend_states.size(),depth_states.size());
}}
void nv_gpu_shutdown(void){nv_gpu_sync();resident_guest={};resident_color_offsets.clear();release(resident_color_view);resident_color_snapshot.clear();resident_depth_snapshot.clear();release(index_buffer);index_capacity=index_cursor=0;bound_draw.valid=false;constants_current=false;mirrored_clear_color=nullptr;for(auto &entry:rendered_textures){release(entry.view);release(entry.image);}rendered_textures.clear();for(auto &sample:gpu_samples){release(sample.disjoint);release(sample.begin);release(sample.end);sample.pending=false;}release(present_gpu);release(present_gpu_read);present_pixels_current=false;release(depth_resolve_srv);depth_resolve_source=nullptr;release(depth_resolve_uav);release(depth_resolve_output);release(depth_resolve_read);release(depth_resolve_shader);release(depth_resolve_constants);preserved_color=nullptr;preserved_depth=nullptr;release(resolve_srv);resolve_source=nullptr;release(resolve_uav);release(resolve_output);release(resolve_read);release(resolve_shader);release(resolve_constants);for(auto &t:target_pool)release_target(t);target_pool.clear();release(texture);for(unsigned i=0;i<3;i++){release(extra_texture[i]);release(extra_sampler[i]);}for(auto &t:textures)release(t.srv);textures.clear();release(sampler);release(blend);release(ds);clear_states(sampler_states);clear_states(blend_states);clear_states(depth_states);release(raster);release(vb);release(cb);release(layout);release(vs);release(ps);for(auto &simple_shader:simple_ps)release(simple_shader);release(rtv);release(dsv);release(color);release(depth);release(color_read);release(depth_read);release(context);release(device);}
