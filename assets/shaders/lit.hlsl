//@ VERTEX

#include "../../shaders/mesh.hlsl"

struct VertexOutput
{
    float4 lightSpacePos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv0 : TEXCOORD2;
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    // Get bone transform (clamp to valid range)
    int boneIndex = int(input.boneIndex.x) % 32;
    float4x4 boneTransform = boneTransforms[boneIndex];
    
    // Apply bone transformation to vertex position and normal
    float4 skinnedPosition = mul(boneTransform, float4(input.position, 1.0));
    float3 skinnedNormal = normalize(mul((float3x3)boneTransform, input.normal));
    
    // Apply model transformation
    float4 worldPosition = mul(m, skinnedPosition);
    float3 worldNormal = normalize(mul((float3x3)m, skinnedNormal));

    VertexOutput output;
    output.position = mul(vp, worldPosition);
    output.normal = worldNormal;
    output.uv0 = float2(input.uv0.x, 1.0 - input.uv0.y);
    output.lightSpacePos = mul(lightViewProjection, worldPosition);
    
    return output;
}

//@ END
//@ FRAGMENT

#include "../../shaders/light.hlsl"
#include "../../shaders/shadow.hlsl"

Texture2D<float4> Texture : register(t1, space2);
SamplerState Sampler : register(s1, space2);

struct PixelInput
{
    float4 lightSpacePos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv0 : TEXCOORD2;
    float4 position : SV_Position;
};

float4 ps(PixelInput input) : SV_TARGET
{
    // Sample texture
    float4 texColor = Texture.Sample(Sampler, input.uv0);
    
    // Calculate light
    float3 ambient = ambientColor * ambientIntensity;
    float diffuse = -dot(input.normal, lightDirection) + 0.5 * 0.5;
    float3 diffuseLight = diffuseColor * diffuseIntensity * diffuse;
    
    // Calculate shadow with normal-based bias
    float shadow = calculateShadow(input.lightSpacePos, input.normal);
    
    // Combine lighting with texture and shadow
    float3 finalColor = (ambient + diffuseLight * (0.8 + shadow * 0.2)) * texColor.rgb;
    return float4(finalColor, texColor.a);
}

//@ END
