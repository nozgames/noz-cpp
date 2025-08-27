//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/asset.h>
#include <noz/noz.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

namespace fs = std::filesystem;

static float SRGBToLinear(float srgb)
{
    if (srgb <= 0.04045f)
        return srgb / 12.92f;

    return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
}

static void ConvertSRGBToLinear(uint8_t* pixels, int width, int height, int channels)
{
    int total_pixels = width * height;
    for (int i = 0; i < total_pixels; ++i)
    {
        // Convert RGB channels (skip alpha)
        for (int c = 0; c < std::min(3, channels); ++c)
        {
            uint8_t srgb_value = pixels[i * channels + c];
            float srgb_float = (float)srgb_value / 255.0f;
            float linear_float = SRGBToLinear(srgb_float);
            pixels[i * channels + c] = static_cast<uint8_t>(std::round(linear_float * 255.0f));
        }
    }
}

static void GenerateMipmap(
    const uint8_t* src, int src_width, int src_height, 
    uint8_t* dst, int dst_width, int dst_height, 
    int channels)
{
    float x_ratio = (float)src_width / dst_width;
    float y_ratio = (float)src_height / dst_height;
    
    for (int y = 0; y < dst_height; y++)
    {
        for (int x = 0; x < dst_width; x++)
        {
            // Simple box filter for downsampling
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;
            
            int src_x0 = (int)src_x;
            int src_y0 = (int)src_y;
            int src_x1 = std::min(src_x0 + 1, src_width - 1);
            int src_y1 = std::min(src_y0 + 1, src_height - 1);
            
            float fx = src_x - src_x0;
            float fy = src_y - src_y0;
            
            for (int c = 0; c < channels; c++)
            {
                float v00 = src[(src_y0 * src_width + src_x0) * channels + c];
                float v10 = src[(src_y0 * src_width + src_x1) * channels + c];
                float v01 = src[(src_y1 * src_width + src_x0) * channels + c];
                float v11 = src[(src_y1 * src_width + src_x1) * channels + c];
                
                float v0 = v00 * (1 - fx) + v10 * fx;
                float v1 = v01 * (1 - fx) + v11 * fx;
                float v = v0 * (1 - fy) + v1 * fy;
                
                dst[(y * dst_width + x) * channels + c] = (uint8_t)v;
            }
        }
    }
}

static void WriteTextureData(
    Stream* stream,
    const uint8_t* data,
    int width,
    int height,
    int channels,
    const std::string& min_filter,
    const std::string& mag_filter,
    const std::string& clamp_u,
    const std::string& clamp_v,
    const std::string& clamp_w,
    bool has_mipmaps)
{
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_TEXTURE;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);
    
    // Convert string options to enum values
    uint8_t min_filter_value = (min_filter == "nearest" || min_filter == "point") ? 0 : 1;
    uint8_t mag_filter_value = (mag_filter == "nearest" || mag_filter == "point") ? 0 : 1;
    
    uint8_t clamp_u_value = 1; // Default to ClampToEdge
    if (clamp_u == "repeat") clamp_u_value = 0;
    else if (clamp_u == "clamp_to_edge") clamp_u_value = 1;
    else if (clamp_u == "mirrored_repeat") clamp_u_value = 2;
    else if (clamp_u == "clamp_to_border") clamp_u_value = 3;
    
    uint8_t clamp_v_value = 1;
    if (clamp_v == "repeat") clamp_v_value = 0;
    else if (clamp_v == "clamp_to_edge") clamp_v_value = 1;
    else if (clamp_v == "mirrored_repeat") clamp_v_value = 2;
    else if (clamp_v == "clamp_to_border") clamp_v_value = 3;
    
    uint8_t clamp_w_value = 1;
    if (clamp_w == "repeat") clamp_w_value = 0;
    else if (clamp_w == "clamp_to_edge") clamp_w_value = 1;
    else if (clamp_w == "mirrored_repeat") clamp_w_value = 2;
    else if (clamp_w == "clamp_to_border") clamp_w_value = 3;
    
    // Write texture metadata
    uint32_t format = (channels == 4) ? 1 : 0; // 0=RGB, 1=RGBA
    WriteU32(stream, format);
    WriteU32(stream, width);
    WriteU32(stream, height);
    
    // Write sampler options
    WriteU8(stream, min_filter_value);
    WriteU8(stream, mag_filter_value);
    WriteU8(stream, clamp_u_value);
    WriteU8(stream, clamp_v_value);
    WriteU8(stream, clamp_w_value);
    WriteBool(stream, has_mipmaps);
    
    // Write pixel data
    size_t data_size = width * height * channels;
    WriteU32(stream, data_size);
    WriteBytes(stream, (void*)data, data_size);
}

static void WriteTextureWithMipmaps(
    Stream* stream,
    const std::vector<std::vector<uint8_t>>& mip_levels,
    const std::vector<std::pair<int, int>>& mip_dimensions,
    int channels,
    const std::string& min_filter,
    const std::string& mag_filter,
    const std::string& clamp_u,
    const std::string& clamp_v,
    const std::string& clamp_w)
{
    if (mip_levels.empty() || mip_dimensions.empty())
    {
        throw std::runtime_error("Invalid mipmap data");
    }
    
    // Calculate total size for runtime
    size_t total_size = 0;
    for (const auto& level : mip_levels)
    {
        total_size += level.size();
    }
    
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_TEXTURE;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);
    
    // Convert string options to enum values
    uint8_t min_filter_value = (min_filter == "nearest" || min_filter == "point") ? 0 : 1;
    uint8_t mag_filter_value = (mag_filter == "nearest" || mag_filter == "point") ? 0 : 1;
    
    uint8_t clamp_u_value = 1;
    if (clamp_u == "repeat") clamp_u_value = 0;
    else if (clamp_u == "clamp_to_edge") clamp_u_value = 1;
    else if (clamp_u == "mirrored_repeat") clamp_u_value = 2;
    else if (clamp_u == "clamp_to_border") clamp_u_value = 3;
    
    uint8_t clamp_v_value = 1;
    if (clamp_v == "repeat") clamp_v_value = 0;
    else if (clamp_v == "clamp_to_edge") clamp_v_value = 1;
    else if (clamp_v == "mirrored_repeat") clamp_v_value = 2;
    else if (clamp_v == "clamp_to_border") clamp_v_value = 3;
    
    uint8_t clamp_w_value = 1;
    if (clamp_w == "repeat") clamp_w_value = 0;
    else if (clamp_w == "clamp_to_edge") clamp_w_value = 1;
    else if (clamp_w == "mirrored_repeat") clamp_w_value = 2;
    else if (clamp_w == "clamp_to_border") clamp_w_value = 3;
    
    // Write texture metadata
    uint32_t format = (channels == 4) ? 1 : 0; // 0=RGB, 1=RGBA
    uint32_t width = mip_dimensions[0].first;
    uint32_t height = mip_dimensions[0].second;
    uint32_t num_mip_levels = static_cast<uint32_t>(mip_levels.size());
    
    WriteU32(stream, format);
    WriteU32(stream, width);
    WriteU32(stream, height);
    
    // Write sampler options
    WriteU8(stream, min_filter_value);
    WriteU8(stream, mag_filter_value);
    WriteU8(stream, clamp_u_value);
    WriteU8(stream, clamp_v_value);
    WriteU8(stream, clamp_w_value);
    WriteBool(stream, true); // has mipmaps
    
    // Write number of mip levels
    WriteU32(stream, num_mip_levels);
    
    // Write each mip level
    for (size_t i = 0; i < mip_levels.size(); ++i)
    {
        // Write mip level dimensions
        WriteU32(stream, mip_dimensions[i].first);
        WriteU32(stream, mip_dimensions[i].second);
        
        // Write mip level data
        WriteU32(stream, static_cast<uint32_t>(mip_levels[i].size()));
        WriteBytes(stream, (void*)mip_levels[i].data(), mip_levels[i].size());
    }
}

void ImportTexture(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta)
{
    fs::path src_path = source_path;
    
    // Load image using stb_image
    int width, height, channels;
    unsigned char* image_data = stbi_load(src_path.string().c_str(), &width, &height, &channels, 0);
    
    if (!image_data)
    {
        throw std::runtime_error("Failed to load texture file");
    }
    
    // Parse texture properties from meta props (with defaults)
    std::string min_filter = meta->GetString("texture", "min_filter", "linear");
    std::string mag_filter = meta->GetString("texture", "mag_filter", "linear");
    std::string clamp_u = meta->GetString("texture", "clamp_u", "clamp_to_edge");
    std::string clamp_v = meta->GetString("texture", "clamp_v", "clamp_to_edge");
    std::string clamp_w = meta->GetString("texture", "clamp_w", "clamp_to_edge");
    bool generate_mipmaps = meta->GetBool("texture", "mipmaps", false);
    bool convert_from_srgb = meta->GetBool("texture", "srgb", false);
    
    // Convert to RGBA if needed
    std::vector<uint8_t> rgba_data;
    if (channels != 4)
    {
        rgba_data.resize(width * height * 4);
        for (int i = 0; i < width * height; ++i)
        {
            for (int c = 0; c < 3; ++c)
            {
                rgba_data[i * 4 + c] = (c < channels) ? image_data[i * channels + c] : 0;
            }
            rgba_data[i * 4 + 3] = (channels == 4) ? image_data[i * channels + 3] : 255; // Alpha
        }
        channels = 4;
    }
    else
    {
        rgba_data.assign(image_data, image_data + (width * height * channels));
    }
    
    stbi_image_free(image_data);
    
    // Convert from sRGB to linear if requested
    if (convert_from_srgb)
        ConvertSRGBToLinear(rgba_data.data(), width, height, channels);

    // Generate mipmaps if requested
    if (generate_mipmaps)
    {
        std::vector<std::vector<uint8_t>> mip_levels;
        std::vector<std::pair<int, int>> mip_dimensions;
        
        // Add base level
        mip_levels.push_back(rgba_data);
        mip_dimensions.push_back({width, height});
        
        // Generate additional mip levels
        int current_width = width;
        int current_height = height;
        
        while (current_width > 1 || current_height > 1)
        {
            int next_width = std::max(1, current_width / 2);
            int next_height = std::max(1, current_height / 2);
            
            std::vector<uint8_t> mip_data(next_width * next_height * channels);
            
            // Generate mipmap from previous level
            GenerateMipmap(
                mip_levels.back().data(), current_width, current_height,
                mip_data.data(), next_width, next_height,
                channels
            );
            
            mip_levels.push_back(std::move(mip_data));
            mip_dimensions.push_back({next_width, next_height});
            
            current_width = next_width;
            current_height = next_height;
        }
        
        WriteTextureWithMipmaps(
            output_stream,
            mip_levels,
            mip_dimensions,
            channels,
            min_filter,
            mag_filter,
            clamp_u,
            clamp_v,
            clamp_w
        );
    }
    else
    {
        WriteTextureData(
            output_stream,
            rgba_data.data(),
            width,
            height,
            channels,
            min_filter,
            mag_filter,
            clamp_u,
            clamp_v,
            clamp_w,
            false
        );
    }

}

bool DoesTextureDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // Check if dependency is the meta file for this texture
    fs::path meta_path = fs::path(source_path.string() + ".meta");
    
    return meta_path == dependency_path;
}

static const char* g_texture_extensions[] = {
    ".png",
    ".jpg",
    ".jpeg",
    ".bmp",
    ".tga",
    ".gif",
    nullptr
};

static AssetImporterTraits g_texture_importer_traits = {
    .type_name = "Texture",
    .type = TYPE_TEXTURE,
    .signature = ASSET_SIGNATURE_TEXTURE,
    .file_extensions = g_texture_extensions,
    .import_func = ImportTexture,
    .does_depend_on = DoesTextureDependOn
};

AssetImporterTraits* GetTextureImporterTraits()
{
    return &g_texture_importer_traits;
}