#version 450

//@ VERTEX

#include "../../shader_include/mesh.glsl"

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec3 f_normal;

void main()
{
    gl_Position = transform_to_screen(position);
    f_uv = uv0;

    // First rotate normal by object rotation (transposed)
    float obj_cos = cos(object.obj_rot);
    float obj_sin = sin(object.obj_rot);
    vec3 world_normal = vec3(
    dot(normal.xy, vec2(obj_cos, obj_sin)),    // Transposed: +sin instead of -sin
    dot(normal.xy, vec2(-obj_sin, obj_cos)),   // Transposed: -sin instead of +sin
    normal.z
    );

    // Then rotate by camera rotation (also transposed)
    f_normal = vec3(
    dot(world_normal.xy, vec2(camera.cam_rot.y, camera.cam_rot.x)),  // Transposed: +x instead of -x
    dot(world_normal.xy, vec2(-camera.cam_rot.x, camera.cam_rot.y)), // Transposed: flipped from original
    world_normal.z
    );
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec3 f_normal;
layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 0) uniform sampler2D mainTexture;
layout(set = 2, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;

void main()
{
    vec3 lightDir = normalize(vec3(-1, -1, -1));
    float diffuse = max(dot(f_normal, -lightDir), 0.0);
    float lighting = 0.3 + 0.7 * diffuse;
    vec4 texColor = texture(mainTexture, f_uv);
    outColor = vec4(texColor.rgb * lighting, texColor.a);
}

//@ END
