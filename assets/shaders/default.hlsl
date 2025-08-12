// SDL3 requires space1 for all cbuffer declarations

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
    output.position = mul(vp, mul(m, float4(input.position, 1.0)));
    output.uv0 = float2(input.uv0.x, 1.0 - input.uv0.y);
    return output;
}

//@ END

//@ FRAGMENT

Texture2D<float4> Texture : register(t1, space2);
SamplerState Sampler : register(s1, space2);

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

float4 ps(PixelInput input) : SV_TARGET
{
    float4 texColor = Texture.Sample(Sampler, input.uv0);
    return float4(texColor.rgb, 1);
}

//@ END