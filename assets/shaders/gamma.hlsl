// SDL3 requires space1 for all cbuffer declarations

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
    // For fullscreen effects, we use the vertex positions directly as NDC coordinates
    output.position = float4(input.position.xy, 0.0, 1.0);
    output.uv0 = input.uv0;
    return output;
}

//@ END

//@ FRAGMENT

Texture2D<float4> sceneTexture : register(t1, space2);
SamplerState sceneSampler : register(s1, space2);

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

// Convert linear to sRGB gamma space
float3 linearToSRGB(float3 color)
{
    // Standard sRGB gamma conversion
    float3 result;
    result.x = color.x <= 0.0031308 ? color.x * 12.92 : 1.055 * pow(color.x, 1.0/2.4) - 0.055;
    result.y = color.y <= 0.0031308 ? color.y * 12.92 : 1.055 * pow(color.y, 1.0/2.4) - 0.055;
    result.z = color.z <= 0.0031308 ? color.z * 12.92 : 1.055 * pow(color.z, 1.0/2.4) - 0.055;
    return result;
}

float4 ps(PixelInput input) : SV_TARGET
{
    // Sample the linear color from the scene
    float4 linearColor = sceneTexture.Sample(sceneSampler, input.uv0);
    
    // Convert to sRGB for display
    float3 srgbColor = linearToSRGB(linearColor.rgb);
    
    return float4(srgbColor, linearColor.a);
}

//@ END