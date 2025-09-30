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
layout(location = 0) out vec2 f_uv0;

void main()
{
    // Position
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    gl_Position = vec4(screen_pos.xy, 0.0, 1.0);

    f_uv0 = v_uv0;
}

//@ END

//@ FRAGMENT

layout(set = 3, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;

layout(set = 4, binding = 0, row_major) uniform LightBuffer
{
    vec3 direction;
    float padding;
    vec4 diffuse_color;
    vec4 shadow_color;
} light;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 f_uv0;
layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(mainTexture, f_uv0) * colorData.color;
}

//@ END

