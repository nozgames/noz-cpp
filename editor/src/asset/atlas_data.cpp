//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <plutovg.h>
#include "utils/rect_packer.h"

using namespace noz;

extern void InitAtlasEditor(AtlasData* atlas);

static void AllocAtlasDataImpl(AssetData* a) {
    assert(a->type == ASSET_TYPE_ATLAS);
    AtlasData* atlas = static_cast<AtlasData*>(a);
    atlas->impl = static_cast<AtlasDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(AtlasDataImpl)));
    memset(atlas->impl, 0, sizeof(AtlasDataImpl));
    atlas->impl->width = ATLAS_DEFAULT_SIZE;
    atlas->impl->height = ATLAS_DEFAULT_SIZE;
    atlas->impl->dpi = ATLAS_DEFAULT_DPI;
    atlas->impl->dirty = true;
    atlas->impl->packer = new rect_packer(ATLAS_DEFAULT_SIZE, ATLAS_DEFAULT_SIZE);
}

static void DestroyAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    if (atlas->impl) {
        if (atlas->impl->pixels) {
            Free(atlas->impl->pixels);
            atlas->impl->pixels = nullptr;
        }
        if (atlas->impl->packer) {
            delete atlas->impl->packer;
            atlas->impl->packer = nullptr;
        }
        Free(atlas->impl);
        atlas->impl = nullptr;
    }
}

static void CloneAtlasData(AssetData* a) {
    assert(a->type == ASSET_TYPE_ATLAS);
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* old_impl = atlas->impl;
    int old_width = old_impl->width;
    int old_height = old_impl->height;
    AllocAtlasDataImpl(a);
    memcpy(atlas->impl, old_impl, sizeof(AtlasDataImpl));
    // Don't share pixel buffer or packer - force regeneration
    atlas->impl->pixels = nullptr;
    atlas->impl->texture = nullptr;
    atlas->impl->packer = new rect_packer(old_width, old_height);
    atlas->impl->dirty = true;
}

// Rect management

AtlasRect* FindRectForMesh(AtlasData* atlas, const Name* mesh_name) {
    AtlasDataImpl* impl = atlas->impl;
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name == mesh_name) {
            return &impl->rects[i];
        }
    }
    return nullptr;
}

static int FindFreeRectSlot(AtlasDataImpl* impl) {
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) return i;
    }
    if (impl->rect_count < ATLAS_MAX_RECTS) {
        return impl->rect_count++;
    }
    return -1;
}

AtlasRect* AllocateRect(AtlasData* atlas, const Name* mesh_name, const Bounds2& bounds) {
    AtlasDataImpl* impl = atlas->impl;

    // Calculate required size in pixels
    Vec2 size = GetSize(bounds);
    int req_width = (int)(size.x * impl->dpi) + 2;  // +2 for padding
    int req_height = (int)(size.y * impl->dpi) + 2;

    if (req_width > impl->width || req_height > impl->height) {
        return nullptr;  // Too large for atlas
    }

    // Find a free slot
    int slot = FindFreeRectSlot(impl);
    if (slot < 0) return nullptr;

    // Use rect_packer to find position
    rect_packer::BinRect bin_rect;
    int result = impl->packer->Insert(req_width, req_height, rect_packer::method::BestShortSideFit, bin_rect);
    if (result == 0) {
        return nullptr;  // No room
    }

    AtlasRect& rect = impl->rects[slot];
    rect.x = bin_rect.x;
    rect.y = bin_rect.y;
    rect.width = bin_rect.w;
    rect.height = bin_rect.h;
    rect.mesh_name = mesh_name;
    rect.mesh_bounds = bounds;
    rect.valid = true;

    impl->dirty = true;
    return &rect;
}

void FreeRect(AtlasData* atlas, AtlasRect* rect) {
    if (rect) {
        rect->valid = false;
        rect->mesh_name = nullptr;
        atlas->impl->dirty = true;
    }
}

void ClearAllRects(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    for (int i = 0; i < impl->rect_count; i++) {
        impl->rects[i].valid = false;
        impl->rects[i].mesh_name = nullptr;
    }
    impl->rect_count = 0;
    impl->dirty = true;

    // Reset packer
    if (impl->packer) {
        delete impl->packer;
    }
    impl->packer = new rect_packer(impl->width, impl->height);
}

// PlutoVG rendering

static void EnsurePixelBuffer(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels) {
        impl->pixels = (u8*)Alloc(ALLOCATOR_DEFAULT, impl->width * impl->height * 4);
        memset(impl->pixels, 0, impl->width * impl->height * 4);
    }
}

void RenderMeshToAtlas(AtlasData* atlas, MeshData* mesh, const AtlasRect& rect) {
    AtlasDataImpl* impl = atlas->impl;
    MeshDataImpl* mesh_impl = mesh->impl;

    EnsurePixelBuffer(atlas);

    // Create a surface for the entire atlas
    plutovg_surface_t* surface = plutovg_surface_create_for_data(
        impl->pixels, impl->width, impl->height, impl->width * 4
    );
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);

    // Clip to the rect
    plutovg_canvas_clip_rect(canvas, (float)rect.x, (float)rect.y, (float)rect.width, (float)rect.height);

    // Set up transform: mesh coords -> pixel coords
    // mesh_bounds maps to rect in pixels
    float scale = (float)impl->dpi;
    float offset_x = rect.x + 1 - rect.mesh_bounds.min.x * scale;
    float offset_y = rect.y + 1 - rect.mesh_bounds.min.y * scale;

    // Render each face
    for (int fi = 0; fi < mesh_impl->face_count; fi++) {
        FaceData& face = mesh_impl->faces[fi];
        if (face.vertex_count < 3) continue;

        plutovg_canvas_new_path(canvas);

        // Build path for this face
        for (int vi = 0; vi < face.vertex_count; vi++) {
            int v_idx = face.vertices[vi];
            Vec2 pos = mesh_impl->vertices[v_idx].position;

            // Transform to pixel coordinates
            float px = offset_x + pos.x * scale;
            float py = offset_y + pos.y * scale;

            if (vi == 0) {
                plutovg_canvas_move_to(canvas, px, py);
            } else {
                // Check if edge to this vertex is curved
                int prev_vi = (vi - 1 + face.vertex_count) % face.vertex_count;
                int prev_v_idx = face.vertices[prev_vi];

                // Find the edge between prev_v_idx and v_idx
                int edge_idx = GetEdge(mesh, prev_v_idx, v_idx);
                if (edge_idx >= 0 && IsEdgeCurved(mesh, edge_idx)) {
                    // Quadratic bezier curve
                    Vec2 control = GetEdgeControlPoint(mesh, edge_idx);
                    float cpx = offset_x + control.x * scale;
                    float cpy = offset_y + control.y * scale;
                    plutovg_canvas_quad_to(canvas, cpx, cpy, px, py);
                } else {
                    plutovg_canvas_line_to(canvas, px, py);
                }
            }
        }

        // Close the path - handle potential curve on last edge
        int last_v_idx = face.vertices[face.vertex_count - 1];
        int first_v_idx = face.vertices[0];
        int edge_idx = GetEdge(mesh, last_v_idx, first_v_idx);
        if (edge_idx >= 0 && IsEdgeCurved(mesh, edge_idx)) {
            Vec2 control = GetEdgeControlPoint(mesh, edge_idx);
            Vec2 first_pos = mesh_impl->vertices[first_v_idx].position;
            float cpx = offset_x + control.x * scale;
            float cpy = offset_y + control.y * scale;
            float px = offset_x + first_pos.x * scale;
            float py = offset_y + first_pos.y * scale;
            plutovg_canvas_quad_to(canvas, cpx, cpy, px, py);
        } else {
            plutovg_canvas_close_path(canvas);
        }

        // Get face color from palette
        int palette_index = g_editor.palette_map[mesh_impl->palette];
        Color color = g_editor.palettes[palette_index].colors[face.color];
        plutovg_canvas_set_rgba(canvas, color.r, color.g, color.b, color.a);
        plutovg_canvas_fill(canvas);
    }

    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);

    impl->dirty = true;
}

void SyncAtlasTexture(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;
    if (!impl->pixels || !impl->dirty) return;

    // Convert from premultiplied ARGB to RGBA for GPU upload
    // We need a temporary buffer since we want to keep impl->pixels in ARGB for further PlutoVG rendering
    u32 buffer_size = impl->width * impl->height * 4;
    u8* rgba_pixels = static_cast<u8 *>(Alloc(ALLOCATOR_SCRATCH, buffer_size));
    memcpy(rgba_pixels, impl->pixels, buffer_size);
    plutovg_convert_argb_to_rgba(rgba_pixels, rgba_pixels, impl->width, impl->height, impl->width * 4);

    if (!impl->texture) {
        impl->texture = CreateTexture(ALLOCATOR_DEFAULT, rgba_pixels, impl->width, impl->height,
                                       TEXTURE_FORMAT_RGBA8, GetName("atlas_texture"));
        impl->material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
        SetTexture(impl->material, impl->texture, 0);
    } else {
        UpdateTexture(impl->texture, rgba_pixels);
    }

    impl->dirty = false;
}

void RegenerateAtlas(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;

    // Clear pixel buffer
    if (impl->pixels) {
        memset(impl->pixels, 0, impl->width * impl->height * 4);
    }

    // Re-render all attached meshes
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        // Load the mesh
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
        if (mesh_asset) {
            MeshData* mesh = static_cast<MeshData*>(mesh_asset);
            RenderMeshToAtlas(atlas, mesh, impl->rects[i]);
        }
    }

    impl->dirty = true;
}

void UpdateAtlas(AtlasData* atlas) {
    AtlasDataImpl* impl = atlas->impl;

    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        // Load the mesh
        AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
        if (!mesh_asset) continue;

        MeshData* mesh = static_cast<MeshData*>(mesh_asset);
        Bounds2 bounds = mesh->bounds;

        if (impl->rects[i].mesh_bounds == bounds) {
            // Same bounds - just re-render into existing rect
            RenderMeshToAtlas(atlas, mesh, impl->rects[i]);
        } else {
            // Bounds changed - need new rect
            FreeRect(atlas, &impl->rects[i]);
            AtlasRect* new_rect = AllocateRect(atlas, impl->rects[i].mesh_name, bounds);
            if (!new_rect) {
                // No room - full regeneration needed
                ClearAllRects(atlas);
                RegenerateAtlas(atlas);
                return;
            }
            RenderMeshToAtlas(atlas, mesh, *new_rect);
        }
    }
}

// UV computation

Vec2 GetAtlasUV(AtlasData* atlas, const AtlasRect& rect, const Vec2& position) {
    AtlasDataImpl* impl = atlas->impl;

    // Map position from mesh space to UV space
    float u = (position.x - rect.mesh_bounds.min.x) / GetSize(rect.mesh_bounds).x;
    float v = (position.y - rect.mesh_bounds.min.y) / GetSize(rect.mesh_bounds).y;

    // Map to rect in atlas
    u = (rect.x + u * rect.width) / (float)impl->width;
    v = (rect.y + v * rect.height) / (float)impl->height;

    return {u, v};
}

// Save/Load

static void SaveAtlasData(AssetData* a, const std::filesystem::path& path) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Save atlas configuration
    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "w " << impl->width << "\n";
    file << "h " << impl->height << "\n";
    file << "dpi " << impl->dpi << "\n";

    // Save attached meshes
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name) {
            file << "m " << impl->rects[i].mesh_name->value << "\n";
        }
    }
}

static void LoadAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    std::ifstream file(a->path);
    if (!file.is_open()) return;

    std::vector<const Name*> mesh_names;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) continue;

        if (cmd == "w") {
            iss >> impl->width;
        } else if (cmd == "h") {
            iss >> impl->height;
        } else if (cmd == "dpi") {
            iss >> impl->dpi;
        } else if (cmd == "m") {
            std::string mesh_name;
            if (iss >> mesh_name) {
                mesh_names.push_back(GetName(mesh_name.c_str()));
            }
        }
    }

    // Attach meshes (they may not be loaded yet, so we store names and allocate rects later in post_load)
    for (const Name* mesh_name : mesh_names) {
        int slot = FindFreeRectSlot(impl);
        if (slot >= 0) {
            impl->rects[slot].mesh_name = mesh_name;
            impl->rects[slot].valid = false;  // Will be properly allocated in post_load
        }
    }
}

static void PostLoadAtlasData(AssetData* a) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Now allocate proper rects for all attached meshes
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].mesh_name && !impl->rects[i].valid) {
            AssetData* mesh_asset = GetAssetData(ASSET_TYPE_MESH, impl->rects[i].mesh_name);
            if (mesh_asset) {
                MeshData* mesh = static_cast<MeshData*>(mesh_asset);
                AtlasRect* rect = AllocateRect(atlas, impl->rects[i].mesh_name, mesh->bounds);
                if (rect) {
                    RenderMeshToAtlas(atlas, mesh, *rect);
                }
            }
        }
    }
}

static void LoadAtlasMetaData(AssetData* a, Props* meta) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    impl->width = meta->GetInt("atlas", "width", ATLAS_DEFAULT_SIZE);
    impl->height = meta->GetInt("atlas", "height", ATLAS_DEFAULT_SIZE);
    impl->dpi = meta->GetInt("atlas", "dpi", ATLAS_DEFAULT_DPI);

    // Resize packer to match loaded dimensions
    if (impl->packer) {
        impl->packer->Resize(impl->width, impl->height);
    }

    InitAtlasEditor(atlas);
}

static void SaveAtlasMetaData(AssetData* a, Props* meta) {
    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    meta->SetInt("atlas", "width", impl->width);
    meta->SetInt("atlas", "height", impl->height);
    meta->SetInt("atlas", "dpi", impl->dpi);
}

void InitAtlasData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ATLAS);

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AllocAtlasDataImpl(a);

    atlas->bounds = Bounds2{Vec2{-5.0f, -5.0f}, Vec2{5.0f, 5.0f}};
    atlas->vtable = {
        .destructor = DestroyAtlasData,
        .load = LoadAtlasData,
        .post_load = PostLoadAtlasData,
        .save = SaveAtlasData,
        .load_metadata = LoadAtlasMetaData,
        .save_metadata = SaveAtlasMetaData,
        .clone = CloneAtlasData,
    };
}

AssetData* NewAtlasData(const std::filesystem::path& path) {
    constexpr const char* default_atlas = "w 1024\nh 1024\ndpi 96\n";

    std::string type_folder = ToString(ASSET_TYPE_ATLAS);
    Lower(type_folder.data(), (u32)type_folder.size());

    std::filesystem::path full_path = path.is_relative()
        ? std::filesystem::path(g_editor.project_path) / g_editor.save_dir / type_folder / path
        : path;
    full_path += ".atlas";

    // Create directory if it doesn't exist
    std::filesystem::create_directories(full_path.parent_path());

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 256);
    WriteCSTR(stream, default_atlas);
    SaveStream(stream, full_path);
    Free(stream);

    AtlasData* atlas = static_cast<AtlasData*>(CreateAssetData(full_path));
    LoadAtlasData(atlas);
    return atlas;
}
