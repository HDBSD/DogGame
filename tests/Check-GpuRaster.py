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
static unsigned quad_repetitions=1;
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
 if(quad_repetitions>1 && nv_gpu_indexed_upload_enabled()){
  unsigned index[4];index[0]=nv_gpu_append_vertex(v);index[1]=nv_gpu_append_vertex(v+1);
  index[2]=nv_gpu_append_vertex(v+2);index[3]=nv_gpu_append_vertex(v+4);
  unsigned a[3]={index[0],index[1],index[2]},b[3]={index[1],index[3],index[2]};
  for(unsigned i=0;i<quad_repetitions;i++){nv_gpu_indexed_triangle(a);nv_gpu_indexed_triangle(b);}
 }else for(unsigned i=0;i<quad_repetitions;i++){nv_gpu_triangle(v);nv_gpu_triangle(v+3);}
 nv_gpu_end();
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

 /* GPU clears mirror immediate full CPU overwrites without synchronizing
  * the attachment that will be discarded or losing the other attachment. */
 {
  uint32_t fixture_pixels[4096];float fixture_depth[4096];
  memcpy(fixture_pixels,pixels,sizeof pixels);memcpy(fixture_depth,zvalues,sizeof zvalues);
  NvGpuDraw fixture=d;fixture.color=(uint8_t*)fixture_pixels;fixture.depth=fixture_depth;
  NvGpuDraw &d=fixture;uint32_t (&pixels)[4096]=fixture_pixels;float (&zvalues)[4096]=fixture_depth;
  NvGpuClear clear={};clear.color=d.color;clear.depth=d.depth;clear.pitch=d.pitch;
  clear.width=clear.height=64;clear.color_value=0xFF2468AC;clear.depth_value=1;
  CHECK(nv_gpu_begin(&d));quad(.2,1,0,0,1);
  uint64_t reads=readbacks;
  CHECK(nv_gpu_clear_attachments(&clear));
  for(unsigned i=0;i<4096;i++){pixels[i]=clear.color_value;zvalues[i]=1;}
  CHECK(!dirty && !color_dirty && !depth_dirty && readbacks==reads);
  CHECK(nv_gpu_begin(&d));CHECK(readbacks==reads);quad(.7,0,1,0,1);nv_gpu_sync();
  CHECK(pixels[2048]==0xFF00FF00 && fabs(zvalues[2048]-.7)<1e-6);

  CHECK(nv_gpu_begin(&d));quad(.2,1,0,0,1);
  /* CPU edits between a mirrored clear and the next draw must be uploaded. */
  CHECK(nv_gpu_clear_attachments(&clear));
  for(unsigned i=0;i<4096;i++){pixels[i]=clear.color_value;zvalues[i]=1;}
  pixels[32*64+48]=0xFFFF0000;
  d.clip_w=32;CHECK(nv_gpu_begin(&d));quad(.6,0,1,0,1);nv_gpu_sync();
  CHECK(pixels[32*64+16]==0xFF00FF00 && pixels[32*64+48]==0xFFFF0000);
  d.clip_w=64;CHECK(nv_gpu_begin(&d));quad(.2,1,0,0,1);
  clear.color=nullptr;CHECK(nv_gpu_clear_attachments(&clear));
  for(float &z:zvalues)z=1;
  CHECK(dirty && color_dirty && !depth_dirty);
  CHECK(nv_gpu_begin(&d));quad(.6,0,0,1,1);nv_gpu_sync();
  CHECK(pixels[2048]==0xFF0000FF && fabs(zvalues[2048]-.6)<1e-6);

  CHECK(nv_gpu_begin(&d));quad(.2,1,0,0,1);
  clear.color=d.color;clear.depth=nullptr;clear.color_value=0xFF00FF00;
  CHECK(nv_gpu_clear_attachments(&clear));
  for(uint32_t &p:pixels)p=clear.color_value;
  CHECK(dirty && !color_dirty && depth_dirty);
  CHECK(nv_gpu_begin(&d));quad(.8,0,0,1,1);nv_gpu_sync();
  CHECK(pixels[2048]==0xFF00FF00 && fabs(zvalues[2048]-.2)<1e-6);

  CHECK(nv_gpu_begin(&d));quad(.1,1,0,0,1);
  clear.width=32;uint64_t clears=gpu_color_clears;
  CHECK(!nv_gpu_clear_attachments(&clear));
  CHECK(gpu_color_clears==clears && dirty && color_dirty);
  clear.width=64;clear.depth=zvalues+1;
  CHECK(!nv_gpu_clear_attachments(&clear));CHECK(gpu_color_clears==clears);
  nv_gpu_sync();CHECK(pixels[2048]==0xFFFF0000);
 }

 /* No writable attachment means no compatibility readback is needed. */
 {
  NvGpuDraw masked=d;masked.mask=0;masked.depth_write=0;
  uint64_t reads=readbacks;
  CHECK(nv_gpu_begin(&masked));quad(.1,0,1,0,1);
  CHECK(!color_dirty && !depth_dirty && !dirty);
  nv_gpu_sync();CHECK(readbacks==reads);
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
 CHECK(nv_gpu_begin(&d));large_target=color;
 for(unsigned pass=0;pass<10;pass++){
  d.width=d.height=d.clip_w=d.clip_h=32;d.pitch=128;
  CHECK(nv_gpu_begin(&d));quad(.1,1,0,0,1);nv_gpu_sync();
  CHECK(pixels[16*32+16]==0xFFFF0000);
  d.width=d.height=d.clip_w=d.clip_h=64;d.pitch=256;
  CHECK(nv_gpu_begin(&d));quad(.1,0,1,0,1);nv_gpu_sync();
  CHECK(color==large_target);CHECK(pixels[32*64+32]==0xFF00FF00);
 }
 CHECK(target_pool.size()<=8);
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
 /* A guest-sized full clear must clear the entire physical scaled target. */
 {
  std::vector<uint32_t> image(640*480,0xFFFF0000);
  std::vector<float> depths(640*480,1);
  NvGpuDraw scaled={};scaled.color=(uint8_t*)image.data();scaled.depth=depths.data();
  scaled.mode=2;scaled.mask=0x01010101;scaled.src=1;scaled.zscale=1;
  scaled.depth_test=scaled.depth_write=1;scaled.depth_func=0x201;
  scaled.width=scaled.clip_w=640;scaled.height=scaled.clip_h=480;
  scaled.pitch=640*4;scaled.depth_stride=640;
  internal=1;CHECK(nv_gpu_begin(&scaled));quad(.2,1,0,0,1);
  NvGpuClear clear={};clear.color=scaled.color;clear.depth=scaled.depth;
  clear.pitch=scaled.pitch;clear.width=640;clear.height=480;
  clear.color_value=0xFF2468AC;clear.depth_value=1;
  uint64_t reads=readbacks;CHECK(nv_gpu_clear_attachments(&clear));
  std::fill(image.begin(),image.end(),clear.color_value);std::fill(depths.begin(),depths.end(),1);
  CHECK(nv_gpu_begin(&scaled));CHECK(readbacks==reads);quad(.7,0,1,0,1);nv_gpu_sync();
  CHECK(image[400*640+500]==clear.color_value && image[32*640+32]==0xFF00FF00);
  CHECK(fabs(depths[32*640+32]-.7)<1e-6 && depths[400*640+500]==1);
  internal=0;
 }
 /* Same-size surfaces retain independent GPU colour/depth contents. CPU
  * mutations invalidate each attachment independently at the next rebind. */
 {
  std::vector<uint32_t> a(64*64,0),b(64*64,0);
  std::vector<float> za(64*64,1),zb(64*64,1);
  NvGpuDraw surface={};surface.width=surface.height=surface.clip_w=surface.clip_h=64;
  surface.pitch=256;surface.depth_stride=64;surface.mode=2;
  surface.mask=0x01010101;surface.src=1;surface.zscale=1;
  surface.depth_test=surface.depth_write=1;surface.depth_func=0x201;
  surface.color=(uint8_t*)a.data();surface.depth=za.data();
  CHECK(nv_gpu_begin(&surface));quad(.2,1,0,0,1);nv_gpu_sync();
  ID3D11Texture2D *first_color=color,*first_depth=depth;
  surface.color=(uint8_t*)b.data();surface.depth=zb.data();
  CHECK(nv_gpu_begin(&surface));quad(.8,0,1,0,1);nv_gpu_sync();
  surface.color=(uint8_t*)a.data();surface.depth=za.data();
  uint64_t kept_color=resident_color_hits,kept_depth=resident_depth_hits;
  CHECK(nv_gpu_begin(&surface));
  if(resident_surfaces_enabled()){
   CHECK(color==first_color && depth==first_depth);
   CHECK(resident_color_hits==kept_color+1 && resident_depth_hits==kept_depth+1);
  }
  quad(.5,0,0,1,1);nv_gpu_sync();CHECK(a[32*64+32]==0xFFFF0000);
  std::fill(za.begin(),za.end(),1.f);kept_color=resident_color_hits;kept_depth=resident_depth_hits;
  CHECK(nv_gpu_begin(&surface));
  if(resident_surfaces_enabled())CHECK(resident_color_hits==kept_color+1 && resident_depth_hits==kept_depth);
  quad(.5,0,0,1,1);nv_gpu_sync();CHECK(a[32*64+32]==0xFF0000FF);
  a[32*64+32]=0xFFFFFFFF;surface.clip_w=16;surface.depth_test=surface.depth_write=0;
  kept_color=resident_color_hits;CHECK(nv_gpu_begin(&surface));
  if(resident_surfaces_enabled())CHECK(resident_color_hits==kept_color);
  quad(.1,0,1,0,1);nv_gpu_sync();
  CHECK(a[32*64+32]==0xFFFFFFFF && a[32*64+8]==0xFF00FF00);
  /* A layout change at the same address must not reuse the old GPU image. */
  surface.width=32;surface.clip_w=16;kept_color=resident_color_hits;
  CHECK(nv_gpu_begin(&surface));CHECK(resident_color_hits==kept_color);
  quad(.1,1,0,0,1);nv_gpu_sync();CHECK(a[32*64+8]==0xFFFF0000);
  /* Uniform CPU contents reproduce exact colour/depth values on the GPU
   * without uploading either attachment. Guest clear values above zscale
   * retain their CPU representation while the GPU stores normalized one. */
  surface.width=64;surface.clip_w=16;
  std::fill(a.begin(),a.end(),0x7F13579Bu);std::fill(za.begin(),za.end(),2.5f);
  uint64_t color_clears=resident_color_clears,depth_clears=resident_depth_clears;
  uint64_t color_bytes=surface_color_upload_bytes,depth_bytes=surface_depth_upload_bytes;
  CHECK(nv_gpu_begin(&surface));
  if(resident_surfaces_enabled()){
   CHECK(resident_color_clears==color_clears+1 && resident_depth_clears==depth_clears+1);
   CHECK(surface_color_upload_bytes==color_bytes && surface_depth_upload_bytes==depth_bytes);
  }
  quad(.1,0,1,0,1);nv_gpu_sync();
  CHECK(a[32*64+32]==0x7F13579B && a[32*64+8]==0xFF00FF00 && za[32*64+32]==2.5f);
  context->CopyResource(depth_read,depth);D3D11_MAPPED_SUBRESOURCE mapped={};
  CHECK(SUCCEEDED(context->Map(depth_read,0,D3D11_MAP_READ,0,&mapped)));
  CHECK(((const float*)((const char*)mapped.pData+32*mapped.RowPitch))[32]==1.f);
  context->Unmap(depth_read,0);
 }
 /* Bounded residency evicts old identities and reloads their CPU contents. */
 {
  std::vector<std::vector<uint32_t>> images(12,std::vector<uint32_t>(64*64,0xFFFF0000));
  NvGpuDraw surface={};surface.width=surface.height=surface.clip_w=surface.clip_h=64;
  surface.pitch=256;surface.mode=2;surface.mask=0x01010101;surface.src=1;surface.zscale=1;
  for(unsigned i=0;i<images.size();i++){
   surface.color=(uint8_t*)images[i].data();CHECK(nv_gpu_begin(&surface));quad(.5,1,0,0,1);nv_gpu_sync();
   CHECK(target_pool.size()<=8 && surface_pool_bytes()<=surface_pool_budget);
  }
  if(resident_surfaces_enabled())for(const auto &entry:target_pool)CHECK(entry.guest.color!=(uint8_t*)images[0].data());
  surface.color=(uint8_t*)images[0].data();surface.clip_w=16;
  CHECK(nv_gpu_begin(&surface));quad(.5,0,1,0,1);nv_gpu_sync();
  CHECK(images[0][32*64+32]==0xFFFF0000 && images[0][32*64+8]==0xFF00FF00);
 }
 /* Force indexed draws across DISCARD/NO_OVERWRITE and both ring wraps. */
 {
  std::vector<uint32_t> image(64*64,0);
  NvGpuDraw repeated={};repeated.color=(uint8_t*)image.data();
  repeated.width=repeated.height=repeated.clip_w=repeated.clip_h=64;
  repeated.pitch=64*4;repeated.mode=2;repeated.mask=0x01010101;repeated.src=1;repeated.zscale=1;
  quad_repetitions=32;uint64_t before=indexed_batches;
  for(unsigned i=0;i<180;i++){
   CHECK(nv_gpu_begin(&repeated));quad(.5,i==179?0:1,i==179?1:0,0,1);
   if(i==0){vertex_cursor=capacity-2;if(index_capacity)index_cursor=index_capacity-8;}
  }
  quad_repetitions=1;nv_gpu_sync();CHECK(image[32*64+32]==0xFF00FF00);
  CHECK(nv_gpu_indexed_upload_enabled()?indexed_batches==before+180:indexed_batches==before);
 }
 /* A failed optional index upload must expand the original triangle stream,
  * draw it directly, and allow a later indexed upload to recover. */
 if(nv_gpu_indexed_upload_enabled())
 {
  std::vector<uint32_t> image(64*64,0);NvGpuDraw fallback={};
  fallback.color=(uint8_t*)image.data();fallback.width=fallback.height=64;
  fallback.clip_w=fallback.clip_h=64;fallback.pitch=64*4;
  fallback.mode=2;fallback.mask=0x01010101;fallback.src=1;fallback.zscale=1;
  release(index_buffer);index_cursor=0;index_capacity=32768;
  D3D11_BUFFER_DESC wrong={};wrong.ByteWidth=index_capacity*4;
  wrong.BindFlags=D3D11_BIND_INDEX_BUFFER;wrong.Usage=D3D11_USAGE_DEFAULT;
  CHECK(SUCCEEDED(device->CreateBuffer(&wrong,nullptr,&index_buffer)));
  quad_repetitions=32;uint64_t before=indexed_batches;
  CHECK(nv_gpu_begin(&fallback));quad(.5,1,0,0,1);nv_gpu_sync();
  CHECK(indexed_batches==before && image[32*64+32]==0xFFFF0000);
  release(index_buffer);index_capacity=index_cursor=0;
  CHECK(nv_gpu_begin(&fallback));quad(.5,0,1,0,1);nv_gpu_sync();
  CHECK(indexed_batches==before+1 && image[32*64+32]==0xFF00FF00);
  quad_repetitions=1;
 }
 /* Repeated batches reuse bindings/constants; readback must force a rebind.
  * The second draw also exercises changed vertex data with unchanged state. */
 {
  std::vector<uint32_t> image(64*64,0);
  NvGpuDraw repeated={};repeated.color=(uint8_t*)image.data();
  repeated.width=repeated.height=repeated.clip_w=repeated.clip_h=64;
  repeated.pitch=64*4;repeated.mode=2;repeated.mask=0x01010101;
  repeated.src=1;repeated.zscale=1;
  CHECK(nv_gpu_begin(&repeated));quad(.5,1,0,0,1);
  uint64_t bindings=pipeline_binds,constants=constant_uploads;
  CHECK(nv_gpu_begin(&repeated));quad(.5,0,1,0,1);
  if(submission_cache_enabled())CHECK(pipeline_binds==bindings && constant_uploads==constants);
  else CHECK(pipeline_binds>bindings && constant_uploads>constants);
  nv_gpu_sync();CHECK(image[32*64+32]==0xFF00FF00);
  bindings=pipeline_binds;constants=constant_uploads;
  CHECK(nv_gpu_begin(&repeated));quad(.5,0,0,1,1);nv_gpu_sync();
  CHECK(pipeline_binds>bindings && image[32*64+32]==0xFF0000FF);
  if(submission_cache_enabled())CHECK(constant_uploads==constants);
  /* Scissor changes must take effect even when other pipeline state matches. */
  repeated.clip_w=16;
  CHECK(nv_gpu_begin(&repeated));quad(.5,1,0,0,1);nv_gpu_sync();
  CHECK(image[32*64+8]==0xFFFF0000 && image[32*64+32]==0xFF0000FF);
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
project=project.replace('</ItemDefinitionGroup>','<Link><AdditionalDependencies>d3d11.lib;d3dcompiler.lib;dxguid.lib;%(AdditionalDependencies)</AdditionalDependencies></Link></ItemDefinitionGroup>')
(out/'gpu_raster.vcxproj').write_text(project)
subprocess.run([msbuild,str(out/'gpu_raster.vcxproj'),'/p:Configuration=Release','/p:Platform=x64','/verbosity:minimal'],check=True)
subprocess.run([str(out/'bin/gpu_raster.exe')],check=True)
subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_RESOLVE':'0','RECOMP_GPU_DEPTH_RESOLVE':'0'})


subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_SIMPLE_SHADERS':'0'})

subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_RENDERED_TEXTURES':'0'})

subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_STATE_CACHE':'0'})

subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_INDEXED_UPLOAD':'0'})

subprocess.run([str(out/'bin/gpu_raster.exe')],check=True,env={**os.environ,'RECOMP_GPU_RESIDENT_SURFACES':'0'})
