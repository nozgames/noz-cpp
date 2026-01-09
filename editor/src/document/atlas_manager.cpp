//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <algorithm>
#include <vector>
#include <string>

namespace fs = std::filesystem;

static std::vector<AtlasDocument*> g_managed_atlases;

// Get the atlas prefix from config
static const char* GetManagedAtlasPrefix() {
    return g_editor.atlas.prefix.value;
}

static bool IsManagedAtlasName(const Name* name) {
    if (!name) return false;
    const char* prefix = GetManagedAtlasPrefix();
    return strncmp(name->value, prefix, strlen(prefix)) == 0;
}

static int GetNextAtlasIndex() {
    int max_index = -1;
    const char* prefix = GetManagedAtlasPrefix();
    size_t prefix_len = strlen(prefix);
    for (AtlasDocument* atlas : g_managed_atlases) {
        if (atlas && atlas->name) {
            const char* suffix = atlas->name->value + prefix_len;
            int index = atoi(suffix);
            if (index > max_index) max_index = index;
        }
    }
    return max_index + 1;
}

static AtlasDocument* CreateManagedAtlas() {
    int index = GetNextAtlasIndex();
    String64 name;
    Format(name, "%s%02d", GetManagedAtlasPrefix(), index);

    // Create the atlas file
    fs::path atlas_path = fs::path(g_editor.project_path) / g_editor.save_dir / "atlas" / name.value;
    atlas_path += ".atlas";
    fs::create_directories(atlas_path.parent_path());

    // Write default atlas content with configured size
    int size = g_editor.atlas.size;
    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 256);
    WriteCSTR(stream, "w %d\n", size);
    WriteCSTR(stream, "h %d\n", size);
    WriteCSTR(stream, "d %d\n", g_editor.atlas.dpi);
    SaveStream(stream, atlas_path);
    Free(stream);

    // Create and load the asset
    AtlasDocument* atlas = static_cast<AtlasDocument*>(CreateAssetData(atlas_path));
    if (atlas) {
        // Load the atlas to initialize the packer
        if (atlas->vtable.load) {
            atlas->vtable.load(atlas);
        }

        // Register as managed
        g_managed_atlases.push_back(atlas);
        LogInfo("Created managed atlas: %s (%dx%d)", name, size, size);
    }

    return atlas;
}

void InitAtlasManager() {
    g_managed_atlases.clear();
}

void ShutdownAtlasManager() {
    g_managed_atlases.clear();
}

void RegisterManagedAtlas(AtlasDocument* atlas) {
    if (!atlas) return;

    // Check if already registered
    for (AtlasDocument* existing : g_managed_atlases) {
        if (existing == atlas) return;
    }

    // Only register if it has the managed prefix
    if (IsManagedAtlasName(atlas->name)) {
        g_managed_atlases.push_back(atlas);
    }
}

void UnregisterManagedAtlas(AtlasDocument* atlas) {
    auto it = std::find(g_managed_atlases.begin(), g_managed_atlases.end(), atlas);
    if (it != g_managed_atlases.end()) {
        g_managed_atlases.erase(it);
    }
}

bool IsManagedAtlas(AtlasDocument* atlas) {
    if (!atlas) return false;
    return IsManagedAtlasName(atlas->name);
}

AtlasDocument* GetManagedAtlas(int index) {
    if (index < 0 || index >= (int)g_managed_atlases.size()) return nullptr;
    return g_managed_atlases[index];
}

int GetManagedAtlasCount() {
    return (int)g_managed_atlases.size();
}

int GetAtlasIndex(AtlasDocument* atlas) {
    for (int i = 0; i < (int)g_managed_atlases.size(); i++) {
        if (g_managed_atlases[i] == atlas) return i;
    }
    return 0;  // Default to 0 if not found
}

bool NeedsAtlasAssignment(MeshData* mesh) {
    if (!mesh || !mesh->impl) return false;

    // If mesh already has an atlas pointer and is in that atlas, it's assigned
    if (mesh->impl->atlas) {
        AtlasRect* rect = FindRectForMesh(mesh->impl->atlas, mesh->name);
        if (rect) return false;
    }

    // Check if mesh is in any atlas
    AtlasDocument* existing = FindAtlasForMesh(mesh->name);
    if (existing) {
        // Update the mesh's atlas pointer
        mesh->impl->atlas = existing;
        return false;
    }

    return true;
}

AtlasDocument* AutoAssignMeshToAtlas(MeshData* mesh) {
    if (!mesh || !mesh->impl) return nullptr;

    // Check if already assigned
    if (!NeedsAtlasAssignment(mesh)) {
        return mesh->impl->atlas;
    }

    // Try existing managed atlases first
    for (AtlasDocument* atlas : g_managed_atlases) {
        if (!atlas) continue;

        AtlasRect* rect = AllocateRect(atlas, mesh);
        if (rect) {
            RenderMeshToAtlas(atlas, mesh, *rect);
            mesh->impl->atlas = atlas;
            MarkModified(atlas);
            LogInfo("Assigned mesh '%s' to atlas '%s'", mesh->name->value, atlas->name->value);
            return atlas;
        }
    }

    // All existing atlases are full, create a new one
    AtlasDocument* new_atlas = CreateManagedAtlas();
    if (!new_atlas) {
        LogError("Failed to create new atlas for mesh '%s'", mesh->name->value);
        return nullptr;
    }

    AtlasRect* rect = AllocateRect(new_atlas, mesh);
    if (rect) {
        RenderMeshToAtlas(new_atlas, mesh, *rect);
        mesh->impl->atlas = new_atlas;
        MarkModified(new_atlas);
        LogInfo("Assigned mesh '%s' to new atlas '%s'", mesh->name->value, new_atlas->name->value);
        return new_atlas;
    }

    // Mesh is too large for atlas
    LogError("Mesh '%s' is too large for atlas size %d", mesh->name->value, g_editor.atlas.size);
    return nullptr;
}

// Extract name prefix for sorting (everything before last underscore, or full name)
static std::string GetNamePrefix(const Name* name) {
    if (!name) return "";
    std::string str = name->value;
    size_t pos = str.rfind('_');
    if (pos != std::string::npos && pos > 0) {
        return str.substr(0, pos);
    }
    return str;
}

void RebuildAllAtlases() {
    LogInfo("Rebuilding all atlases...");

    // Collect all meshes that should be in managed atlases
    struct MeshInfo {
        MeshData* mesh;
        std::string prefix;
        std::string name;
    };
    std::vector<MeshInfo> all_meshes;

    for (u32 i = 0; i < GetDocumentCount(); i++) {
        Document* asset = GetDocument(i);
        if (asset->type != ASSET_TYPE_MESH) continue;

        MeshData* mesh = static_cast<MeshData*>(asset);
        if (!mesh->impl) continue;

        // Include meshes that are in managed atlases OR need assignment
        bool in_managed = mesh->impl->atlas && IsManagedAtlas(mesh->impl->atlas);
        bool needs_assignment = NeedsAtlasAssignment(mesh);

        if (in_managed || needs_assignment) {
            all_meshes.push_back({
                mesh,
                GetNamePrefix(mesh->name),
                mesh->name->value
            });
        }
    }

    if (all_meshes.empty()) {
        LogInfo("No meshes to rebuild");
        return;
    }

    // Sort by prefix first, then by full name for consistent ordering
    std::sort(all_meshes.begin(), all_meshes.end(),
        [](const MeshInfo& a, const MeshInfo& b) {
            int cmp = a.prefix.compare(b.prefix);
            if (cmp != 0) return cmp < 0;
            return a.name < b.name;
        });

    LogInfo("Rebuilding %d meshes sorted by prefix", (int)all_meshes.size());

    // Clear all mesh atlas references
    for (const MeshInfo& info : all_meshes) {
        info.mesh->impl->atlas = nullptr;
    }

    // Clear existing atlases (rects + pixels) but keep the files
    for (AtlasDocument* atlas : g_managed_atlases) {
        if (!atlas || !atlas->impl) continue;
        ClearAllRects(atlas);
        if (atlas->impl->pixels) {
            memset(atlas->impl->pixels, 0, atlas->impl->size.x * atlas->impl->size.y * 4);
        }
        atlas->impl->dirty = true;
        MarkModified(atlas);
    }

    // Re-assign meshes in sorted order (will use existing atlases first, create new if needed)
    int success_count = 0;
    for (const MeshInfo& info : all_meshes) {
        AtlasDocument* atlas = AutoAssignMeshToAtlas(info.mesh);
        if (atlas) {
            success_count++;
        } else {
            LogError("Failed to reassign mesh '%s'", info.name.c_str());
        }
    }

    // Delete any atlases that ended up empty
    std::vector<AtlasDocument*> empty_atlases;
    for (AtlasDocument* atlas : g_managed_atlases) {
        if (!atlas) continue;

        bool has_meshes = false;
        for (int i = 0; i < atlas->impl->rect_count; i++) {
            if (atlas->impl->rects[i].valid) {
                has_meshes = true;
                break;
            }
        }

        if (!has_meshes) {
            empty_atlases.push_back(atlas);
        }
    }

    for (AtlasDocument* empty : empty_atlases) {
        LogInfo("Deleting unused atlas: %s", empty->name->value);
        UnregisterManagedAtlas(empty);
        DeleteAsset(empty);
    }

    // Rebuild sorted asset list to include any newly created atlases
    SortAssets();

    LogInfo("Atlas rebuild complete: %d/%d meshes assigned to %d atlases",
        success_count, (int)all_meshes.size(), (int)g_managed_atlases.size());
}

void MarkMeshAtlasDirty(MeshData* mesh) {
    if (!mesh || !mesh->impl) return;
    mesh->impl->atlas_dirty = true;
}

// Calculate required rect size for a mesh
static void GetRequiredRectSize(MeshData* mesh, AtlasDocument* atlas, int* out_width, int* out_height) {
    MeshDataImpl* mesh_impl = mesh->impl;
    AtlasDataImpl* impl = atlas->impl;

    if (mesh_impl->frame_count == 0) {
        *out_width = 0;
        *out_height = 0;
        return;
    }

    // Use mesh bounds (already computed as max across all frames)
    Vec2 frame_size = GetSize(mesh->bounds);
    int frame_width = (int)(frame_size.x * impl->dpi) + g_editor.atlas.padding * 2;
    int frame_height = (int)(frame_size.y * impl->dpi) + g_editor.atlas.padding * 2;

    *out_width = frame_width * mesh_impl->frame_count;
    *out_height = frame_height;
}

void UpdateDirtyMeshAtlases() {
    for (u32 i = 0; i < GetDocumentCount(); i++) {
        Document* asset = GetDocument(i);
        if (asset->type != ASSET_TYPE_MESH) continue;

        MeshData* mesh = static_cast<MeshData*>(asset);
        if (!mesh->impl || !mesh->impl->atlas_dirty) continue;

        mesh->impl->atlas_dirty = false;

        AtlasRect* rect = nullptr;
        AtlasDocument* atlas = FindAtlasForMesh(mesh->name, &rect);

        if (!atlas || !rect) continue;  // Not in an atlas yet

        // Calculate new required size
        int required_width, required_height;
        GetRequiredRectSize(mesh, atlas, &required_width, &required_height);

        bool fits = (required_width <= rect->width &&
                     required_height <= rect->height &&
                     mesh->impl->frame_count == rect->frame_count);

        if (fits) {
            // SIMPLE PATH: Clear and rerender in same spot
            ClearRectPixels(atlas, *rect);
            RenderMeshToAtlas(atlas, mesh, *rect);
            MarkModified(atlas);
        } else {
            // BIGGER: Burn old spot, allocate new rect
            LogInfo("Mesh '%s' changed size, allocating new rect", mesh->name->value);
            ClearRectPixels(atlas, *rect);  // Clear old pixels
            rect->valid = false;             // Mark old rect invalid (dead space)
            rect->mesh_name = nullptr;

            // Try to allocate new rect in same atlas
            AtlasRect* new_rect = AllocateRect(atlas, mesh);
            if (new_rect) {
                RenderMeshToAtlas(atlas, mesh, *new_rect);
                MarkModified(atlas);
            } else {
                // Atlas full - try other atlases or create new
                AtlasDocument* new_atlas = AutoAssignMeshToAtlas(mesh);
                if (new_atlas) {
                    MarkModified(new_atlas);
                }
            }
        }
    }
    // NO automatic RebuildAllAtlases() - let user run reatlas command to optimize
}
