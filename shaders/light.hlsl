cbuffer ColorBuffer : register(b0, space3)
{
    float4 color;
};

cbuffer LightBuffer : register(b1, space3)
{
    float3 ambientColor;
    float ambientIntensity;
    float3 diffuseColor;
    float diffuseIntensity;
    float3 lightDirection;
    float shadowBias;
};
