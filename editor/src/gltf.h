//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#pragma once

// Forward declarations
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;
struct cgltf_animation;
struct cgltf_animation_channel;
struct cgltf_skin;
struct cgltf_primitive;
struct cgltf_accessor;

// @types
struct GLTFBone
{
    std::string name;
    Mat4 world_to_local;
    Mat4 local_to_world;
    Vec3 position;
    Vec3 scale;
    Vec3 direction;
    int index;
    int parent_index;
    float rotation;
    float length;
};

struct GLTFAnimation
{
    int frame_count;
    int frame_stride;
    std::vector<void*> tracks;  // vector of animation_track_t
    std::vector<float> data;
};

struct GLTFMesh
{
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<Color> colors;
    std::vector<float> outlines;
    std::vector<uint32_t> bone_indices;
    std::vector<uint16_t> indices;
};

struct GLTFBoneFilter
{
    std::vector<std::string> exclude_bones;
    bool keep_leaf_bones = false;
};

class GLTFLoader
{
    cgltf_data* data = nullptr;
    std::filesystem::path path;

public:
    GLTFLoader() = default;
    ~GLTFLoader() { close(); }
    
    // Non-copyable
    GLTFLoader(const GLTFLoader&) = delete;
    GLTFLoader& operator=(const GLTFLoader&) = delete;
    
    // Movable
    GLTFLoader(GLTFLoader&& other) noexcept : data(other.data), path(std::move(other.path))
    {
        other.data = nullptr;
    }
    
    GLTFLoader& operator=(GLTFLoader&& other) noexcept
    {
        if (this != &other)
        {
            close();
            data = other.data;
            path = std::move(other.path);
            other.data = nullptr;
        }
        return *this;
    }
    
    bool open(const std::filesystem::path& file_path);
    void close();
    
    std::vector<GLTFBone> read_bones(const GLTFBoneFilter& filter = {});
    GLTFMesh read_mesh(const std::vector<GLTFBone>& bones = {});
    GLTFAnimation read_animation(const std::vector<GLTFBone>& bones, const std::string& animation_name);
    
    // Helper methods
    GLTFBoneFilter load_bone_filter_from_meta(const std::filesystem::path& meta_path);
    
private:
    void process_bone_recursive(struct cgltf_node* node, std::vector<GLTFBone>& bones, int parent_index, const GLTFBoneFilter& filter);
};