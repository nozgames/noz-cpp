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

cbuffer VignetteBuffer : register(b0, space3)
{
    float vignetteColorR;   // R component
    float vignetteColorG;   // G component  
    float vignetteColorB;   // B component
    float intensity;        // Alpha/intensity of the effect
    float radius;          // Radius where vignette starts
    float softness;        // Softness of the vignette transition
    float padding1;        // Padding
    float padding2;        // Padding
};

struct PixelInput
{
    float2 uv0 : TEXCOORD0;
};

// Simple hash function for noise generation
float hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float4 ps(PixelInput input) : SV_TARGET
{
    // Reconstruct color from individual components
    float3 vignetteColor = float3(vignetteColorR, vignetteColorG, vignetteColorB);
    
    // Calculate distance from center (0.5, 0.5)
    float2 center = float2(0.5, 0.5);
    float2 dist = input.uv0 - center;
    
    // Account for aspect ratio to keep vignette circular
    dist.x *= 1.7777; // 16:9 aspect ratio, adjust as needed
    
    float distance = length(dist);
    
    // Calculate vignette factor (0 = no darkening, 1 = full dark)
    float vignette = smoothstep(radius - softness, radius, distance);
    
    // Disable dithering for now to debug the gradient issue
    //float2 screenPos = input.uv0 * 1024.0; // Scale UV to pixel-like coordinates
    //float noise = hash(screenPos) - 0.5; // Center noise around 0
    //float ditherAmount = 0.01; // Stronger dithering to break up 8-bit quantization
    //vignette += noise * ditherAmount;
    
    // Clamp to prevent over/under saturation
    vignette = saturate(vignette);
    
    // Output vignette color with alpha based on intensity
    // Alpha blending will apply the colored vignette over the scene
    return float4(vignetteColor, vignette * intensity);
}

//@ END