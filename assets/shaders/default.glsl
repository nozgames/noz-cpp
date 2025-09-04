#version 450

//@ VERTEX

#include "../../shader_include/mesh.glsl"

layout(location = 0) out vec2 fragTexCoord;

void main()
{
    gl_Position = transform_to_screen(position);
    fragTexCoord = uv0;
}

//@ END

//@ FRAGMENT

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 0) uniform sampler2D mainTexture;
layout(set = 2, binding = 0) uniform ColorBuffer
{
    vec4 color;
} colorData;

void main()
{
    outColor = texture(mainTexture, fragTexCoord);
}

//@ END
