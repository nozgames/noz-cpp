//@ VERTEX

#include "../../shader_include/mesh.glsl"

layout(location = 0) out vec2 v_uv0;

void main()
{
    gl_Position = transform_to_screen(position);
    v_uv0 = uv0;
}

//@ END

//@ FRAGMENT

layout(set = 2, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;

layout(set = 1, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 v_uv0;
layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(mainTexture, v_uv0) * colorData.color;
}

//@ END
