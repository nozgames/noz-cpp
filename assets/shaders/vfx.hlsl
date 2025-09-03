//@ VERTEX

#include "../../shader_include/mesh.hlsl"

struct VertexOutput
{
    float2 uv0 : TEXCOORD0;
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    VertexOutput output;
    output.position = transform_to_screen(input.position);
    output.uv0 = input.uv0;
    return output;
}

//@ END

//@ FRAGMENT

#include "../../shader_include/color.hlsl"

Texture2D<float4> Texture : register(t1, space2);
SamplerState Sampler : register(s1, space2);

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

float4 ps(PixelInput input) : SV_TARGET
{
    return Texture.Sample(Sampler, input.uv0) * color;
}

//@ END
