"""Check batch identities and packed GPU vertex reuse at the production boundary."""
from pathlib import Path
import subprocess
from test_support import msbuild

here = Path(__file__).resolve().parent
runtime = here.parent / 'build/runtime/xboxrecomp/src/kernel'
source = (runtime / 'nv2a_pb_exec.c').read_text()
transform = source[source.index('static int transform_batch_vertices('):
                   source.index('#if defined(_WIN32)\nstatic int gpu_decode_texture')]
packing = source[source.index('static unsigned gpu_append_transformed_vertex('):
                 source.index('\n#endif\n\nstatic void raster_xf_triangle')]
code = f'#include "{(runtime / "nv2a_gpu.h").as_posix()}"\n#include "{(runtime / "nv2a_vsh_interp.h").as_posix()}"\n' + r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static Nv2aVshOutput s_xf[16];
static uint32_t s_xf_identity[16];
static unsigned s_gpu_vertex_index[16];
static struct {unsigned idx[16],xf_mode,composite_seen;} s_gpu;
static unsigned calls,appends;
static int cacheable;
static NvGpuVertex uploaded[64];
int nv2a_vsh_cacheable(void){return cacheable;}
static int transform_vertex(unsigned index,Nv2aVshOutput *out)
{
    memset(out,0,sizeof(*out));calls++;
    out->pos[0]=index+.123f;out->pos[1]=index+.456f;out->pos[3]=1;
    out->d0[0]=cacheable?index:(float)calls;
    out->tex[2][0]=index+.5f;out->fog[0]=index+.25f;return 1;
}
static void gpu_scale_texture(unsigned stage,float *v){v[0]*=stage+1;}
static float fog_factor(float v){return v*.5f;}
unsigned nv_gpu_append_vertex(const NvGpuVertex *v){uploaded[appends]=*v;return appends++;}
''' + transform + packing + r'''
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void)
{
    s_gpu.xf_mode=2;s_gpu.idx[0]=7;s_gpu.idx[1]=3;s_gpu.idx[2]=7;
    for(unsigned trial=0;trial<2;trial++)
    {
        cacheable=trial==0;calls=appends=0;
        CHECK(transform_batch_vertices(3));
        CHECK(calls==(cacheable?2:3));
        CHECK(s_xf_identity[2]==(cacheable?0:2));
        for(unsigned i=0;i<3;i++)s_gpu_vertex_index[i]=UINT32_MAX;
        unsigned a=gpu_append_transformed_vertex(&s_xf[0]);
        unsigned b=gpu_append_transformed_vertex(&s_xf[1]);
        unsigned c=gpu_append_transformed_vertex(&s_xf[2]);
        CHECK(a!=b && ((a==c)==cacheable));
        CHECK(gpu_append_transformed_vertex(&s_xf[0])==a);
        CHECK(uploaded[a].pos[0]==s_xf[0].pos[0]);
        CHECK(uploaded[a].extra[1][0]==s_xf[0].tex[2][0]*3);
        CHECK(uploaded[a].fog==s_xf[0].fog[0]*.5f);
        Nv2aVshOutput clipped=s_xf[0];
        unsigned first=gpu_append_transformed_vertex(&clipped);
        clipped.d0[0]=99;
        unsigned next=gpu_append_transformed_vertex(&clipped);
        CHECK(first!=next && uploaded[first].color[0]!=uploaded[next].color[0]);
        /* A new batch at the same addresses must upload fresh attributes. */
        s_xf[0].d0[0]=123;s_gpu_vertex_index[0]=UINT32_MAX;
        next=gpu_append_transformed_vertex(&s_xf[0]);
        CHECK(next!=a && uploaded[next].color[0]==123);
    }
    puts("Passed: batch-local identity, side-effecting transforms, clipped temporaries, packed attributes and fresh batches.");
    return 0;
}
'''
out = here / 'irq-test-build'
(out / 'gpu_vertex_identity.c').write_text(code)
project = (out / 'apu_irq_harness.vcxproj').read_text()
project = project.replace('apu_irq_harness.c', 'gpu_vertex_identity.c')
project = project.replace('obj\\apu_irq\\', 'obj\\gpu_vertex_identity\\')
(out / 'gpu_vertex_identity.vcxproj').write_text(project)
subprocess.run([msbuild, str(out / 'gpu_vertex_identity.vcxproj'),
                '/p:Configuration=Release', '/p:Platform=x64', '/verbosity:minimal'], check=True)
subprocess.run([str(out / 'bin/gpu_vertex_identity.exe')], check=True)
