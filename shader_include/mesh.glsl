layout(set = 0, binding = 0) uniform CameraBuffer
{
    mat3 view_projection;
} camera;

layout(set = 0, binding = 1) uniform ObjectBuffer
{
    mat3 transform;
} object;

// Vertex input attributes
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 normal;
layout(location = 4) in float bone_index;

vec4 transform_to_screen(vec2 vertex_pos)
{
    // Combined transform: view-projection * object
    mat3 mvp = camera.view_projection * object.transform;

    // Transform vertex directly to screen space
    vec3 screenPos = mvp * vec3(vertex_pos, 1.0);

    return vec4(screenPos.xy, 0.0, 1.0);
}
