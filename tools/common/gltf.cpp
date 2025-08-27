//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include <gltf.h>
#include "../../src/internal.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <noz/allocator.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

// Conversion helpers
static vec3 convert_vector3(float* vector3);
static vec2 convert_vector2(float* vector2);  
static vec2 convert_uv(float* vector2);
static quat convert_quaternion(float* quaternion);

// @file
bool GLTFLoader::open(const std::filesystem::path& file_path)
{
    close();
    
    cgltf_options options = {};
    struct cgltf_data* gltf_data = nullptr;
    
    std::string path_str = file_path.string();
    cgltf_result result = cgltf_parse_file(&options, path_str.c_str(), &gltf_data);
    if (result != cgltf_result_success)
        return false;
    
    result = cgltf_load_buffers(&options, gltf_data, path_str.c_str());
    if (result != cgltf_result_success)
    {
        cgltf_free(gltf_data);
        return false;
    }
    
    data = gltf_data;
    path = file_path;
    
    return true;
}

void GLTFLoader::close()
{
    if (data)
    {
        cgltf_free(data);
        data = nullptr;
    }
    path.clear();
}

// @class_methods
std::vector<GLTFBone> GLTFLoader::read_bones(const GLTFBoneFilter& filter)
{
    std::vector<GLTFBone> bones;
    if (!data || !data->nodes_count)
        return bones;
    
    // Find the root node (usually named "root")
    struct cgltf_node* root_node = nullptr;
    for (size_t i = 0; i < data->nodes_count; ++i)
    {
        struct cgltf_node* node = &data->nodes[i];
        if (node->name && std::string(node->name) == "root")
        {
            root_node = node;
            break;
        }
    }
    
    // If no root found, use first node with children
    if (!root_node)
    {
        for (size_t i = 0; i < data->nodes_count; ++i)
        {
            if (data->nodes[i].children_count > 0)
            {
                root_node = &data->nodes[i];
                break;
            }
        }
    }
    
    if (!root_node)
        return bones;
    
    // Recursively process bones starting from root
    process_bone_recursive(root_node, bones, -1, filter);
    
    // Update bone lengths and directions
    for (size_t i = 0; i < bones.size(); ++i)
    {
        GLTFBone& bone = bones[i];
        if (bone.parent_index >= 0)
        {
            GLTFBone& parent = bones[bone.parent_index];
            vec3 direction = bone.position - parent.position;
            parent.length = length(direction);
            if (parent.length > 0.0f)
                parent.direction = normalize(direction);
        }
    }
    
    return bones;
}

void GLTFLoader::process_bone_recursive(struct cgltf_node* node, std::vector<GLTFBone>& bones, int parent_index, const GLTFBoneFilter& filter)
{
    if (!node || !node->name)
        return;
        
    std::string node_name = node->name;
    
    // Check if this bone should be excluded
    for (const auto& exclude_name : filter.exclude_bones)
    {
        if (node_name == exclude_name)
            return;
    }
    
    // Create the bone
    GLTFBone bone = {};
    bone.name = node_name;
    bone.index = static_cast<int>(bones.size());
    bone.parent_index = parent_index;
    
    // Extract transformation data
    if (node->has_matrix)
    {
        // TODO: Extract position, rotation, scale from matrix
        bone.position = vec3(node->matrix[12], node->matrix[13], node->matrix[14]);
        bone.rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
        bone.scale = vec3(1.0f, 1.0f, 1.0f);
    }
    else
    {
        // Extract from individual components
        bone.position = node->has_translation ? convert_vector3(node->translation) : vec3(0.0f);
        bone.rotation = node->has_rotation ? convert_quaternion(node->rotation) : quat(1.0f, 0.0f, 0.0f, 0.0f);
        bone.scale = node->has_scale ? convert_vector3(node->scale) : vec3(1.0f);
    }
    
    bones.push_back(bone);
    int current_bone_index = static_cast<int>(bones.size() - 1);
    
    // Process children
    for (size_t i = 0; i < node->children_count; ++i)
    {
        process_bone_recursive(node->children[i], bones, current_bone_index, filter);
    }
}

GLTFMesh GLTFLoader::read_mesh(const std::vector<GLTFBone>& bones)
{
    GLTFMesh mesh;
    if (!data || !data->meshes_count)
        return mesh;
        
    struct cgltf_mesh* cgltf_mesh = &data->meshes[0];
    if (!cgltf_mesh->primitives_count)
        return mesh;
        
    struct cgltf_primitive* primitive = &cgltf_mesh->primitives[0];
    
    // Read positions
    if (primitive->attributes)
    {
        for (size_t i = 0; i < primitive->attributes_count; ++i)
        {
            if (primitive->attributes[i].type == cgltf_attribute_type_position)
            {
                struct cgltf_accessor* accessor = primitive->attributes[i].data;
                if (accessor && accessor->count > 0)
                {
                    mesh.positions.resize(accessor->count);
                    uint8_t* buffer_data = (uint8_t*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;
                    
                    // Convert each position vector
                    float* positions_float = (float*)buffer_data;
                    for (size_t pos_idx = 0; pos_idx < accessor->count; ++pos_idx)
                    {
                        mesh.positions[pos_idx] = convert_vector3(&positions_float[pos_idx * 3]);
                    }
                }
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_normal)
            {
                struct cgltf_accessor* accessor = primitive->attributes[i].data;
                if (accessor && accessor->count > 0)
                {
                    mesh.normals.resize(accessor->count);
                    uint8_t* buffer_data = (uint8_t*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;
                    
                    // Convert each normal vector
                    float* normals_float = (float*)buffer_data;
                    for (size_t norm_idx = 0; norm_idx < accessor->count; ++norm_idx)
                    {
                        mesh.normals[norm_idx] = convert_vector3(&normals_float[norm_idx * 3]);
                    }
                }
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_texcoord)
            {
                struct cgltf_accessor* accessor = primitive->attributes[i].data;
                if (accessor && accessor->count > 0)
                {
                    mesh.uvs.resize(accessor->count);
                    uint8_t* buffer_data = (uint8_t*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;
                    
                    // Convert each UV coordinate
                    float* uvs_float = (float*)buffer_data;
                    for (size_t uv_idx = 0; uv_idx < accessor->count; ++uv_idx)
                    {
                        mesh.uvs[uv_idx] = convert_uv(&uvs_float[uv_idx * 2]);
                    }
                }
            }
        }
    }
    
    // Read indices if present
    if (primitive->indices)
    {
        struct cgltf_accessor* accessor = primitive->indices;
        if (accessor && accessor->count > 0)
        {
            mesh.indices.resize(accessor->count);
            uint8_t* buffer_data = (uint8_t*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;
            
            // Handle different index types
            if (accessor->component_type == cgltf_component_type_r_16u)
            {
                memcpy(mesh.indices.data(), buffer_data, accessor->count * sizeof(uint16_t));
            }
            else if (accessor->component_type == cgltf_component_type_r_32u)
            {
                // Convert from uint32 to uint16
                uint32_t* indices32 = (uint32_t*)buffer_data;
                for (size_t i = 0; i < accessor->count; ++i)
                {
                    mesh.indices[i] = static_cast<uint16_t>(indices32[i]);
                }
            }
        }
    }
    
    return mesh;
}

GLTFAnimation GLTFLoader::read_animation(const std::vector<GLTFBone>& bones, const std::string& animation_name)
{
    GLTFAnimation animation;
    if (!data || !data->animations_count)
        return animation;
    
    // Find the animation by name
    struct cgltf_animation* cgltf_anim = nullptr;
    for (size_t i = 0; i < data->animations_count; ++i)
    {
        if (data->animations[i].name && animation_name == data->animations[i].name)
        {
            cgltf_anim = &data->animations[i];
            break;
        }
    }
    
    // If no name specified or not found, use first animation
    if (!cgltf_anim && data->animations_count > 0)
        cgltf_anim = &data->animations[0];
        
    if (!cgltf_anim || !cgltf_anim->channels_count)
        return animation;
    
    // Calculate frame count and stride
    animation.frame_count = 0;
    animation.frame_stride = static_cast<int>(bones.size() * 10); // position(3) + rotation(4) + scale(3)
    
    // Find maximum frame count across all channels
    for (size_t i = 0; i < cgltf_anim->channels_count; ++i)
    {
        struct cgltf_animation_channel* channel = &cgltf_anim->channels[i];
        if (channel->sampler && channel->sampler->input)
        {
            int frames = static_cast<int>(channel->sampler->input->count);
            animation.frame_count = std::max(animation.frame_count, frames);
        }
    }
    
    if (animation.frame_count == 0)
        return animation;
    
    // Allocate animation data
    size_t data_size = animation.frame_count * animation.frame_stride;
    animation.data.resize(data_size, 0.0f);
    
    // Process each animation channel
    for (size_t i = 0; i < cgltf_anim->channels_count; ++i)
    {
        struct cgltf_animation_channel* channel = &cgltf_anim->channels[i];
        if (!channel->sampler || !channel->target_node || !channel->target_node->name)
            continue;
            
        // Find the bone index for this channel's target node
        int bone_index = -1;
        for (size_t b = 0; b < bones.size(); ++b)
        {
            if (bones[b].name == channel->target_node->name)
            {
                bone_index = static_cast<int>(b);
                break;
            }
        }
        
        if (bone_index == -1)
            continue;
            
        struct cgltf_animation_sampler* sampler = channel->sampler;
        if (!sampler->input || !sampler->output)
            continue;
            
        // Get the output data
        uint8_t* output_buffer = (uint8_t*)sampler->output->buffer_view->buffer->data + 
                                sampler->output->buffer_view->offset + sampler->output->offset;
        
        // Process based on target path
        for (size_t frame = 0; frame < sampler->input->count && frame < static_cast<size_t>(animation.frame_count); ++frame)
        {
            size_t frame_offset = frame * animation.frame_stride + bone_index * 10;
            
            if (channel->target_path == cgltf_animation_path_type_translation)
            {
                float* translation = (float*)(output_buffer + frame * 3 * sizeof(float));
                vec3 pos = convert_vector3(translation);
                animation.data[frame_offset + 0] = pos.x;
                animation.data[frame_offset + 1] = pos.y;
                animation.data[frame_offset + 2] = pos.z;
            }
            else if (channel->target_path == cgltf_animation_path_type_rotation)
            {
                float* rotation = (float*)(output_buffer + frame * 4 * sizeof(float));
                quat rot = convert_quaternion(rotation);
                animation.data[frame_offset + 3] = rot.x;
                animation.data[frame_offset + 4] = rot.y;
                animation.data[frame_offset + 5] = rot.z;
                animation.data[frame_offset + 6] = rot.w;
            }
            else if (channel->target_path == cgltf_animation_path_type_scale)
            {
                float* scale = (float*)(output_buffer + frame * 3 * sizeof(float));
                vec3 scl = convert_vector3(scale);
                animation.data[frame_offset + 7] = scl.x;
                animation.data[frame_offset + 8] = scl.y;
                animation.data[frame_offset + 9] = scl.z;
            }
        }
    }
    
    return animation;
}

GLTFBoneFilter GLTFLoader::load_bone_filter_from_meta(const std::filesystem::path& meta_path)
{
    GLTFBoneFilter filter;
    // TODO: Implement meta file loading
    return filter;
}


// Helper functions for vector conversion
static vec3 convert_vector3(float* vector3)
{
    if (!vector3)
        return vec3(0.0f);

    return vec3(vector3[0], vector3[2], vector3[1]);
}

static vec2 convert_vector2(float* vector2)
{
    if (!vector2)
        return vec2(0.0f);

    return vec2(vector2[0], vector2[1]);
}

static vec2 convert_uv(float* vector2)
{
    if (!vector2)
        return vec2(0.0f);

    return vec2(vector2[0], vector2[1]);
}

static quat convert_quaternion(float* quaternion)
{
    if (!quaternion)
        return quat(1.0f, 0.0f, 0.0f, 0.0f);

    return quat(quaternion[3], quaternion[0], quaternion[1], quaternion[2]);
}
