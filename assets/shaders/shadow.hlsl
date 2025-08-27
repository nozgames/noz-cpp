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

cbuffer BoneBuffer : register(vs_b2, space1)
{
    float4x4 boneTransforms[16]; // Bone skinning matrices (world transform * inverse bind pose)
};

struct VertexInput
{
    float3 position : POSITION;
    float2 uv0 : TEXCOORD1;
    float3 normal : TEXCOORD0;
    float2 boneIndex : TEXCOORD2; // Single bone index per vertex
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    // Apply bone skinning to position (limit to 16 bones)
    int boneIndex = int(input.boneIndex.x) % 16; // Clamp to 16 bones
    float4x4 boneTransform = boneTransforms[boneIndex];
    
    // Apply bone transform to position
    float4 skinnedPosition = mul(boneTransform, float4(input.position, 1.0));
    float4x4 mvp = mul(lightViewProjection, m);
    
    // Apply light view-projection matrix directly
    VertexOutput output;
    output.position = mul(mvp, skinnedPosition);
    return output;
}

//@ END

//@ FRAGMENT

struct PixelInput
{
    float4 position : SV_POSITION;
};

void ps(PixelInput input)
{    
    // For shadow mapping, we only write to depth buffer
    // No color output needed - depth is written automatically
}

//@ END