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

float calculateShadow(float4 lightSpacePos, float3 normal)
{
    // Perform perspective divide
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    
    // Transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;           
    projCoords.y = 1.0f - projCoords.y;
    
    // Check if fragment is outside light frustum
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 1.0;
    
    // Calculate slope-scale depth bias (Peter Panning prevention)
    // The bias increases based on the angle between the surface and light
    float NdotL = saturate(dot(normal, -lightDirection));
    float slopeBias = tan(acos(NdotL));
    slopeBias = clamp(slopeBias, 0, 0.01);
    
    // Combine constant bias with slope-scale bias
    // shadowBias is the base bias (typically 0.0005 to 0.002)
    float bias = shadowBias + slopeBias * shadowBias;
    bias = min(bias, 0.01); // Cap maximum bias to prevent light bleeding
    
    // Apply bias by subtracting from depth (closer to light)
    float currentDepth = projCoords.z - bias;
    
    // Use hardware shadow comparison
    // Returns 1.0 if currentDepth < shadowMap depth (lit), 0.0 otherwise (shadowed)
    return NOZ_SHADOW_TEXTURE.SampleCmp(NOZ_SHADOW_SAMPLER, projCoords.xy, currentDepth);
}

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
