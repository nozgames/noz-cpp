cbuffer LightBuffer : register(b4, space3)
{
    float3 ambientColor;
    float ambientIntensity;
    float3 diffuseColor;
    float diffuseIntensity;
    float3 lightDirection;
    float shadowBias;
};
