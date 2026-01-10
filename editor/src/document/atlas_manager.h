//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct MeshDocument;
    struct AtlasDocument;

    // Initialize/shutdown atlas manager
    extern void InitAtlasManager();
    extern void ShutdownAtlasManager();

    // Auto-assign mesh to an atlas (lazy - finds or creates atlas as needed)
    // Returns the atlas the mesh was assigned to, or nullptr on failure
    extern AtlasDocument* AutoAssignMeshToAtlas(MeshDocument* mesh);

    // Check if mesh needs atlas assignment (no atlas or not in any atlas)
    extern bool NeedsAtlasAssignment(MeshDocument* mesh);

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

    // Mark mesh as needing atlas re-render (call when mesh is modified)
    extern void MarkMeshAtlasDirty(MeshDocument* mesh);

    // Update all dirty mesh atlases (call before saving assets)
    extern void UpdateDirtyMeshAtlases();
}
