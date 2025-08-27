//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Shader;

struct LoadedCoreAssets
{
    struct
    {
        Shader* shadow;
        Shader* ui;
        Shader* text;
        Shader* lit;
        Shader* gizmo;
        Shader* gamma;
    } shaders;

    struct
    {
        Font* fallback;
    } fonts;

    struct
    {
        Texture* white;
    } textures;
};

extern LoadedCoreAssets CoreAssets;
