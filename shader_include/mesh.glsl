cbuffer CameraBuffer : register(b0, space0)
{
    float2 cam_pos;
    float2 cam_size;
    float2 cam_rot;
};

cbuffer ObjectBuffer : register(b1, space0)
{
    float2 obj_pos;
    float2 obj_scale;
    float obj_rot;
};

cbuffer BoneBuffer : register(b2, space0)
{
    float3x3 bones[32];
};

struct VertexInput
{
    float2 position : POSITION;
    float2 uv0 : TEXCOORD0;
    float2 normal : TEXCOORD1;
    float bone_index : TEXCOORD2;
};

float4 transform_to_screen(float2 vertex_pos)
{
    // Scale and rotate the vertex
    float2 scaled = vertex_pos * obj_scale;
    float obj_cos = cos(obj_rot);
    float obj_sin = sin(obj_rot);
    float2 rotated = float2(
        dot(scaled, float2(obj_cos, -obj_sin)),
        dot(scaled, float2(obj_sin, obj_cos))
    );

    // Position in world, then transform by camera
    float2 world = rotated + obj_pos;
    float2 translated = world - cam_pos;
    float2 view = float2(
        dot(translated, float2(cam_rot.y, -cam_rot.x)),
        dot(translated, cam_rot)
    );

    // Convert to NDC (-1 to 1 range)
    return float4(
        view.x / cam_size.x,
        view.y / cam_size.y,
        0.0,
        1.0
    );
}
