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

layout(set = 3, binding = 0, row_major) uniform UIBuffer {
    float border_radius;
} ui;

layout(location = 0) in vec2 v_position;
layout(location = 1) in float v_depth;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec2 v_normal;

layout(location = 0) out vec2 f_uv;

void main() {
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    float depth = (object.depth + (v_depth * object.depth_scale) - object.depth_min) / (object.depth_max - object.depth_min);

    // Scale border_radius from world units to NDC
    float scale_x = camera.view_projection[0][0];
    float scale_y = camera.view_projection[1][1];
    screen_pos.x += v_normal.x * ui.border_radius * scale_x;
    screen_pos.y += v_normal.y * ui.border_radius * scale_y;

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

layout(set = 5, binding = 0) uniform UIBuffer {
    vec4 border_color;
    float border_radius;
    float border_width;
    float padding0;
    float padding1;
} ui_buffer;

void main() {
    // Premultiply input colors
    vec4 color = color_buffer.color;
    vec4 border_color = ui_buffer.border_color;

    // premultiply
    color.rgb *= color.a;
    border_color.rgb *= border_color.a;

    // Squircle distance using superellipse formula: |x|^n + |y|^n = 1
    // n=2 is circle, n=4+ gives squircle, higher n = more square
    float n = 4.0;
    float dist = pow(pow(abs(f_uv.x), n) + pow(abs(f_uv.y), n), 1.0 / n);
    float edge = fwidth(dist);

    // border
    float border = (1 + edge) - (ui_buffer.border_width / ui_buffer.border_radius);
    color = mix(color, border_color, smoothstep(border - edge, border, dist));

    // radius (premultiplied alpha - multiply RGBA by the edge falloff)
    float radius_alpha = 1.0 - smoothstep(1.0 - edge, 1.0, dist);
    color *= radius_alpha;

    // final color
    outColor = color;
}


//@ END

