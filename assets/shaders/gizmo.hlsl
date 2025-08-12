// SDL3 requires space1 for all cbuffer declarations

//@ VERTEX

#include "../../shaders/mesh.hlsl"

struct VertexOutput
{
    float3 normal : TEXCOORD0;
    float2 uv0 : TEXCOORD1;
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    // Apply bone skinning to position (limit to 16 bones)
    int boneIndex = int(input.boneIndex.x) % 16; // Clamp to 16 bones
    float4x4 boneTransform = boneTransforms[boneIndex];
    float4 skinnedPosition = mul(boneTransform, float4(input.position, 1.0));
    
    // Transform normal by bone (using 3x3 matrix)
    float3 skinnedNormal = mul((float3x3) boneTransform, input.normal);
    
    float4x4 mvp = mul(vp, m);
    float4x4 lightMVP = mul(lightViewProjection, m);
    
    float3x3 mv = (float3x3) mul(v, m);
    
    // Apply model-view-projection
    VertexOutput output;
    output.position = mul(mvp, float4(input.position, 1.0));
    output.normal = normalize(mul((float3x3) m, input.normal));
    output.uv0 = input.uv0;
    return output;
}

//@ END

//@ FRAGMENT

#include "../../shaders/light.hlsl"

Texture2D diffuseTexture : register(t0, space2);
SamplerState diffuseSampler : register(s0, space2);

struct PixelInput
{
    float3 normal : TEXCOORD0;
    float2 uv0 : TEXCOORD1;
};

float4 ps(PixelInput input) : SV_TARGET
{
    // Sample texture
    float4 texColor = diffuseTexture.Sample(diffuseSampler, input.uv0);
    float diffuse = -dot(input.normal, lightDirection);
    diffuse = 1;
    return float4(texColor.rgb * (0.5 + diffuse * 0.5), texColor.a);
}

//@ END
