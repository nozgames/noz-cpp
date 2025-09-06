#version 450

//@ VERTEX

layout(set = 0, binding = 0, row_major) uniform CameraBuffer
{
    mat3 view_projection;
} camera;

layout(set = 0, binding = 1, row_major) uniform ObjectBuffer
{
    mat3 transform;
} object;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 normal;
layout(location = 4) in float bone_index;

vec4 transform_to_screen(vec2 vertex_pos)
{
    // Combined transform: view-projection * object
    mat3 mvp = camera.view_projection * object.transform;

    // Transform vertex directly to screen space
    vec3 screenPos = mvp * vec3(vertex_pos, 1.0);

    return vec4(screenPos.xy, 0.0, 1.0);
}


layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec3 f_normal;

void main()
{
    // Position
    mat3 mvp = camera.view_projection * object.transform;
    vec3 screenPos = mvp * vec3(vertex_pos, 1.0);
    gl_Position = vec4(screenPos.xy, 0.0, 1.0);

    // Uv
    f_uv = uv0;

    // Normal

    // First rotate normal by object rotation (transposed)
//    float obj_cos = cos(object.obj_rot);
//    float obj_sin = sin(object.obj_rot);
//    vec3 world_normal = vec3(
//    dot(normal.xy, vec2(obj_cos, obj_sin)),    // Transposed: +sin instead of -sin
//    dot(normal.xy, vec2(-obj_sin, obj_cos)),   // Transposed: -sin instead of +sin
//    normal.z
//    );
//
//    // Then rotate by camera rotation (also transposed)
//    f_normal = vec3(
//    dot(world_normal.xy, vec2(camera.cam_rot.y, camera.cam_rot.x)),  // Transposed: +x instead of -x
//    dot(world_normal.xy, vec2(-camera.cam_rot.x, camera.cam_rot.y)), // Transposed: flipped from original
//    world_normal.z
//    );
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
    vec3 lightDir = normalize(vec3(-0.5,-0.5, -1));
    float diffuse = max(dot(f_normal, -lightDir), 0.0);
    float lighting = 0.3 + 0.7 * diffuse;
    vec4 texColor = texture(mainTexture, f_uv);
    outColor = vec4(texColor.rgb * lighting, texColor.a);
}

//@ END
