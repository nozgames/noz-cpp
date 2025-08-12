cbuffer CameraBuffer : register(b0, space1)
{
    float4x4 vp;
    float4x4 v;
    float4x4 lightViewProjection;
};

cbuffer ObjectBuffer : register(b1, space1)
{
    float4x4 m;
};

cbuffer BoneBuffer : register(b2, space1)
{
    float4x4 boneTransforms[32];
};

struct VertexInput
{
    float3 position : POSITION;
    float2 uv0 : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 boneIndex : TEXCOORD2;
};
