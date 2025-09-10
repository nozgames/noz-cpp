//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Shader;

struct LoadedCoreAssets
{
    struct
    {
        Shader* ui;
        Shader* text;
        Shader* lit;
        Shader* vfx;
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

extern LoadedCoreAssets g_core_assets;
