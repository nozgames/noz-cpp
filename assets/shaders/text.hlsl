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

Texture2D<float4> Texture : register(t0, space1);
SamplerState Sampler : register(s0, space1);

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

float4 ps(PixelInput input) : SV_TARGET
{
    float distance = Texture.Sample(Sampler, input.uv0).r;
    float width = fwidth(distance);
    float alpha = smoothstep(0.5 - width, 0.5 + width, distance);
    return float4(color.rgb, alpha * color.a);
}

//@ END