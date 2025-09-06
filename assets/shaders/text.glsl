//@ VERTEX

layout(set = 0, binding = 0, row_major) uniform CameraBuffer
{
    mat3 view_projection;
} camera;

layout(set = 1, binding = 0, row_major) uniform ObjectBuffer
{
    mat3 transform;
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

layout(set = 2, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;

layout(set = 3, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 f_uv0;
layout(location = 0) out vec4 FragColor;

void main()
{
    float distance = texture(mainTexture, f_uv0).r;
    float width = fwidth(distance);
    float alpha = smoothstep(0.5 - width, 0.5 + width, distance);
    FragColor = vec4(colorData.color.rgb, alpha * colorData.color.a);
}

//@ END