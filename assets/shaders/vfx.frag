/*

    NoZ Game Engine - VFX Fragment Shader

    Copyright(c) 2025 NoZ Games, LLC

*/

#version 450

// Input from vertex shader
layout(location = 0) in vec4 v_color;

// Output
layout(location = 0) out vec4 frag_color;

void main()
{
    // Simple color output with alpha blending support
    frag_color = v_color;
    
    // Discard fully transparent pixels to improve performance
    if (frag_color.a < 0.01)
        discard;
}