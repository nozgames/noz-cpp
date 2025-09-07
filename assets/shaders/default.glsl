#version 450

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
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec3 f_normal;

void main()
{
    // Position  
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    gl_Position = vec4(screen_pos.xy, 0.0, 1.0);

    // Uv
    f_uv = v_uv0;

    // Normal
    vec2 transform_right = normalize(object.transform[0].xy);
    vec2 transform_up = normalize(object.transform[1].xy);
    vec3 world_normal = vec3(
        dot(v_normal.xy, vec2(transform_right.x, transform_up.x)),
        dot(v_normal.xy, vec2(transform_right.y, transform_up.y)),
        v_normal.z
    );
    
    f_normal = normalize(world_normal);
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec3 f_normal;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;
layout(set = 3, binding = 0) uniform sampler2D mainTexture;

void main()
{
    vec3 lightDir = normalize(vec3(-0.5,-0.5, -1));
    float diffuse = max(dot(f_normal, -lightDir), 0.0);
    float lighting = 0.3 + 0.7 * diffuse;
    vec4 texColor = texture(mainTexture, f_uv);
    outColor = vec4(texColor.rgb * lighting * colorData.color.rgb, texColor.a * colorData.color.a);
}

//@ END
