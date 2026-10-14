/**
 * NV2A register combiners, evaluated per pixel for the software rasteriser.
 *
 * Every pixel the NV2A writes goes through its register combiners: up to
 * eight general stages and a final stage that mix the vertex colours, the
 * four texture results, two constants per stage and fog. D3D's fixed-function
 * texture stages compile to them too, so a title never draws without them.
 * The executor used to stand in with "texel * diffuse" from stage 0, which is
 * right for a menu quad and wrong for anything that mixes two textures --
 * Burnout 3 composites its whole 3D scene into the frame that way.
 *
 * The encoding (ICW/OCW bit layout, input mappings, output scale/bias, mux,
 * final combiner) follows xemu's pixel-shader generator
 * (hw/xbox/nv2a/pgraph/glsl/psh.c); the comments in nv2a_combiner.c say where
 * each rule comes from. Portable C, no graphics API.
 */
#ifndef XBOXRECOMP_NV2A_COMBINER_H
#define XBOXRECOMP_NV2A_COMBINER_H

#include <stdint.h>

typedef struct {
    uint32_t color_icw[8], alpha_icw[8];    /* SET_COMBINER_COLOR/ALPHA_ICW */
    uint32_t color_ocw[8], alpha_ocw[8];    /* SET_COMBINER_COLOR/ALPHA_OCW */
    uint32_t factor0[8], factor1[8];        /* SET_COMBINER_FACTOR0/1 (ARGB) */
    uint32_t final0, final1;                /* SET_COMBINER_SPECULAR_FOG_CW0/1 */
    uint32_t final_c0, final_c1;            /* SET_SPECULAR_FOG_FACTOR */
    uint32_t control;                       /* SET_COMBINER_CONTROL */
    uint32_t stage_program;                 /* SET_SHADER_STAGE_PROGRAM */
} Nv2aCombiner;

/* One pixel. Colours are r,g,b,a in [0,1]; `fog` is (fog colour, factor).
 * t[i] is what texture stage i produced. The result is clamped to [0,1]. */
void nv2a_rc_eval(const Nv2aCombiner *rc, const float v0[4],
                  const float v1[4], const float fog[4],
                  const float t[4][4], float out[4]);

/* Recognize the exact one-stage modulate/diffuse program once per triangle.
 * Mode zero requires the general evaluator. Texture-stage clipping remains
 * the caller's responsibility, including stages whose colour is unused. */
int nv2a_rc_simple_mode(const Nv2aCombiner *rc);
void nv2a_rc_eval_simple(int mode, const float v0[4], const float fog[4],
                         const float t0[4], float out[4]);

/* D3DCOLOR (0xAARRGGBB) to r,g,b,a floats. */
void nv2a_rc_unpack(uint32_t argb, float out[4]);

/* Exactly rounded byte/255 values, without per-tap floating divisions or
 * an out-of-line call. Immutable so raster workers share it safely. */
static inline void nv2a_rc_unpack_fast(uint32_t argb, float out[4])
{
    static const float unit_byte[256] = {
        0.00000000f, 0.00392156886f, 0.00784313772f, 0.0117647061f, 0.0156862754f, 0.0196078438f, 0.0235294122f, 0.0274509806f,
        0.0313725509f, 0.0352941193f, 0.0392156877f, 0.0431372561f, 0.0470588244f, 0.0509803928f, 0.0549019612f, 0.0588235296f,
        0.0627451017f, 0.0666666701f, 0.0705882385f, 0.0745098069f, 0.0784313753f, 0.0823529437f, 0.0862745121f, 0.0901960805f,
        0.0941176489f, 0.0980392173f, 0.101960786f, 0.105882354f, 0.109803922f, 0.113725491f, 0.117647059f, 0.121568628f,
        0.125490203f, 0.129411772f, 0.133333340f, 0.137254909f, 0.141176477f, 0.145098045f, 0.149019614f, 0.152941182f,
        0.156862751f, 0.160784319f, 0.164705887f, 0.168627456f, 0.172549024f, 0.176470593f, 0.180392161f, 0.184313729f,
        0.188235298f, 0.192156866f, 0.196078435f, 0.200000003f, 0.203921571f, 0.207843140f, 0.211764708f, 0.215686277f,
        0.219607845f, 0.223529413f, 0.227450982f, 0.231372550f, 0.235294119f, 0.239215687f, 0.243137255f, 0.247058824f,
        0.250980407f, 0.254901975f, 0.258823544f, 0.262745112f, 0.266666681f, 0.270588249f, 0.274509817f, 0.278431386f,
        0.282352954f, 0.286274523f, 0.290196091f, 0.294117659f, 0.298039228f, 0.301960796f, 0.305882365f, 0.309803933f,
        0.313725501f, 0.317647070f, 0.321568638f, 0.325490206f, 0.329411775f, 0.333333343f, 0.337254912f, 0.341176480f,
        0.345098048f, 0.349019617f, 0.352941185f, 0.356862754f, 0.360784322f, 0.364705890f, 0.368627459f, 0.372549027f,
        0.376470596f, 0.380392164f, 0.384313732f, 0.388235301f, 0.392156869f, 0.396078438f, 0.400000006f, 0.403921574f,
        0.407843143f, 0.411764711f, 0.415686280f, 0.419607848f, 0.423529416f, 0.427450985f, 0.431372553f, 0.435294122f,
        0.439215690f, 0.443137258f, 0.447058827f, 0.450980395f, 0.454901963f, 0.458823532f, 0.462745100f, 0.466666669f,
        0.470588237f, 0.474509805f, 0.478431374f, 0.482352942f, 0.486274511f, 0.490196079f, 0.494117647f, 0.498039216f,
        0.501960814f, 0.505882382f, 0.509803951f, 0.513725519f, 0.517647088f, 0.521568656f, 0.525490224f, 0.529411793f,
        0.533333361f, 0.537254930f, 0.541176498f, 0.545098066f, 0.549019635f, 0.552941203f, 0.556862772f, 0.560784340f,
        0.564705908f, 0.568627477f, 0.572549045f, 0.576470613f, 0.580392182f, 0.584313750f, 0.588235319f, 0.592156887f,
        0.596078455f, 0.600000024f, 0.603921592f, 0.607843161f, 0.611764729f, 0.615686297f, 0.619607866f, 0.623529434f,
        0.627451003f, 0.631372571f, 0.635294139f, 0.639215708f, 0.643137276f, 0.647058845f, 0.650980413f, 0.654901981f,
        0.658823550f, 0.662745118f, 0.666666687f, 0.670588255f, 0.674509823f, 0.678431392f, 0.682352960f, 0.686274529f,
        0.690196097f, 0.694117665f, 0.698039234f, 0.701960802f, 0.705882370f, 0.709803939f, 0.713725507f, 0.717647076f,
        0.721568644f, 0.725490212f, 0.729411781f, 0.733333349f, 0.737254918f, 0.741176486f, 0.745098054f, 0.749019623f,
        0.752941191f, 0.756862760f, 0.760784328f, 0.764705896f, 0.768627465f, 0.772549033f, 0.776470602f, 0.780392170f,
        0.784313738f, 0.788235307f, 0.792156875f, 0.796078444f, 0.800000012f, 0.803921580f, 0.807843149f, 0.811764717f,
        0.815686285f, 0.819607854f, 0.823529422f, 0.827450991f, 0.831372559f, 0.835294127f, 0.839215696f, 0.843137264f,
        0.847058833f, 0.850980401f, 0.854901969f, 0.858823538f, 0.862745106f, 0.866666675f, 0.870588243f, 0.874509811f,
        0.878431380f, 0.882352948f, 0.886274517f, 0.890196085f, 0.894117653f, 0.898039222f, 0.901960790f, 0.905882359f,
        0.909803927f, 0.913725495f, 0.917647064f, 0.921568632f, 0.925490201f, 0.929411769f, 0.933333337f, 0.937254906f,
        0.941176474f, 0.945098042f, 0.949019611f, 0.952941179f, 0.956862748f, 0.960784316f, 0.964705884f, 0.968627453f,
        0.972549021f, 0.976470590f, 0.980392158f, 0.984313726f, 0.988235295f, 0.992156863f, 0.996078432f, 1.00000000f,
    };
    out[0] = unit_byte[(argb >> 16) & 255];
    out[1] = unit_byte[(argb >> 8) & 255];
    out[2] = unit_byte[argb & 255];
    out[3] = unit_byte[argb >> 24];
}

#endif
