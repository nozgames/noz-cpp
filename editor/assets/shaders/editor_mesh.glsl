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

layout(location = 0) in vec2 v_position;
layout(location = 1) in float v_depth;
layout(location = 2) in float v_opacity;
layout(location = 3) in vec2 v_uv;
layout(location = 4) in vec2 v_normal;
layout(location = 5) in ivec4 v_bone_indices;
layout(location = 6) in vec4 v_bone_weights;
layout(location = 7) in int v_atlas_index;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec2 f_world_pos;
layout(location = 2) flat out int f_is_animated;
layout(location = 3) flat out int f_frame_count;
layout(location = 4) flat out float f_frame_width_uv;
layout(location = 5) flat out float f_frame_rate;
layout(location = 6) flat out int f_atlas_index;

void main() {
    // Calculate world position from local position and transform
    vec3 world_pos = vec3(v_position, 1.0) * object.transform;
    f_world_pos = world_pos.xy;

    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    float depth = (object.depth + (v_depth * object.depth_scale) - object.depth_min) / (object.depth_max - object.depth_min);
    gl_Position = vec4(screen_pos.xy, 1.0f - depth, 1.0);
    f_uv = v_uv;

    // Pass animation data to fragment shader
    f_is_animated = v_bone_indices.x;
    f_frame_count = v_bone_indices.y;
    f_frame_width_uv = v_bone_weights.x;
    f_frame_rate = v_bone_weights.y;
    f_atlas_index = v_atlas_index;
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec2 f_world_pos;
layout(location = 2) flat in int f_is_animated;
layout(location = 3) flat in int f_frame_count;
layout(location = 4) flat in float f_frame_width_uv;
layout(location = 5) flat in float f_frame_rate;
layout(location = 6) flat in int f_atlas_index;
layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform ColorBuffer {
    vec4 color;
    vec4 emission;
    vec2 uv_offset;
    vec2 padding;
} color_buffer;

layout(set = 5, binding = 0) uniform FragmentUserBuffer {
    vec2 world_size;    // World size in blocks
    float block_size;   // Block size in world units (1.0)
    float ambient;      // Minimum ambient light (0.05)
    float time;         // Global game time for animations
} lighting;

layout(set = 6, binding = 0) uniform sampler2DArray mainTexture;


void main() {
    // Animate UV for animated meshes
    vec2 uv = f_uv;
    if (f_is_animated > 0 && f_frame_count > 1) {
        int current_frame = int(mod(floor(lighting.time * f_frame_rate), float(f_frame_count)));
        uv.x += float(current_frame) * f_frame_width_uv;
    }

    outColor = texture(mainTexture, vec3(uv, float(f_atlas_index))) * color_buffer.color;
}

//@ END
