#pragma once

namespace noz::renderer
{
    // Binary model data structure (shared with import system)
    struct ModelData
    {
        uint16_t vertexCount;
        uint16_t indexCount;
        bool hasNormals;
        bool hasUVs;
        bool hasBoneIndices;
        bool hasAnimations;
        uint8_t padding[2]; // For alignment
    };
    
    // Bone data structure for serialization
    struct BoneData
    {
        char name[64];
        int32_t index;
        int32_t parentIndex;
        glm::mat4 worldToLocal;
        glm::mat4 localToWorld;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        float length;
        glm::vec3 direction;
    };
    
    // Animation track data structure for serialization
    struct AnimationTrackData
    {
        int32_t boneIndex;
        int32_t trackType; // AnimationTrackType as int
        int32_t componentCount;
        int32_t dataOffset;
    };
    
    // Animation data structure for serialization
    struct AnimationData
    {
        char name[64];
        int32_t frameCount;
        uint32_t frameStride;
        uint32_t trackCount;
        uint32_t dataSize;
    };
    
    // Resource metadata structure (shared with import system)
    struct ResourceMetadata
    {
        char sourcePath[256];
        char outputPath[256];
        char assetType[32];
        char hash[64];
        uint32_t dependencyCount;
        char dependencies[16][256]; // Up to 16 dependencies
        int64_t lastModified;
        int64_t lastImported;
    };
} 