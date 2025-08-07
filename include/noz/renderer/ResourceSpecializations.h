/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "Font.h"
#include "Shader.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "Texture.h"
#include "Animation.h"
#include "IAnimation.h"
#include "AnimationBlendTree.h"
#include <noz/ui/StyleSheet.h>

// Template specializations must be in global namespace
template<>
inline std::shared_ptr<noz::renderer::Shader> noz::Resources::loadResource<noz::renderer::Shader>(const std::string& name)
{
    // For shaders, we need to handle both vertex and fragment shaders
    // The Shader::Load method expects just the shader name, not a full path
    // It will automatically look for both .vert and .frag files in the resources/shaders directory
    auto shader = noz::renderer::Shader::load(name);
    if (nullptr == shader)
        return nullptr;

    // Enable automatic unloading during shutdown
    shader->autoUnload();

    return shader;
}

template<>
inline std::shared_ptr<noz::renderer::Mesh> noz::Resources::loadResource<noz::renderer::Mesh>(const std::string& name) 
{
    auto mesh = noz::renderer::Mesh::load(name);
    if (!mesh)
        return nullptr;

    // Enable automatic unloading during shutdown
    mesh->autoUnload();

    return mesh;
}

template<>
inline std::shared_ptr<noz::renderer::Texture> noz::Resources::loadResource<noz::renderer::Texture>(const std::string& name) 
{
    auto texture = std::shared_ptr<noz::renderer::Texture>(noz::renderer::Texture::load(name));
    if (!texture)
        return nullptr;

    // Enable automatic unloading during shutdown
    texture->autoUnload();

    return texture;
}

template<>
inline std::shared_ptr<noz::renderer::Skeleton> noz::Resources::loadResource<noz::renderer::Skeleton>(const std::string& name) 
{
    auto skeleton = noz::renderer::Skeleton::load(name);
    if (!skeleton)
        return nullptr;

    // Enable automatic unloading during shutdown
    skeleton->autoUnload();

    return skeleton;
}

template<>
inline std::shared_ptr<noz::renderer::Animation> noz::Resources::loadResource<noz::renderer::Animation>(const std::string& name) 
{
    auto animation = noz::renderer::Animation::load(name);
    if (!animation)
        return nullptr;

	animation->autoUnload();
    return animation;
} 

template<>
inline std::shared_ptr<noz::renderer::Font> noz::Resources::loadResource<noz::renderer::Font>(const std::string& name)
{
    // Load font using meta file configuration
    auto font = noz::renderer::Font::load(name);
    if (!font) return nullptr;
    font->autoUnload();
    return font;
}

template<>
inline std::shared_ptr<noz::ui::StyleSheet> noz::Resources::loadResource<noz::ui::StyleSheet>(const std::string& name)
{
    auto styleSheet = noz::ui::StyleSheet::load(name);
    if (!styleSheet)
        return nullptr;

    // Enable automatic unloading during shutdown
    styleSheet->autoUnload();
    
    return styleSheet;
} 

template<>
inline std::shared_ptr<noz::renderer::IAnimation> noz::Resources::loadResource<noz::renderer::IAnimation>(const std::string& name)
{
    // First try to load as AnimationBlendTree2D (.blendtree2d)
    auto blendTree = noz::renderer::AnimationBlendTree2d::load(name);
    if (blendTree)
    {
        blendTree->autoUnload();
        return blendTree;
    }
    
    // Fall back to single Animation (.animation)
    auto animation = noz::renderer::Animation::load(name);
    if (animation)
    {
        animation->autoUnload();
        return animation;
    }
    
    return nullptr;
}

template<>
inline std::shared_ptr<noz::renderer::AnimationBlendTree2d> noz::Resources::loadResource<noz::renderer::AnimationBlendTree2d>(const std::string& name)
{
    auto blendTree = noz::renderer::AnimationBlendTree2d::load(name);
    if (!blendTree)
        return nullptr;

    blendTree->autoUnload();
    return blendTree;
}
