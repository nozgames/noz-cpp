layout(set = 0, binding = 0) uniform CameraBuffer
{
    vec2 cam_pos;
    vec2 cam_size;
    vec2 cam_rot;
} camera;

layout(set = 0, binding = 1) uniform ObjectBuffer
{
    vec2 obj_pos;
    vec2 obj_scale;
    float obj_rot;
} object;

layout(set = 0, binding = 2) uniform BoneBuffer
{
    mat3 bones[32];
} boneData;

// Vertex input attributes
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec2 normal;
layout(location = 3) in float bone_index;

vec4 transform_to_screen(vec2 vertex_pos)
{
    // Scale and rotate the vertex
    vec2 scaled = vertex_pos * object.obj_scale;
    float obj_cos = cos(object.obj_rot);
    float obj_sin = sin(object.obj_rot);
    vec2 rotated = vec2(
        dot(scaled, vec2(obj_cos, -obj_sin)),
        dot(scaled, vec2(obj_sin, obj_cos))
    );

    // Position in world, then transform by camera
    vec2 world = rotated + object.obj_pos;
    vec2 translated = world - camera.cam_pos;
    vec2 view = vec2(
        dot(translated, vec2(camera.cam_rot.y, -camera.cam_rot.x)),
        dot(translated, camera.cam_rot)
    );

    // Convert to NDC (-1 to 1 range)
    return vec4(
        view.x / camera.cam_size.x,
        view.y / camera.cam_size.y,
        0.0,
        1.0
    );
}
