//@ VERTEX

#include "../../shader_include/mesh.hlsl"

struct VertexOutput
{
    float4 lightSpacePos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv0 : TEXCOORD2;
    float4 position : SV_POSITION;
};

VertexOutput vs(VertexInput input)
{
    float4x4 bone = bones[input.bone_index % 32];
    float4 skinned_position = mul(bone, float4(input.position, 1.0));
    float3 skinned_normal = normalize(mul((float3x3) bone, input.normal));
    float4 world_position = mul(m, skinned_position);
    float3 world_normal = normalize(mul((float3x3) m, skinned_normal));

    VertexOutput output;
    output.position = mul(vp, world_position);
    output.normal = world_normal;
    output.uv0 = float2(input.uv0.x, 1.0 - input.uv0.y);
    output.lightSpacePos = mul(lightViewProjection, world_position);
    
    return output;
}

//@ END
//@ FRAGMENT

#include "../../shader_include/light.hlsl"
#include "../../shader_include/shadow.hlsl"

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
