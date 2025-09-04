//@ VERTEX

#include "../../shader_include/mesh.glsl"

layout(location = 0) out vec2 f_uv0;
layout(location = 1) out vec4 f_color;

void main()
{
    gl_Position = transform_to_screen(position);
    f_uv0 = uv0;
    f_color = v_color;
}

//@ END

//@ FRAGMENT


layout(set = 0, location = 0) in vec2 f_uv0;
layout(set = 0, location = 1) in vec4 f_color;
layout(set = 0, location = 0) out vec4 FragColor;

layout(set = 1, binding = 0) uniform sampler2D mainTexture;

layout(set = 2, binding = 0) uniform ColorBuffer
{
    vec4 color;
} color;

void main()
{
    float distance = texture(mainTexture, f_uv0).r;
    float width = fwidth(distance);
    float alpha = smoothstep(0.485 - width, 0.485 + width, distance);
    FragColor = vec4(f_color.rgb, alpha * f_color.a);
}

//@ END
