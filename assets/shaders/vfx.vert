/*

    NoZ Game Engine - VFX Vertex Shader

    Copyright(c) 2025 NoZ Games, LLC

*/

#version 450

// Input attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

// Output to fragment shader
layout(location = 0) out vec4 v_color;

// Uniforms
layout(set = 0, binding = 0) uniform CameraUniforms
{
    mat4 u_view;
    mat4 u_projection;
    mat4 u_view_projection;
    vec3 u_camera_position;
} camera;

layout(set = 1, binding = 0) uniform ObjectUniforms
{
    mat4 u_model;
    vec4 u_color;
} object;

void main()
{
    // Transform position to world space then to clip space
    vec4 world_position = object.u_model * vec4(a_position, 1.0);
    gl_Position = camera.u_view_projection * world_position;
    
    // Pass color to fragment shader
    v_color = object.u_color;
}