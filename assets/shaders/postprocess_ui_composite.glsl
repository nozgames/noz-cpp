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
layout(location = 0) out vec2 f_uv;

void main() {
    // Fullscreen quad: position is already in NDC (-1 to 1)
    gl_Position = vec4(v_position, 0.0, 1.0);
    f_uv = v_uv;
}

//@ END

//@ FRAGMENT

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

void main() {
    // Simple passthrough - alpha blending is handled by pipeline state
    FragColor = texture(mainTexture, f_uv);
}

//@ END
