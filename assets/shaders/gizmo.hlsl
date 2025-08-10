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
    float2 uv0 : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 boneIndex : TEXCOORD2; // Single bone index per vertex
};

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

Texture2D diffuseTexture : register(t0, space2);
SamplerState diffuseSampler : register(s0, space2);

cbuffer LightBuffer : register(b0, space3)
{
    float3 lightDirection;
    float ambientIntensity;
    float3 ambientColor;
    float diffuseIntensity;
    float3 diffuseColor;
    float padding; // Padding to align to 16-byte boundary
};

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