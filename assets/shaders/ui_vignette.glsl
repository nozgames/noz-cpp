//@ VERTEX

layout(set = 0, binding = 0, row_major) uniform CameraBuffer {
    mat3 view_projection;
} camera;

layout(set = 1, binding = 0, row_major) uniform ObjectBuffer {
    mat3 transform;
    float depth;
} object;

layout(location = 0) in vec2 v_position;
layout(location = 1) in float v_depth;
layout(location = 2) in vec2 v_uv;
layout(location = 0) out vec2 f_uv;

void main()
{
    mat3 mvp = object.transform * camera.view_projection;
    vec3 screen_pos = vec3(v_position, 1.0) * mvp;
    gl_Position = vec4(screen_pos.xy, 0.0, 1.0);

    f_uv = v_uv;
}

//@ END

//@ FRAGMENT

layout(set = 4, binding = 0) uniform ColorBuffer {
    vec4 color;
    vec4 emission;
    vec2 uv_offset;
    vec2 padding;
} colorData;

layout(set = 5, binding = 0) uniform FragmentUserBuffer {
    vec4 color;
    float intensity;
    float smoothness;
    float padding0;
    float padding1;
} vignette;

layout(set = 6, binding = 0) uniform sampler2D mainTexture;

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

vec3 vignetteElliptical(
    vec2 uv,
    vec3 innerColor,
    vec3 outerColor,
    float intensity,
    float smoothness,
    vec2 ratio,
    float alpha) {
    vec2 centered = (uv - 0.5) * ratio;
    float dist = length(centered);
    dist = pow(dist, intensity);
    dist = smoothstep(0.0, smoothness, dist);
    return mix(innerColor, outerColor, dist * alpha);
}

vec3 vignetteRectangular(vec2 uv, vec3 innerColor, vec3 outerColor,
                         float intensity, float smoothness, float alpha) {
    vec2 centered = abs(uv - 0.5) * 2.0;
    float dist = max(centered.x, centered.y);
    dist = pow(dist, intensity);
    //dist = smoothstep(0.0, smoothness, dist);
    return mix(innerColor, outerColor, dist * alpha);
}

void main()
{
    vec2 uv = f_uv;
    uv *= 1.0 - uv.yx;

    float inner_radius = 0.2;
    float outer_radius = 0.7;
    float vignette_strength = 1.0;
    float dither_strength = 0.1;

    float dist = distance(f_uv, vec2(0.5));
    float vig = smoothstep(inner_radius, outer_radius, dist) * vignette_strength;
    float dither = fract(sin(dot(f_uv, vec2(12.9898, 78.233))) * 43758.5453123) * dither_strength;

    FragColor = mix(colorData.color, vignette.color + vec4(colorData.emission.rgb * colorData.emission.a, 0.0f), vig + dither);
}


//@ END
