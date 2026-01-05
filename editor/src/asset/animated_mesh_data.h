//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "mesh_data.h"

struct AnimatedMeshDataImpl {
    MeshData frames[ANIMATED_MESH_MAX_FRAMES];
    int frame_count;
    int current_frame;
    AnimatedMesh* playing;
    float play_time;
};

struct AnimatedMeshData : AssetData {
    AnimatedMeshDataImpl* impl;
};

extern void InitAnimatedMeshData(AssetData* a);
extern AssetData* NewAnimatedMeshData(const std::filesystem::path& path);
extern AnimatedMesh* ToAnimatedMesh(AnimatedMeshData* m);
