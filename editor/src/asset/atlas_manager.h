//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct MeshData;
struct AtlasData;

// Initialize/shutdown atlas manager
extern void InitAtlasManager();
extern void ShutdownAtlasManager();

// Auto-assign mesh to an atlas (lazy - finds or creates atlas as needed)
// Returns the atlas the mesh was assigned to, or nullptr on failure
extern AtlasData* AutoAssignMeshToAtlas(MeshData* mesh);

// Check if mesh needs atlas assignment (no atlas or not in any atlas)
extern bool NeedsAtlasAssignment(MeshData* mesh);

// Manual rebuild command - clears all managed atlases, sorts by name prefix, repacks optimally
extern void RebuildAllAtlases();

// Register an existing atlas as managed (called during post-load for auto-managed atlases)
extern void RegisterManagedAtlas(AtlasData* atlas);

// Unregister a managed atlas
extern void UnregisterManagedAtlas(AtlasData* atlas);

// Check if an atlas is auto-managed
extern bool IsManagedAtlas(AtlasData* atlas);

// Get managed atlas by index
extern AtlasData* GetManagedAtlas(int index);
extern int GetManagedAtlasCount();

// Mark mesh as needing atlas re-render (call when mesh is modified)
extern void MarkMeshAtlasDirty(MeshData* mesh);

// Update all dirty mesh atlases (call before saving assets)
extern void UpdateDirtyMeshAtlases();
