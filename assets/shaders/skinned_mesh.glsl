#version 450

//@ VERTEX

layout(set = 0, binding = 0, row_major) uniform CameraBuffer {
    mat3 view_projection;
} camera;

layout(set = 1, binding = 0, row_major) uniform ObjectBuffer {
    mat3 transform;
    float depth;
    float depth_scale;
    float depth_min;
    float depth_max;
} object;

layout(set = 2, binding = 0, row_major) uniform SkeletonBuffer {
    mat3 bones[32];
} skeleton;

layout(location = 0) in vec2 v_position;
layout(location = 1) in float v_depth;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in ivec4 v_bones_indices;
layout(location = 4) in vec4 v_bone_weights;

layout(location = 0) out vec2 f_uv;

void main() {
    mat3 skin_matrix = skeleton.bones[v_bones_indices.x] * v_bone_weights.x
        + skeleton.bones[v_bones_indices.y] * v_bone_weights.y
        + skeleton.bones[v_bones_indices.z] * v_bone_weights.z
        + skeleton.bones[v_bones_indices.w] * v_bone_weights.w;
    mat3 mvp = object.transform * camera.view_projection * skin_matrix;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    float depth = (object.depth + (v_depth * object.depth_scale) - object.depth_min) / (object.depth_max - object.depth_min);
    gl_Position = vec4(screen_pos.xy, 1.0f - depth, 1.0);
    f_uv = v_uv;
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform ColorBuffer {
    vec4 color;
    vec4 emission;
    vec2 uv_offset;
    vec2 padding;
} color_buffer;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

void main() {
    vec4 diffuse = texture(mainTexture, f_uv + color_buffer.uv_offset) * color_buffer.color;
    outColor = vec4(mix(diffuse.rgb, color_buffer.emission.rgb, color_buffer.emission.a), diffuse.a);
}

//@ END

