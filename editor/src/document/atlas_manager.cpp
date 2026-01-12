//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"
#include "utils/pixel_data.h"

namespace fs = std::filesystem;

namespace noz::editor {

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
        AtlasDocument* atlas = static_cast<AtlasDocument*>(CreateDocument(atlas_path));
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

    bool NeedsAtlasAssignment(SpriteDocument* sdoc) {
        if (!sdoc) return false;

        // If sprite already has an atlas pointer and is in that atlas, it's assigned
        if (sdoc->atlas) {
            AtlasRect* rect = FindRectForSprite(sdoc->atlas, sdoc->name);
            if (rect) return false;
        }

        // Check if sprite is in any atlas
        AtlasDocument* existing = FindAtlasForSprite(sdoc->name);
        if (existing) {
            // Update the sprite's atlas pointer
            sdoc->atlas = existing;
            return false;
        }

        return true;
    }

    AtlasDocument* AutoAssignSpriteToAtlas(SpriteDocument* sdoc) {
        if (!sdoc) return nullptr;

        // Check if already assigned
        if (!NeedsAtlasAssignment(sdoc)) {
            return sdoc->atlas;
        }

        // Try existing managed atlases first
        for (AtlasDocument* atlas : g_managed_atlases) {
            if (!atlas) continue;

            AtlasRect* rect = AllocateRect(atlas, sdoc);
            if (rect) {
                RenderSpriteToAtlas(atlas, sdoc, *rect);
                sdoc->atlas = atlas;
                MarkModified(atlas);
                LogInfo("Assigned sprite '%s' to atlas '%s'", sdoc->name->value, atlas->name->value);
                return atlas;
            }
        }

        // All existing atlases are full, create a new one
        AtlasDocument* new_atlas = CreateManagedAtlas();
        if (!new_atlas) {
            LogError("Failed to create new atlas for sprite '%s'", sdoc->name->value);
            return nullptr;
        }

        AtlasRect* rect = AllocateRect(new_atlas, sdoc);
        if (rect) {
            RenderSpriteToAtlas(new_atlas, sdoc, *rect);
            sdoc->atlas = new_atlas;
            MarkModified(new_atlas);
            LogInfo("Assigned sprite '%s' to new atlas '%s'", sdoc->name->value, new_atlas->name->value);
            return new_atlas;
        }

        // Sprite is too large for atlas
        LogError("Sprite '%s' is too large for atlas size %d", sdoc->name->value, g_editor.atlas.size);
        return nullptr;
    }

    void RebuildAllAtlases() {
        LogInfo("Rebuilding all atlases...");

        struct SpriteInfo {
            SpriteDocument* sdoc;
            std::string prefix;
            std::string name;
        };
        std::vector<SpriteInfo> all_sprites;

        for (int i = 0; i < GetDocumentCount(); i++) {
            Document* asset = GetDocument(i);
            if (asset->def->type != ASSET_TYPE_SPRITE) continue;

            SpriteDocument* sdoc = static_cast<SpriteDocument*>(asset);
            if (!sdoc) continue;

            // Include sprites that are in managed atlases OR need assignment
            bool in_managed = sdoc->atlas && IsManagedAtlas(sdoc->atlas);
            bool needs_assignment = NeedsAtlasAssignment(sdoc);

            if (in_managed || needs_assignment) {
                all_sprites.push_back({
                    sdoc,
                    GetNamePrefix(sdoc->name),
                    sdoc->name->value
                });
            }
        }

        if (all_sprites.empty()) {
            LogInfo("No sprites to rebuild");
            return;
        }

        // Sort by prefix first, then by full name for consistent ordering
        std::sort(all_sprites.begin(), all_sprites.end(),
            [](const SpriteInfo& a, const SpriteInfo& b) {
                int cmp = a.prefix.compare(b.prefix);
                if (cmp != 0) return cmp < 0;
                return a.name < b.name;
            });

        LogInfo("Rebuilding %d sprites sorted by prefix", (int)all_sprites.size());

        // Clear all sprite atlas references
        for (const SpriteInfo& info : all_sprites) {
            info.sdoc->atlas = nullptr;
        }

        // Clear existing atlases (rects + pixels) but keep the files
        for (AtlasDocument* adoc : g_managed_atlases) {
            if (!adoc) continue;
            ClearAllRects(adoc);
            if (adoc->pixels) {
                Clear(adoc->pixels);
            }
            adoc->dirty = true;
            MarkModified(adoc);
        }

        // Re-assign sprites in sorted order (will use existing atlases first, create new if needed)
        int success_count = 0;
        for (const SpriteInfo& info : all_sprites) {
            AtlasDocument* atlas = AutoAssignSpriteToAtlas(info.sdoc);
            if (atlas) {
                success_count++;
            } else {
                LogError("Failed to reassign sprite '%s'", info.name.c_str());
            }
        }

        // Delete any atlases that ended up empty
        std::vector<AtlasDocument*> empty_atlases;
        for (AtlasDocument* adoc : g_managed_atlases) {
            if (!adoc) continue;

            bool has_sprites = false;
            for (int i = 0; i < adoc->rect_count; i++) {
                if (adoc->rects[i].valid) {
                    has_sprites = true;
                    break;
                }
            }

            if (!has_sprites) {
                empty_atlases.push_back(adoc);
            }
        }

        for (AtlasDocument* empty : empty_atlases) {
            LogInfo("Deleting unused atlas: %s", empty->name->value);
            UnregisterManagedAtlas(empty);
            DeleteAsset(empty);
        }

        LogInfo("Atlas rebuild complete: %d/%d sprites assigned to %d atlases",
            success_count, (int)all_sprites.size(), (int)g_managed_atlases.size());
    }

    void MarkSpriteAtlasDirty(SpriteDocument* sdoc) {
        if (!sdoc) return;
        sdoc->atlas_dirty = true;
    }

    // Calculate required rect size for a sprite
    void GetRequiredRectSize(SpriteDocument* sdoc, AtlasDocument* adoc, int* out_width, int* out_height) {
        if (sdoc->frame_count == 0) {
            *out_width = 0;
            *out_height = 0;
            return;
        }

        // Calculate max raster bounds across all frames
        RectInt max_bounds = {0, 0, 0, 0};
        for (u16 fi = 0; fi < sdoc->frame_count; ++fi) {
            shape::UpdateSamples(&sdoc->frames[fi].shape);
            shape::UpdateBounds(&sdoc->frames[fi].shape);
            RectInt& rb = sdoc->frames[fi].shape.raster_bounds;
            if (fi == 0) {
                max_bounds = rb;
            } else {
                int min_x = Min(max_bounds.x, rb.x);
                int min_y = Min(max_bounds.y, rb.y);
                int max_x = Max(max_bounds.x + max_bounds.w, rb.x + rb.w);
                int max_y = Max(max_bounds.y + max_bounds.h, rb.y + rb.h);
                max_bounds = {min_x, min_y, max_x - min_x, max_y - min_y};
            }
        }

        int frame_width = max_bounds.w + g_editor.atlas.padding * 2;
        int frame_height = max_bounds.h + g_editor.atlas.padding * 2;

        *out_width = frame_width * sdoc->frame_count;
        *out_height = frame_height;
    }

    void UpdateDirtySpriteAtlases() {
#if 0        
        for (int doc_index = 0; doc_index < GetDocumentCount(); doc_index++) {
            Document* doc = GetDocument(doc_index);
            if (doc->def->type != ASSET_TYPE_SPRITE) continue;

            SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);
            if (!sdoc || !sdoc->atlas_dirty) continue;

            sdoc->atlas_dirty = false;

            AtlasRect* rect = nullptr;
            AtlasDocument* atlas = FindAtlasForSprite(sdoc->name, &rect);

            if (!atlas || !rect) {
                // Not in an atlas yet - try to assign
                if (sdoc->frame_count > 0) {
                    AutoAssignSpriteToAtlas(sdoc);
                }
                continue;
            }

            // Calculate new required size
            int required_width, required_height;
            GetRequiredRectSize(sdoc, atlas, &required_width, &required_height);

            bool fits = (required_width <= rect->width &&
                         required_height <= rect->height &&
                         sdoc->frame_count == rect->frame_count);

            if (fits) {
                // SIMPLE PATH: Clear and rerender in same spot
                ClearRectPixels(atlas, *rect);
                RenderSpriteToAtlas(atlas, sdoc, *rect);
                MarkModified(atlas);
            } else {
                // BIGGER: Burn old spot, allocate new rect
                LogInfo("Sprite '%s' changed size, allocating new rect", sdoc->name->value);
                ClearRectPixels(atlas, *rect);  // Clear old pixels
                rect->valid = false;             // Mark old rect invalid (dead space)
                rect->asset_name = nullptr;

                // Try to allocate new rect in same atlas
                AtlasRect* new_rect = AllocateRect(atlas, sdoc);
                if (new_rect) {
                    RenderSpriteToAtlas(atlas, sdoc, *new_rect);
                    MarkModified(atlas);
                } else {
                    // Atlas full - try other atlases or create new
                    AtlasDocument* new_atlas = AutoAssignSpriteToAtlas(sdoc);
                    if (new_atlas) {
                        MarkModified(new_atlas);
                    }
                }
            }
        }
#endif            
    }
}
