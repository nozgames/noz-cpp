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
    // Sample RGBA texture: RGB = normal map, A = SDF
    vec4 texSample = texture(mainTexture, f_uv0);
    vec3 normalMapRGB = texSample.rgb;
    float distance = texSample.a;
    
    // Convert normal map from [0,1] range back to [-1,1] range
    vec3 normal = normalize(normalMapRGB * 2.0 - 1.0);
    
    // Debug: visualize normals by outputting them as colors (uncomment to debug)
    // FragColor = vec4(normalMapRGB, 1.0); return;
    
    // Light setup - positioned at top-right of screen in 2D space
    vec2 screenPos = gl_FragCoord.xy / vec2(1920, 1080); // Assuming 1080p, adjust as needed
    vec2 lightPos = vec2(0.85, 0.15); // Top-right position (normalized screen coords)
    vec3 lightDir = normalize(vec3(lightPos - screenPos, 0.2)); // Slight Z offset for 3D effect
    
    // Basic lighting calculations
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // More dramatic lighting
    float ambient = 0.3;
    float diffuse = 0.8 * NdotL;
    float lighting = ambient + diffuse;
    
    // More prominent specular highlight
    vec3 viewDir = vec3(0.0, 0.0, 1.0); // Looking straight at 2D surface
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * 0.6;
    lighting += specular;
    
    // Additional rim lighting for more dramatic effect
    float rimDot = 1.0 - max(dot(normal, vec3(0.0, 0.0, 1.0)), 0.0);
    float rimLight = pow(rimDot, 2.0) * 0.4;
    lighting += rimLight;
    
    // Apply SDF for text rendering
    float width = fwidth(distance);
    float alpha = smoothstep(0.47 - width, 0.47 + width, distance);
    
    // Combine base color with lighting
    vec3 litColor = f_color.rgb;

    FragColor = vec4(litColor, alpha * f_color.a);
}

//@ END
