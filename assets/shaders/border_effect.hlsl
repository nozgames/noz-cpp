// Border Effect Shader for Icon Generation
// Adds a configurable border to rendered icons

//@ VERTEX

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
    output.position = float4(input.position.xy, 0.0, 1.0);
    output.uv0 = input.uv0;
    return output;
}

//@ END

//@ FRAGMENT

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer BorderBuffer : register(b0, space3)
{
    float borderWidth;      // Border width in pixels
    float borderColorR;     // Border color R
    float borderColorG;     // Border color G
    float borderColorB;     // Border color B
    float borderAlpha;      // Border alpha/opacity
    float textureWidth;     // Texture width in pixels
    float textureHeight;    // Texture height in pixels
    float padding;          // Padding for alignment
};

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

// Check if a pixel is solid (has alpha above threshold)
bool isSolid(float2 uv)
{
    float alpha = Texture.Sample(Sampler, uv).a;
    return alpha > 0.01;
}

float4 ps(PixelInput input) : SV_TARGET
{
    float2 textureSize = float2(textureWidth, textureHeight);
    float2 texelSize = 1.0 / textureSize;
    
    // Sample the original texture
    float4 originalColor = Texture.Sample(Sampler, input.uv0);
    bool currentPixelSolid = originalColor.a > 0.01;
    
    // If this pixel is transparent, check if we're near a solid edge (for outer border)
    float borderMask = 0.0;
    if (!currentPixelSolid)
    {
        // Check surrounding pixels in a radius for solid boundaries
        float radius = borderWidth;
        for (float x = -radius; x <= radius; x += 1.0)
        {
            for (float y = -radius; y <= radius; y += 1.0)
            {
                if (x == 0.0 && y == 0.0) continue; // Skip center pixel
                
                float2 sampleUV = input.uv0 + float2(x, y) * texelSize;
                
                // Skip if outside texture bounds
                if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
                    continue;
                
                // Check if this neighbor is solid
                if (isSolid(sampleUV))
                {
                    float distance = length(float2(x, y));
                    if (distance <= radius)
                    {
                        // Smooth falloff based on distance
                        float falloff = 1.0 - smoothstep(radius - 1.0, radius, distance);
                        borderMask = max(borderMask, falloff);
                    }
                }
            }
        }
    }
    
    // Create the border color
    float3 borderColor = float3(borderColorR, borderColorG, borderColorB);
    
    // If we have border, show border color, otherwise show original
    float3 finalColor = currentPixelSolid ? originalColor.rgb : lerp(originalColor.rgb, borderColor, borderMask);
    float finalAlpha = currentPixelSolid ? originalColor.a : max(originalColor.a, borderMask * borderAlpha);
    
    return float4(finalColor, finalAlpha);
}

//@ END