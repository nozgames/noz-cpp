//@ VERTEX

#include "../../shaders/mesh.hlsl"

struct VertexOutput
{
    float2 uv0 : TEXCOORD0;
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    VertexOutput output;
    output.position = mul(mul(vp, m), float4(input.position, 1.0));
    output.uv0 = input.uv0;
    return output;
}

//@ END

//@ FRAGMENT

#include "../../shaders/color.hlsl"

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

float4 ps(PixelInput input) : SV_TARGET
{
    float4 texColor = Texture.Sample(Sampler, input.uv0);
    return color * texColor;
}

//@ END
