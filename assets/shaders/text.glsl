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
    // Sample SDF texture
    float distance = texture(mainTexture, f_uv0).a;
    
    // Compute normal from SDF gradient
    vec2 texelSize = 1.0 / textureSize(mainTexture, 0);
    float sdf_right = texture(mainTexture, f_uv0 + vec2(texelSize.x, 0.0)).a;
    float sdf_left = texture(mainTexture, f_uv0 + vec2(-texelSize.x, 0.0)).a;
    float sdf_up = texture(mainTexture, f_uv0 + vec2(0.0, -texelSize.y)).a;
    float sdf_down = texture(mainTexture, f_uv0 + vec2(0.0, texelSize.y)).a;
    
    // Calculate gradient (normal points toward higher SDF values)
    vec2 gradient = vec2(sdf_right - sdf_left, sdf_down - sdf_up) * 0.5;
    vec3 normal = normalize(vec3(gradient, 1.0)); // Z=1 for subtle 3D effect
    
    // Light setup - positioned at top-right of screen in 2D space
    vec2 screenPos = gl_FragCoord.xy / vec2(1920, 1080); // Assuming 1080p, adjust as needed
    vec2 lightPos = vec2(0.85, 0.15); // Top-right position (normalized screen coords)
    vec3 lightDir = normalize(vec3(lightPos - screenPos, 0.2)); // Slight Z offset for 3D effect
    
    // Basic lighting calculations
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // Ambient + diffuse lighting
    float ambient = 0.1;
    float diffuse = 0.9 * NdotL;
    float lighting = ambient + diffuse;
    
    // Optional: Add specular highlight
    vec3 viewDir = vec3(0.0, 0.0, 1.0); // Looking straight at 2D surface
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * 0.5;
    lighting += specular;
    
    // Apply SDF for text rendering
    float width = fwidth(distance);
    float alpha = smoothstep(0.485 - width, 0.485 + width, distance);
    
    // Combine base color with lighting
    vec3 litColor = f_color.rgb * lighting;
    
    FragColor = vec4(litColor, alpha * f_color.a);
}

//@ END
