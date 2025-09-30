#version 450

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
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec3 f_normal;

void main()
{
    // Position  
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    gl_Position = vec4(screen_pos.xy, object.depth, 1.0);

    // Uv
    f_uv = v_uv0;

    // Normal
    vec2 transform_right = normalize(object.transform[0].xy);
    vec2 transform_up = normalize(object.transform[1].xy);
    vec3 world_normal = vec3(
        dot(v_normal.xy, vec2(transform_right.x, transform_right.y)),
        dot(v_normal.xy, vec2(transform_up.x, transform_up.y)),
        v_normal.z
    );
    
    f_normal = world_normal;
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec3 f_normal;
layout(location = 0) out vec4 outColor;
layout(set = 3, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;

layout(set = 4, binding = 0) uniform LightBuffer
{
    vec3 direction;
    float padding;
    vec4 diffuse_color;
    vec4 shadow_color;
} light;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

const vec3 shadow_color = vec3(0.3, 0.3, 0.6);
const float shadow_intensity = 0.5;
const vec3 light_color = vec3(1.1, 1.1, 1);
const float light_intensity = 2.0;

void main()
{
    float diffuse = (dot(f_normal.xy, light.direction.xy) + 1) * 0.5;
    vec4 texColor = texture(mainTexture, f_uv) * colorData.color;
    vec3 color = texColor.rgb * colorData.color.rgb;
    //vec3 shadow = mix(texColor.rgb, shadow_color, (1.0f - diffuse) * shadow_intensity * f_normal.z);
    vec3 shadow = mix(color, color * shadow_color, (1.0f - diffuse) * f_normal.z);
    //vec3 light = mix(texColor.rgb, light_color, diffuse * light_intensity * f_normal.z);
    vec3 light = mix(color, color * light_color, diffuse * f_normal.z);
    outColor = vec4(mix(shadow, light, diffuse), texColor.a * colorData.color.a);
}

//@ END
