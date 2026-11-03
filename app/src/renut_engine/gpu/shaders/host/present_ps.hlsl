#include "utility.hlsli"

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target0
{
    // kameo-nx (2026-11-03): all ones in the sampler index: the frame covers the output one to one (present.cpp), so
    // this pixel is that texel, what the filter gives at its centre.
    float4 color = g_Push.samplerIndex == 0xFFFFFFFFu
                       ? g_Textures[NonUniformResourceIndex(g_Push.textureIndex)].Load(int3(int2(input.position.xy), 0))
                       : SampleBindless(g_Push.textureIndex, g_Push.samplerIndex, input.uv);
    // param0 = output gamma exponent. kameo-nx (2026-11-03): exactly 1 skips the pow: an 8-bit value goes through it within
    // far less than half a level, and the ramp's index and the output are 8-bit. Negative: its size, with the pow.
    color.rgb = saturate(color.rgb);
    if (g_Push.param0 != 1.0)
        color.rgb = pow(color.rgb, abs(g_Push.param0));
    // banjo-nx (2026-10-20, banjo_renut_game_gamma): param1's bits = the bindless slot of the gamma ramp the game loaded
    // (256 x 1, its red, green and blue entries over 1023; 0 = none), looked up by the 8-bit value as the 360's display
    // does. The slot goes in as raw bits: Ryujinx follows a bindless handle only back to a constant buffer load, so one
    // computed from a float read zero (black) there (the SA2 port's finding).
    const uint ramp = asuint(g_Push.param1);
    if (ramp != 0)
    {
        const int3 v = int3(round(saturate(color.rgb) * 255.0));
        color.r = g_Textures[NonUniformResourceIndex(ramp)].Load(int3(v.r, 0, 0)).r;
        color.g = g_Textures[NonUniformResourceIndex(ramp)].Load(int3(v.g, 0, 0)).g;
        color.b = g_Textures[NonUniformResourceIndex(ramp)].Load(int3(v.b, 0, 0)).b;
    }
    return float4(color.rgb, 1.0);
}
