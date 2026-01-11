//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct SpriteDocument;
    struct AtlasDocument;

    // Initialize/shutdown atlas manager
    extern void InitAtlasManager();
    extern void ShutdownAtlasManager();

    // Auto-assign sprite to an atlas (lazy - finds or creates atlas as needed)
    // Returns the atlas the sprite was assigned to, or nullptr on failure
    extern AtlasDocument* AutoAssignSpriteToAtlas(SpriteDocument* sprite);

    // Check if sprite needs atlas assignment (no atlas or not in any atlas)
    extern bool NeedsAtlasAssignment(SpriteDocument* sprite);

    // Manual rebuild command - clears all managed atlases, sorts by name prefix, repacks optimally
    extern void RebuildAllAtlases();

    // Register an existing atlas as managed (called during post-load for auto-managed atlases)
    extern void RegisterManagedAtlas(AtlasDocument* adoc);

    // Unregister a managed atlas
    extern void UnregisterManagedAtlas(AtlasDocument* adoc);

    // Check if an atlas is auto-managed
    extern bool IsManagedAtlas(AtlasDocument* adoc);

    // Get managed atlas by index
    extern AtlasDocument* GetManagedAtlas(int index);
    extern int GetManagedAtlasCount();
    extern int GetAtlasIndex(AtlasDocument* adoc);

    // Mark sprite as needing atlas re-render (call when sprite is modified)
    extern void MarkSpriteAtlasDirty(SpriteDocument* sprite);

    // Update all dirty sprite atlases (call before saving assets)
    extern void UpdateDirtySpriteAtlases();
}
