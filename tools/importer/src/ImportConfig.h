/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace noz
{
    namespace import
    {
        struct ImportConfig
        {
            // General settings
            std::string sourceDirectory;
            std::string outputDirectory;
            bool watchForChanges = false;
            bool parallelProcessing = true;
            
            // Font import settings
            struct FontConfig
            {
                int fontSize = 16;
                std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,!?;:()[]{}\"'`~@#$%^&*+-=_|\\/<>";
                int textureSize = 1024;
                int padding = 4;
                bool enableSDF = true;
                int sdfPadding = 4;
            } font;
            
            // Texture import settings
            struct TextureConfig
            {
                bool generateMipmaps = true;
                bool compressTextures = false;
                std::string compressionFormat = "RGBA8";
                bool flipVertically = true;
            } texture;
            
            // Model import settings
            struct ModelConfig
            {
                bool optimizeMeshes = true;
                bool generateNormals = true;
                bool generateTangents = true;
                float scale = 1.0f;
            } model;
            
            // Shader import settings
            struct ShaderConfig
            {
                bool preprocessShaders = true;
                bool validateShaders = true;
                std::vector<std::string> includePaths;
            } shader;
        };
    }
} 