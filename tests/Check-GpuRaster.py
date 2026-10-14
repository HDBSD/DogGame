from test_support import msbuild
"""Run the actual hardware D3D11 backend offscreen and inspect readback."""
from pathlib import Path
import subprocess, os
here=Path(__file__).resolve().parent
backend=here.parent/'build/runtime/xboxrecomp/src/kernel/nv2a_gpu.cpp'
code=f'#include "{backend.as_posix()}"\n#include "{(backend.parent / "nv2a_combiner.c").as_posix()}"\n'+r'''
static uint32_t pixels[64*64];static float zvalues[64*64];
static int internal;
static float quad_offset;
static float quad_fog=1;
static unsigned internal_w=1280,internal_h=960;
extern "C" int xbox_DisplayInternalScaling(void){return internal;}
extern "C" unsigned xbox_DisplayResolutionSelected(void){return 0;}
extern "C" void xbox_DisplayResolutionSize(unsigned,unsigned *w,unsigned *h){*w=internal_w;*h=internal_h;}
static int decode(void*,unsigned x,unsigned y,uint32_t *p){*p=x?(y?0xFFFFFFFF:0xFF00FF00):(y?0xFF0000FF:0xFFFF0000);return 1;}
static int constant_texture(void*,unsigned,unsigned,uint32_t *p){*p=0xFF80FFFF;return 1;}
static unsigned resident_decode_calls;
static int decode_resident_guest(void *arg,unsigned x,unsigned y,uint32_t *pixel){
 resident_decode_calls++;*pixel=((uint32_t*)arg)[y*64+x];return 1;
}
static uint64_t morton_surface(unsigned,unsigned x,unsigned y){
 unsigned offset=0;for(unsigned bit=0;bit<6;bit++){offset|=((x>>bit)&1)<<(2*bit);offset|=((y>>bit)&1)<<(2*bit+1);}return offset*4;
}
static void quad(float z,float r,float g,float b,float alpha){
 NvGpuVertex v[6]={};float xy[6][2]={{0,0},{64,0},{0,64},{64,0},{64,64},{0,64}};
 for(unsigned i=0;i<6;i++){v[i].pos[0]=xy[i][0]+quad_offset;v[i].pos[1]=xy[i][1];v[i].pos[2]=z;v[i].pos[3]=1;v[i].color[0]=r;v[i].color[1]=g;v[i].color[2]=b;v[i].color[3]=alpha;v[i].tex[0]=xy[i][0]/64;v[i].tex[1]=xy[i][1]/64;v[i].tex[3]=1;v[i].fog=quad_fog;}
 for(unsigned i=0;i<6;i++)for(unsigned st=0;st<3;st++)memcpy(v[i].extra[st],v[i].tex,16);
 nv_gpu_triangle(v);nv_gpu_triangle(v+3);nv_gpu_end();
}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed line %d\n",__LINE__);return 1;}}while(0)
int main(){
 {
  std::vector<float> source(128*111),expected(71*48),actual;
  for(size_t i=0;i<source.size();i++)source[i]=i%7?((i%97-20.f)*.03f):1;
  source[99]=NAN;source[102]=INFINITY;
  for(size_t i=0;i<expected.size();i++)expected[i]=i%3?99:2;
  actual=expected;NvGpuDraw guest={};guest.width=67;guest.height=48;guest.depth_stride=71;guest.zscale=13;
  float sx=123.f/67,sy=111.f/48;
  for(unsigned y=0;y<48;y++)for(unsigned x=0;x<67;x++){
   unsigned xx=std::min(122u,(unsigned)((x+.5f)*sx)),yy=std::min(110u,(unsigned)((y+.5f)*sy));
   float v=source[yy*128+xx];float &old=expected[y*71+x];if(v!=1 || old<=13)old=v*13;
  }
  guest.depth=actual.data();read_scaled_depth(source.data(),128,guest,123,111,sx,sy);
  CHECK(!memcmp(expected.data(),actual.data(),expected.size()*sizeof(float)));
 }

 /* Exhaustive channel sums across supported display-scale footprints:
  * SIMD averaging must preserve every integer rounding boundary. */
 for(unsigned n=1;n<=36;n++)for(unsigned value=0;value<=255*n;value++){
  unsigned values[4]={value,255*n-value,value/2,(value*17)%(255*n+1)};
  __m128i sums=_mm_set_epi16(0,0,0,0,(short)values[3],(short)values[2],(short)values[1],(short)values[0]);
  unsigned expected=0;for(unsigned ch=0;ch<4;ch++)expected|=((values[ch]+n/2)/n)<<(8*ch);
  CHECK(average_channels(sums,n)==expected);
 }

 /* Scaled depth uploads must exactly match normalize-after-expansion,
  * including NaN, clear values and physical row padding. */
 {
  std::vector<float> source(648*480),expanded(1920*1080),reference(1936*1080,-123),optimized(reference);
  for(size_t i=0;i<source.size();i++)source[i]=(i%97-20.f)*.17f;
  source[3]=NAN;source[6]=INFINITY;source[9]=-INFINITY;
  NvGpuDraw guest={};guest.width=640;guest.height=480;guest.depth=source.data();guest.depth_stride=648;guest.zscale=13;
  for(unsigned y=0;y<1080;y++)for(unsigned x=0;x<1920;x++)expanded[(size_t)y*1920+x]=source[(size_t)(y*480/1080)*648+x*640/1920];
  NvGpuDraw physical=guest;physical.width=1920;physical.height=1080;physical.depth=expanded.data();physical.depth_stride=1920;
  upload_depth_rows(reference.data(),1936,physical);
  upload_scaled_depth(optimized.data(),1936,guest,1920,1080);
  CHECK(!memcmp(reference.data(),optimized.data(),reference.size()*sizeof(float)));
 }

 /* Depth conversion must retain scalar semantics, row padding and tails. */
 {
  float src[22],dst[26];for(unsigned i=0;i<22;i++)src[i]=(float)i-5;
  src[0]=NAN;src[1]=INFINITY;src[2]=-INFINITY;
  for(float &v:dst)v=-99;
  NvGpuDraw u={};u.width=7;u.height=2;u.depth=src;u.depth_stride=11;u.zscale=13;
  upload_depth_rows(dst,13,u);
  for(unsigned y=0;y<2;y++)for(unsigned x=0;x<13;x++) {
   if(x<7)CHECK(dst[y*13+x]==std::min(1.0f,std::max(0.0f,src[y*11+x]/13)));
   else CHECK(dst[y*13+x]==-99);
  }
  u.depth=nullptr;upload_depth_rows(dst,13,u);
  for(unsigned y=0;y<2;y++)for(unsigned x=0;x<7;x++)CHECK(dst[y*13+x]==1);
 }
 /* Preserve out-of-range clear values, NaNs, tails and independent strides. */
 {
  float src[22],dst[26],expected[26];
  for(unsigned i=0;i<22;i++)src[i]=(i%3==0)?1.0f:(float)i/20;
  src[1]=NAN;src[2]=INFINITY;src[3]=-INFINITY;
  for(unsigned i=0;i<26;i++)dst[i]=expected[i]=(i%2)?99.0f:2.0f;
  NvGpuDraw u={};u.width=7;u.height=2;u.depth=dst;u.depth_stride=13;u.zscale=13;
  for(unsigned y=0;y<2;y++)for(unsigned x=0;x<7;x++)
   if(src[y*11+x]!=1||expected[y*13+x]<=13)expected[y*13+x]=src[y*11+x]*13;
  read_depth_rows(src,11,u);
  for(unsigned i=0;i<26;i++)CHECK(dst[i]==expected[i]||(std::isnan(dst[i])&&std::isnan(expected[i])));
 }
 _putenv_s("RECOMP_GPU","1");
 _putenv_s("RECOMP_PROFILE_GPU","1");
 for(unsigned i=0;i<4096;i++){pixels[i]=0xFF203040;zvalues[i]=1;}
 NvGpuDraw d={};d.color=(uint8_t*)pixels;d.depth=zvalues;d.width=d.height=d.clip_w=d.clip_h=64;d.pitch=256;d.depth_stride=64;d.zscale=1;d.depth_test=d.depth_write=1;d.depth_func=0x201;d.src=1;d.mask=0x01010101;d.mode=2;
 CHECK(nv_gpu_begin(&d));quad(.5,1,0,0,1);
 uint64_t before=readbacks;
 d.depth_test=d.depth_write=0;CHECK(nv_gpu_begin(&d));quad(.5,1,0,0,1);
 d.depth_test=d.depth_write=1;d.height=d.clip_h=32;
 CHECK(nv_gpu_begin(&d));quad(.5,1,0,0,1);
 d.height=d.clip_h=64;CHECK(nv_gpu_begin(&d));CHECK(readbacks==before);
 CHECK(nv_gpu_begin(&d));quad(.8,0,1,0,1);nv_gpu_sync();
 CHECK(pixels[32*64+32]==0xFFFF0000);CHECK(fabs(zvalues[32*64+32]-.5)<1e-6);
 /* A read-only depth pass must not copy depth back or alter guest values.
  * Writes earlier in a mixed pass still require a depth readback. */
 {
  uint64_t reads=depth_readbacks,skips=skipped_depth_readbacks;
  float saved=zvalues[32*64+32];
  d.depth_write=0;CHECK(nv_gpu_begin(&d));quad(.8,0,1,0,1);nv_gpu_sync();
  CHECK(depth_readbacks==reads && skipped_depth_readbacks==skips+1);
  CHECK(zvalues[32*64+32]==saved && pixels[32*64+32]==0xFFFF0000);
  d.depth_write=1;CHECK(nv_gpu_begin(&d));quad(.4,1,0,0,1);
  d.depth_write=0;CHECK(nv_gpu_begin(&d));quad(.3,1,0,0,1);nv_gpu_sync();
  CHECK(depth_readbacks==reads+1 && fabs(zvalues[32*64+32]-.4)<1e-6);
  d.depth_write=1;
 }
 /* Complete clear discard must skip only overwritten attachments. */
 {
  uint32_t clear_pixels[4096];float clear_depth[4096];NvGpuDraw u=d;
  u.color=(uint8_t*)clear_pixels;u.depth=clear_depth;u.depth_write=1;
  for(unsigned i=0;i<4096;i++){clear_pixels[i]=0xFF112233;clear_depth[i]=1;}
  CHECK(nv_gpu_begin(&u));quad(.2,1,0,0,1);
  uint64_t reads=readbacks;
  nv_gpu_discard_overwritten(u.color,u.pitch,0,0,0,64,64,u.depth);nv_gpu_sync();
  CHECK(readbacks==reads && clear_pixels[32*64+32]==0xFF112233 && clear_depth[32*64+32]==1);
  for(unsigned i=0;i<4096;i++){clear_pixels[i]=0xFF445566;clear_depth[i]=.9f;}
  CHECK(nv_gpu_begin(&u));quad(.3,0,1,0,1);nv_gpu_sync();
  CHECK(clear_pixels[32*64+32]==0xFF00FF00 && fabs(clear_depth[32*64+32]-.3)<1e-6);
  CHECK(nv_gpu_begin(&u));quad(.1,1,0,0,1);
  reads=readbacks;nv_gpu_discard_overwritten(u.color,u.pitch,0,1,0,63,64,NULL);nv_gpu_sync();
  CHECK(readbacks==reads+1 && clear_pixels[32*64+32]==0xFFFF0000);
  CHECK(nv_gpu_begin(&u));quad(.05,0,0,1,1);
  nv_gpu_discard_overwritten(u.color,u.pitch,0,0,0,64,64,NULL);nv_gpu_sync();
  CHECK(clear_pixels[32*64+32]==0xFFFF0000 && fabs(clear_depth[32*64+32]-.05)<1e-6);
  CHECK(nv_gpu_begin(&u));quad(.01,0,1,0,1);
  nv_gpu_discard_overwritten(NULL,0,0,0,0,0,0,u.depth);nv_gpu_sync();
  CHECK(clear_pixels[32*64+32]==0xFF00FF00 && fabs(clear_depth[32*64+32]-.05)<1e-6);
 }
 /* Changing only the depth attachment keeps dirty color on the GPU.
  * The old depth must be published before its CPU storage can be reused. */
 {
  uint32_t attachment_pixels[4096];float old_depth[4096],new_depth[4096];NvGpuDraw u=d;
  for(unsigned i=0;i<4096;i++){attachment_pixels[i]=0xFF112233;old_depth[i]=new_depth[i]=1;}
  u.color=(uint8_t*)attachment_pixels;u.depth=old_depth;u.depth_write=1;
  CHECK(nv_gpu_begin(&u));quad(.4,1,0,0,1);
  nv_gpu_sync_depth("depth-only regression");
  CHECK(attachment_pixels[32*64+32]==0xFF112233 && fabs(old_depth[32*64+32]-.4)<1e-6);
  CHECK(color_dirty && !depth_dirty && dirty);
  u.depth=new_depth;CHECK(nv_gpu_begin(&u));
  CHECK(color_dirty && attachment_pixels[32*64+32]==0xFF112233);
  u.clip_x=32;u.clip_w=32;CHECK(nv_gpu_begin(&u));quad(.3,0,1,0,1);nv_gpu_sync();
  CHECK(attachment_pixels[32*64+16]==0xFFFF0000 && attachment_pixels[32*64+48]==0xFF00FF00);
  CHECK(fabs(new_depth[32*64+48]-.3)<1e-6 && new_depth[32*64+16]==1);
 }
 d.blend=1;d.src=0x302;d.dst=0x303;d.equation=0x8006;
 CHECK(nv_gpu_begin(&d));quad(.1,0,0,1,.5);nv_gpu_sync();
 uint32_t c=pixels[32*64+32];CHECK(abs((int)((c>>16)&255)-128)<=1 && abs((int)(c&255)-128)<=1);
 d.blend=0;d.depth_test=d.depth_write=0;d.depth=nullptr;d.mode=1;d.textured=1;d.tw=d.th=2;d.texture_key=1;d.texture_hash=1;d.decode=decode;d.addr_u=d.addr_v=3;
 d.combiner.stage_program=1;
 CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();
 CHECK(pixels[16*64+16]==0xFFFF0000 && pixels[16*64+48]==0xFF00FF00);
 CHECK(pixels[48*64+16]==0xFF0000FF && pixels[48*64+48]==0xFFFFFFFF);
 for(unsigned i=0;i<4096;i++)pixels[i]=0xFF112233;
 d.alpha_test=1;d.alpha_func=0x204;d.alpha_ref=128;
 CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,0);nv_gpu_sync();CHECK(pixels[32*64+32]==0xFF112233);
 d.alpha_test=0;d.mode=0;d.textured=0;d.combiner.stage_program=0;
 uint32_t random=12345;
 for(unsigned trial=0;trial<100;trial++) {
  d.combiner.control=1+trial%3;
  for(unsigned s=0;s<3;s++) {
   random=random*1664525u+1013904223u;d.combiner.factor0[s]=random;
   random=random*1664525u+1013904223u;d.combiner.factor1[s]=random;
   d.combiner.color_icw[s]=0x04010502u;d.combiner.alpha_icw[s]=0x14111512u;
   d.combiner.color_ocw[s]=0xC00u|((trial%7)<<15);d.combiner.alpha_ocw[s]=0xC00u;
  }
  d.combiner.final0=trial%2?0x130C0300u:0;d.combiner.final1=trial%2?0x1C80u:0;
  float v0[4]={.25,.5,.75,1},v1[4]={0},fog[4]={0,0,0,1},t[4][4]={{0,0,0,1},{0,0,0,1},{0,0,0,1},{0,0,0,1}},expected[4];
  nv2a_rc_eval(&d.combiner,v0,v1,fog,t,expected);
  CHECK(nv_gpu_begin(&d));quad(.1,.25,.5,.75,1);nv_gpu_sync();uint32_t result=pixels[32*64+32];
  for(unsigned k=0;k<4;k++){unsigned shift=k==3?24:16-8*k;int want=(int)(expected[k]*255+.5f);CHECK(abs((int)((result>>shift)&255)-want)<=1);}
 }
 d.combiner={};d.combiner.control=1;d.combiner.stage_program=1|(1<<5);
 d.combiner.color_icw[0]=0x08090000;d.combiner.alpha_icw[0]=0x18190000;
 d.combiner.color_ocw[0]=d.combiner.alpha_ocw[0]=0xC00;d.textured=1;
 d.extra[0].valid=1;d.extra[0].width=d.extra[0].height=2;d.extra[0].key=2;d.extra[0].hash=2;d.extra[0].decode=constant_texture;d.extra[0].addr_u=d.extra[0].addr_v=3;
 CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();CHECK(pixels[48*64+48]==0xFF80FFFF);
 for(unsigned i=0;i<4096;i++)pixels[i]=0xFF112233;
 d.combiner.stage_program|=5<<15;d.clip_plane_mode=1<<12;
 CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();CHECK(pixels[32*64+32]==0xFF112233);
 // Exercise all dedicated simple modes against the software evaluator,
 // retaining dynamic texture, fog color/factor and alpha values.
 {
  NvGpuDraw u={};u.color=(uint8_t*)pixels;u.width=u.height=u.clip_w=u.clip_h=64;u.pitch=256;u.zscale=1;u.src=1;u.mask=0x01010101;
  u.textured=1;u.tw=u.th=2;u.texture_key=999;u.texture_hash=999;u.decode=constant_texture;u.addr_u=u.addr_v=3;u.combiner.stage_program=1;
  u.fog_color[0]=.1f;u.fog_color[1]=.2f;u.fog_color[2]=.3f;quad_fog=.25f;
  for(unsigned mode:{1u,2u,5u,6u}){
   u.mode=mode;CHECK(nv_gpu_begin(&u));quad(.1f,.25f,.5f,.75f,.65f);nv_gpu_sync();
   float color[4]={.25f,.5f,.75f,.65f},fog[4]={.1f,.2f,.3f,.25f},texel[4]={128.f/255,1,1,1},expected[4];
   nv2a_rc_eval_simple(mode,color,fog,texel,expected);unsigned pixel=pixels[32*64+32];
   for(unsigned ch=0;ch<4;ch++){unsigned shift=ch==3?24:16-8*ch;CHECK(abs((int)((pixel>>shift)&255)-(int)(expected[ch]*255+.5f))<=1);}
  }
  quad_fog=1;
 }
 // Render-to-texture binding bypasses CPU decoding/upload while preserving
 // mutable guest RAM, refreshed GPU writes, multistage use and layout checks.
 if(use_rendered_textures()){
  uint32_t source_pixels[4096],destination_pixels[4096];
  for(unsigned i=0;i<4096;i++){source_pixels[i]=destination_pixels[i]=0xFF112233;}
  NvGpuDraw source_draw={};source_draw.color=(uint8_t*)source_pixels;
  source_draw.width=source_draw.height=source_draw.clip_w=source_draw.clip_h=64;
  source_draw.pitch=256;source_draw.zscale=1;source_draw.src=1;source_draw.mask=0x01010101;source_draw.mode=2;
  CHECK(nv_gpu_begin(&source_draw));quad(.1,1,0,0,1);nv_gpu_sync();
  NvGpuDraw consumer=source_draw;consumer.color=(uint8_t*)destination_pixels;consumer.mode=1;
  consumer.textured=1;consumer.tw=consumer.th=64;consumer.texture_guest=(uint8_t*)source_pixels;
  consumer.texture_format=0x12;consumer.texture_pitch=256;consumer.texture_key=987;consumer.texture_hash=1;
  consumer.decode=decode_resident_guest;consumer.decode_arg=source_pixels;consumer.addr_u=consumer.addr_v=3;consumer.combiner.stage_program=1;
  resident_decode_calls=0;uint64_t hit_count=rendered_texture_hits,upload_count=uploads;
  CHECK(nv_gpu_begin(&consumer));quad(.1,1,1,1,1);nv_gpu_sync();
  CHECK(destination_pixels[32*64+32]==0xFFFF0000 && resident_decode_calls==0);
  CHECK(rendered_texture_hits==hit_count+1 && uploads==upload_count);
  consumer.mode=0;consumer.combiner.control=1;consumer.combiner.stage_program=1|(1<<5);
  consumer.combiner.color_icw[0]=0x08090000;consumer.combiner.alpha_icw[0]=0x18190000;
  consumer.combiner.color_ocw[0]=consumer.combiner.alpha_ocw[0]=0xC00;
  auto &extra=consumer.extra[0];extra.valid=1;extra.width=extra.height=64;extra.key=988;extra.hash=1;
  extra.guest=(uint8_t*)source_pixels;extra.format=0x12;extra.pitch=256;extra.decode=decode_resident_guest;extra.arg=source_pixels;extra.addr_u=extra.addr_v=3;
  hit_count=rendered_texture_hits;CHECK(nv_gpu_begin(&consumer));quad(.1,1,1,1,1);nv_gpu_sync();
  CHECK(destination_pixels[32*64+32]==0xFFFF0000 && resident_decode_calls==0 && rendered_texture_hits==hit_count+2);
  // A CPU edit must fall back to decoding the new bytes, not sample stale GPU pixels.
  source_pixels[32*64+32]=0xFF0000FF;consumer.mode=1;consumer.extra[0].valid=0;consumer.combiner.stage_program=1;consumer.texture_hash=2;
  CHECK(nv_gpu_begin(&consumer));quad(.1,1,1,1,1);nv_gpu_sync();
  CHECK(destination_pixels[32*64+32]==0xFF0000FF && resident_decode_calls==4096);
  CHECK(!rendered_texture_view((uint8_t*)source_pixels,64,64,0x07,256));
  CHECK(!rendered_texture_view((uint8_t*)source_pixels,32,64,0x12,256));
  CHECK(!rendered_texture_view((uint8_t*)source_pixels,64,64,0x12,128));
  // Re-rendering a recycled source address must replace the resident GPU snapshot.
  CHECK(nv_gpu_begin(&source_draw));quad(.1,0,1,0,1);nv_gpu_sync();
  consumer.texture_hash=3;CHECK(nv_gpu_begin(&consumer));quad(.1,1,1,1,1);nv_gpu_sync();
  CHECK(destination_pixels[32*64+32]==0xFF00FF00 && resident_decode_calls==4096);
  // Swizzled RAM must bind the linear GPU image only with a matching swizzled texture format.
  source_draw.surface_offset=morton_surface;source_draw.surface_format=1;
  CHECK(nv_gpu_begin(&source_draw));quad(.1,0,1,1,1);nv_gpu_sync();
  CHECK(!rendered_texture_view((uint8_t*)source_pixels,64,64,0x12,256));
  consumer.texture_format=0x06;consumer.texture_hash=4;
  CHECK(nv_gpu_begin(&consumer));quad(.1,1,1,1,1);nv_gpu_sync();
  CHECK(destination_pixels[32*64+32]==0xFF00FFFF && resident_decode_calls==4096);
  CHECK(rendered_textures.size()<=8);
 }
 // Updating one animated texture must retain unrelated static cache entries.
 size_t cache_count=textures.size();
 size_t sampler_count=sampler_states.size(),blend_count=blend_states.size(),depth_count=depth_states.size();
 d.combiner.stage_program&=~(5u<<15);d.clip_plane_mode=0;
 for(unsigned frame=0;frame<140;frame++){
  d.extra[0].hash=1000+frame;
  CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();
  CHECK(pixels[48*64+48]==0xFF80FFFF);
  CHECK(textures.size()==cache_count);
  CHECK(sampler_states.size()==sampler_count&&blend_states.size()==blend_count&&depth_states.size()==depth_count);
 }
 // A CPU synchronization boundary must refresh contents without replacing
 // matching render-target resources.
 ID3D11Texture2D *retained_color=color,*retained_depth=depth;
 uint64_t uploads_before=uploads;
 CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();
 CHECK(uploads==uploads_before);
 CHECK(color==retained_color&&depth==retained_depth);
 // A swizzled target must store every checker pixel in Morton order.
 d.combiner={};d.combiner.stage_program=1;d.mode=1;d.extra[0]={};d.alpha_test=0;
 d.surface_offset=morton_surface;d.surface_format=1;
 CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();
 for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
  uint32_t expected;decode(nullptr,x>=32,y>=32,&expected);
  CHECK(pixels[morton_surface(1,x,y)/4]==expected);
 }
 // Verify upload and masked rendering preserve CPU-written swizzled pixels.
 for(unsigned i=0;i<4096;i++)pixels[i]=0xFF123456;
 d.mask=0;CHECK(nv_gpu_begin(&d));quad(.1,1,1,1,1);nv_gpu_sync();
 for(unsigned i=0;i<4096;i++)CHECK(pixels[i]==0xFF123456);
 // Alternating post-processing sizes must reuse the original target objects
 // while refreshing their contents and preserving the correct dimensions.
 ID3D11Texture2D *large_target=color;
 d.surface_offset=nullptr;d.surface_format=0;d.mode=2;d.textured=0;
 d.mask=0x01010101;
 for(unsigned pass=0;pass<10;pass++){
  d.width=d.height=d.clip_w=d.clip_h=32;d.pitch=128;
  CHECK(nv_gpu_begin(&d));quad(.1,1,0,0,1);nv_gpu_sync();
  CHECK(pixels[16*32+16]==0xFFFF0000);
  d.width=d.height=d.clip_w=d.clip_h=64;d.pitch=256;
  CHECK(nv_gpu_begin(&d));quad(.1,0,1,0,1);nv_gpu_sync();
  CHECK(color==large_target);CHECK(pixels[32*64+32]==0xFF00FF00);
 }
 CHECK(target_pool.size()==1);
 // Fill and wrap the production streaming buffer without intermediate
 // readbacks. Earlier geometry must keep its original vertex data.
 d.clip_x=0;d.clip_w=32;CHECK(nv_gpu_begin(&d));quad(.1,1,0,0,1);
 d.clip_x=32;
 for(unsigned draw_index=0;draw_index<6000;draw_index++){
  CHECK(nv_gpu_begin(&d));quad(.1,0,draw_index&1?1.f:0.f,draw_index&1?0.f:1.f,1);
 }
 nv_gpu_sync();CHECK(pixels[32*64+16]==0xFFFF0000);CHECK(pixels[32*64+48]==0xFF00FF00);
 {
  std::vector<uint32_t> scene(640*480,0xFF123456);d={};
  d.color=(uint8_t*)scene.data();d.width=d.clip_w=640;d.height=d.clip_h=480;
  d.pitch=640*4;d.zscale=1;d.src=1;d.mask=0x01010101;d.mode=2;
  internal=1;CHECK(nv_gpu_begin(&d));CHECK(target_width==1280 && target_height==960);
  quad(.5,1,0,0,1);nv_gpu_sync();
  CHECK(scene[32*640+32]==0xFFFF0000 && scene[100*640+100]==0xFF123456);
  if(!getenv("RECOMP_GPU_RESOLVE") || strcmp(getenv("RECOMP_GPU_RESOLVE"),"0")){CHECK(!present_pixels_current && deferred_color_readbacks>0);}
  unsigned pw=0,ph=0;
  const uint32_t *hi=nv_gpu_present_image(d.color,d.pitch,&pw,&ph);
  CHECK(hi && pw==1280 && ph==960 && present_pixels_current);
  CHECK(hi[64*pw+64]==0xFFFF0000 && hi[200*pw+200]==0xFF123456);
  scene[0]^=1;CHECK(!nv_gpu_present_image(d.color,d.pitch,&pw,&ph));scene[0]^=1;
  CHECK(!nv_gpu_present_image(d.color,d.pitch+4,&pw,&ph));
  uint64_t kept=preserved_uploads;
  quad_offset=.5f;CHECK(nv_gpu_begin(&d));CHECK(preserved_uploads==kept+1);quad(.5,1,0,0,1);nv_gpu_sync();quad_offset=0;
  hi=nv_gpu_present_image(d.color,d.pitch,&pw,&ph);CHECK(hi);
  CHECK(hi[64*pw+128]!=hi[64*pw+129]);
  CHECK(scene[32*640+64]!=hi[64*pw+128] && scene[32*640+64]!=hi[64*pw+129]);
  /* A readback/next-pass upload must preserve sharp subpixel boundaries.
   * A software write elsewhere must still appear in that next pass. */
  scene[100*640+100]=0xFFABCDEF;
  CHECK(nv_gpu_begin(&d));quad(.5,1,0,0,1);nv_gpu_sync();
  hi=nv_gpu_present_image(d.color,d.pitch,&pw,&ph);CHECK(hi);
  CHECK(hi[64*pw+128]!=hi[64*pw+129]);
  CHECK(hi[200*pw+200]==0xFFABCDEF);
  std::vector<float> scene_depth(640*480,13);
  d.depth=scene_depth.data();d.depth_stride=640;d.zscale=13;
  d.depth_test=d.depth_write=1;d.depth_func=0x201;internal_h=720;
  CHECK(nv_gpu_begin(&d));CHECK(target_width==1280 && target_height==720);
  quad(.5,0,0,1,1);nv_gpu_sync();
  CHECK(scene[32*640+32]==0xFF0000FF && fabs(scene_depth[32*640+32]-.5)<1e-6);
  CHECK(scene_depth[100*640+100]==13);
  {
   uint64_t kept_depth=preserved_depth_uploads;
   CHECK(nv_gpu_begin(&d));CHECK(preserved_depth_uploads==kept_depth+1);
   quad(.5,0,0,1,1);nv_gpu_sync();
   scene_depth[100*640+100]=7;
   kept_depth=preserved_depth_uploads;CHECK(nv_gpu_begin(&d));CHECK(preserved_depth_uploads==kept_depth);
   quad(.5,0,0,1,1);nv_gpu_sync();CHECK(fabs(scene_depth[100*640+100]-7)<1e-6);
  }
  internal_w=1920;internal_h=1080;d.depth_test=d.depth_write=0;
  CHECK(nv_gpu_begin(&d));quad_offset=.5f;quad(.5,.123f,.456f,.789f,.65f);quad_offset=0;nv_gpu_sync();
  hi=nv_gpu_present_image(d.color,d.pitch,&pw,&ph);CHECK(hi && pw==1920 && ph==1080);
  /* Compare the optimized compatibility resolve with scalar box averaging,
   * including fractional row scaling and all four channels. */
  for(unsigned y=0;y<480;y++)for(unsigned x=0;x<640;x++){
   unsigned x0=(unsigned)(x*3.f),x1=(unsigned)((x+1)*3.f);
   unsigned y0=(unsigned)(y*2.25f),y1=(unsigned)((y+1)*2.25f);
   unsigned sums[4]={},n=0;
   for(unsigned yy=y0;yy<y1;yy++)for(unsigned xx=x0;xx<x1;xx++){
    unsigned pixel=hi[(size_t)yy*pw+xx];for(unsigned ch=0;ch<4;ch++)sums[ch]+=(pixel>>(ch*8))&255;n++;
   }
   unsigned expected=0;for(unsigned ch=0;ch<4;ch++)expected|=((sums[ch]+n/2)/n)<<(ch*8);
   CHECK(scene[(size_t)y*640+x]==expected);
  }
  /* Full-image depth reduction matches the original center sampling and
   * clear-value handling at integer and fractional physical scales. */
  for(unsigned trial=0;trial<3;trial++){
   internal_w=trial==2?1920:1280;internal_h=trial==0?960:trial==1?720:1080;
   d.depth=scene_depth.data();d.depth_stride=640;d.zscale=13;
   d.depth_write=1;CHECK(nv_gpu_begin(&d));
   std::vector<float> physical((size_t)target_width*target_height),expected(640*480,99);
   for(size_t i=0;i<physical.size();i++)physical[i]=i%13==0?1.f:(float)(i%997)/997;
   std::fill(scene_depth.begin(),scene_depth.end(),99.f);
   read_scaled_depth(physical.data(),target_width,NvGpuDraw{d},target_width,target_height,render_scale_x,render_scale_y);
   expected=scene_depth;std::fill(scene_depth.begin(),scene_depth.end(),99.f);
   D3D11_MAPPED_SUBRESOURCE upload={};CHECK(SUCCEEDED(context->Map(depth_read,0,D3D11_MAP_WRITE,0,&upload)));
   for(unsigned y=0;y<target_height;y++)memcpy((char*)upload.pData+(size_t)y*upload.RowPitch,physical.data()+(size_t)y*target_width,target_width*4);
   context->Unmap(depth_read,0);context->CopyResource(depth,depth_read);
   dirty=depth_dirty=true;nv_gpu_sync();
   CHECK(!memcmp(scene_depth.data(),expected.data(),expected.size()*4));
  }
  d.depth=nullptr;d.depth_test=d.depth_write=0;d.zscale=1;
  internal=0;CHECK(nv_gpu_begin(&d));CHECK(target_width==640 && target_height==480);
  quad(.5,0,1,0,1);nv_gpu_sync();CHECK(scene[32*640+32]==0xFF00FF00);
  CHECK(!nv_gpu_present_image(d.color,d.pitch,&pw,&ph));
 }
 CHECK(getenv("RECOMP_GPU_RESOLVE") && !strcmp(getenv("RECOMP_GPU_RESOLVE"),"0") ? gpu_resolves==0 : gpu_resolves>0);
 CHECK(getenv("RECOMP_GPU_DEPTH_RESOLVE") && !strcmp(getenv("RECOMP_GPU_DEPTH_RESOLVE"),"0") ? gpu_depth_resolves==0 : gpu_depth_resolves>0);
 CHECK(getenv("RECOMP_GPU_SIMPLE_SHADERS") && !strcmp(getenv("RECOMP_GPU_SIMPLE_SHADERS"),"0") ? simple_shader_draws==0 : simple_shader_draws>0);
 collect_gpu_samples();CHECK(gpu_duration[0].samples>0 && gpu_duration[0].total_ms>=0);
 uint64_t audited_calls=0;for(const auto &row:sync_audit)audited_calls+=row.calls;CHECK(audited_calls==readbacks && !sync_audit.empty());
 nv_gpu_shutdown();puts("Passed: hardware raster, ordered depth, blending, decoded textures, CPU/GPU handoff, alpha discard and internal supersampling.");return 0;
}
'''
out=here/'irq-test-build';(out/'gpu_raster.cpp').write_text(code)
project=(out/'apu_irq_harness.vcxproj').read_text().replace('apu_irq_harness.c','gpu_raster.cpp').replace('obj\\apu_irq\\','obj\\gpu_raster\\')
project=project.replace('</ItemDefinitionGroup>','<Link><AdditionalDependencies>d3d11.lib;d3dcompiler.lib;%(AdditionalDependencies)</AdditionalDependencies></Link></ItemDefinitionGroup>')
(out/'gpu_raster.vcxproj').write_text(project)
subprocess.run([msbuild,str(out/'gpu_raster.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/gpu_raster.exe')],check=True)
subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_RESOLVE':'0','RECOMP_GPU_DEPTH_RESOLVE':'0'})


subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_SIMPLE_SHADERS':'0'})

subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_RENDERED_TEXTURES':'0'})
