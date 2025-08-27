// SDL3 requires space1 for all cbuffer declarations

//@ VERTEX

#include "../../shader_include/mesh.hlsl"

struct VertexOutput
{
    float3 normal : TEXCOORD0;
    float2 uv0 : TEXCOORD1;
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    float4x4 bone = bones[input.bone_index % 32];
    float4 skinned_position = mul(bone, float4(input.position, 1.0));
    float3 skinned_normal = normalize(mul((float3x3) bone, input.normal));
    float4 world_position = mul(m, skinned_position);
    float3 world_normal = normalize(mul((float3x3) m, skinned_normal));
        
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

#include "../../shader_include/light.hlsl"

Texture2D diffuseTexture : register(t1, space2);
SamplerState diffuseSampler : register(s1, space2);

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
