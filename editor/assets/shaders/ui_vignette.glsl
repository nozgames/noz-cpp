//@ VERTEX

layout(set = 0, binding = 0, row_major) uniform CameraBuffer {
    mat3 view_projection;
} camera;

layout(set = 1, binding = 0, row_major) uniform ObjectBuffer {
    mat3 transform;
    float depth;
} object;

layout(location = 0) in vec2 v_position;
layout(location = 1) in float v_depth;
layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec2 f_uv;

void main() {
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    gl_Position = vec4(screen_pos.xy, v_depth, 1.0);
    f_uv = v_uv;
}

//@ END

//@ FRAGMENT

layout(set = 4, binding = 0) uniform ColorBuffer {
    vec4 color;
} color_buffer;

layout(set = 5, binding = 0) uniform VignetteBuffer {
    float intensity;
    float smoothness;
    float padding0;
    float padding1;
} vignette;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

void main() {
    vec2 uv = f_uv;
    uv *=  1.0 - uv.yx;
    float vig = uv.x * uv.y * vignette.intensity;
    vig = pow(vig, vignette.smoothness);
    FragColor = color_buffer.color * (1.0 - vig);
}

//@ END
