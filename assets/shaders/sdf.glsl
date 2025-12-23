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
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec2 v_normal;  // Repurposed: xy = color UV (color_index, palette_index)
layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec2 f_color_uv;

void main() {
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    float depth = (object.depth + v_depth * object.depth_scale - object.depth_min) / (object.depth_max - object.depth_min);
    gl_Position = vec4(screen_pos.xy, 1.0f - depth, 1.0);
    f_uv = v_uv;
    f_color_uv = v_normal;  // Pass color UV to fragment shader
}

//@ END

//@ FRAGMENT

layout(set = 4, binding = 0) uniform ColorBuffer {
    vec4 color;
    vec4 emission;
    vec2 uv_offset;
    vec2 padding;
} color_buffer;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;    // SDF atlas
layout(set = 6, binding = 1) uniform sampler2D paletteTexture; // Color palette

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec2 f_color_uv;
layout(location = 0) out vec4 FragColor;

void main() {
    // Sample SDF from atlas
    float distance = texture(mainTexture, f_uv).r;
    float width = fwidth(distance);
    float alpha = smoothstep(0.5 - width, 0.5 + width, distance);

    // Sample color from palette (f_color_uv contains color_index, palette_index)
    vec4 palette_color = texture(paletteTexture, (f_color_uv + color_buffer.uv_offset) * 0.015625);

    // Combine SDF alpha with palette color
    float final_alpha = alpha * palette_color.a * color_buffer.color.a;
    vec3 final_color = palette_color.rgb * color_buffer.color.rgb;
    FragColor = vec4(final_color * final_alpha, final_alpha);
}

//@ END
