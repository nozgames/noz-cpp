Texture2D<float> NOZ_SHADOW_TEXTURE : register(t3, space2);
SamplerComparisonState NOZ_SHADOW_SAMPLER : register(s3, space2);

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
