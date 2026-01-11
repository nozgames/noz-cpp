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
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec2 v_normal;
layout(location = 3) in vec4 v_color;
layout(location = 4) in float v_opacity;
layout(location = 5) in float v_depth;
layout(location = 0) out vec2 f_uv;

void main() {
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    float depth = (object.depth - object.depth_min) / (object.depth_max - object.depth_min);
    gl_Position = vec4(screen_pos.xy, 1.0f - depth, 1.0);
    f_uv = v_uv;
}

//@ END

//@ FRAGMENT

layout(set = 4, binding = 0) uniform ColorBuffer {
    vec4 color;
} color_buffer;

layout(set = 5, binding = 0) uniform FragmentUserBuffer {
    vec4 outline_color;
    float outline_width;
    float padding0;
    float padding1;
    float padding2;
} text_buffer;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

void main() {
    float distance = texture(mainTexture, f_uv).r;
    float width = fwidth(distance);
    float alpha = smoothstep(0.5 - width, 0.5 + width, distance);
    if (text_buffer.outline_width > 0.0) {
        vec4 outline_color = text_buffer.outline_color;
        float outline_threshold = 0.5 - text_buffer.outline_width;
        float outline_alpha = smoothstep(outline_threshold - width, outline_threshold + width, distance);
        vec3 final_color = mix(outline_color.rgb, color_buffer.color.rgb, alpha);
        float final_alpha = max(outline_alpha * outline_color.a, alpha * color_buffer.color.a);
        FragColor = vec4(final_color * final_alpha, final_alpha);
    } else {
        float final_alpha = alpha * color_buffer.color.a;
        FragColor = vec4(color_buffer.color.rgb * final_alpha, final_alpha);
    }
}

//@ END