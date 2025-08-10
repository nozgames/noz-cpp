// SDL3 requires space1 for all cbuffer declarations

//@ VERTEX

cbuffer CameraBuffer : register(vs_b0, space1)
{
    float4x4 vp;
    float4x4 v;
    float4x4 lightViewProjection;
};

cbuffer ObjectBuffer : register(vs_b1, space1)
{
    float4x4 m;
};

struct VertexInput
{
    float3 position : POSITION;
    float2 uv0 : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

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

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer TextBuffer : register(ps_b0, space3)
{
    float4 textColor;        // Text color
    float4 outlineColor;     // Outline color
    float outlineWidth;      // Outline width (0-1)
    float smoothing;         // Smoothing factor for anti-aliasing
};

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

float4 ps(PixelInput input) : SV_TARGET
{
    // Sample the R8_UNORM channel (values in range [0, 1])
    float distance = Texture.Sample(Sampler, input.uv0).r;
    float width = fwidth(distance);
    float alpha = smoothstep(0.5 - width, 0.5 + width, distance);
    return float4(textColor.rgb, alpha * textColor.a);
}

//@ END