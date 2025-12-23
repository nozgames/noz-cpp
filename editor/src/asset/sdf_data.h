//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct SdfFacePreview {
    Mesh* mesh;                 // Single quad for this face
    int color;                  // Color index for this face
};

struct SdfRuntimeData {
    std::vector<SdfFacePreview> preview_faces;  // Per-face quad meshes with colors
};

struct SdfData : AssetData {
    MeshData mesh;              // The mesh we edit (reuses mesh editor)
    SdfRuntimeData* runtime;    // Runtime data (non-blittable)

    // SDF generation settings
    float sdf_range;            // SDF distance range in world units (default 4.0)
    int min_resolution;         // Minimum SDF resolution per face (default 16)
    float pixels_per_unit;      // Resolution scaling factor (default 32.0)

    // Preview (generated from mesh for SDF rendering)
    Texture* preview_atlas;     // SDF atlas texture
    Material* preview_material; // Material with SDF shader
    bool preview_dirty;         // Needs regeneration
    noz::Task preview_task;     // Background generation task
};

extern void InitSdfData(AssetData* a);
extern AssetData* NewSdfData(const std::filesystem::path& path);
extern void GenerateSdfPreview(SdfData* s);
extern void MarkSdfPreviewDirty(SdfData* s);
