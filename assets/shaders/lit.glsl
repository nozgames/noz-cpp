#version 450

//@ VERTEX

layout(set = 0, binding = 0, row_major) uniform CameraBuffer
{
    mat3 view_projection;
} camera;

layout(set = 1, binding = 0, row_major) uniform ObjectBuffer
{
    mat3 transform;
    float depth;
} object;

layout(location = 0) in vec2 v_position;
layout(location = 1) in vec2 v_uv0;
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec3 f_normal;

void main() {
    // Position  
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    gl_Position = vec4(screen_pos.xy, object.depth, 1.0);

    // Uv
    f_uv = v_uv0;

    // Normal
    vec2 transform_right = object.transform[0].xy;
    vec2 transform_up = object.transform[1].xy;
    vec3 world_normal = vec3(
        dot(v_normal.xy, vec2(transform_right.x, transform_right.y)),
        dot(v_normal.xy, vec2(transform_up.x, transform_up.y)),
        v_normal.z
    );

    f_normal = vec3(normalize(vec3(v_normal.xy, 0) * object.transform).xy, v_normal.z);
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec3 f_normal;
layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0) uniform ColorBuffer {
    vec4 color;
    vec2 uv_offset;
    vec2 padding;
} color_buffer;

layout(set = 4, binding = 0) uniform LightBuffer {
    vec3 direction;
    float padding;
    vec4 diffuse_color;
    vec4 shadow_color;
} light_buffer;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

void main() {
    vec4 color = texture(mainTexture, f_uv + color_buffer.uv_offset) * color_buffer.color;
    outColor = color;
}

//@ END
