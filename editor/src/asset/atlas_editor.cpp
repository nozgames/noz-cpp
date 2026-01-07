//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

extern Font* FONT_SEGUISB;

constexpr int ATLAS_EDITOR_ID_TOOLBAR = OVERLAY_BASE_ID + 0;
constexpr int ATLAS_EDITOR_ID_REBUILD = OVERLAY_BASE_ID + 1;

static AtlasData* GetAtlasData() {
    return static_cast<AtlasData*>(GetAssetData());
}

static Bounds2 GetAtlasBounds() {
    AssetData* a = GetAssetData();
    return a->bounds;
}

static void AtlasEditorToolbar() {
    AtlasData* atlas = GetAtlasData();
    AtlasDataImpl* impl = atlas->impl;

    BeginOverlay(ATLAS_EDITOR_ID_TOOLBAR, ALIGN_BOTTOM_CENTER);
    BeginColumn({.spacing=8});

    // Toolbar buttons row
    BeginContainer();
    BeginRow({.align=ALIGN_CENTER, .spacing=6});

    // Rebuild button - clears and reallocates all rects
    if (EditorButton(ATLAS_EDITOR_ID_REBUILD, MESH_ICON_LOOP, false)) {
        RebuildAtlas(atlas);
        MarkModified(atlas);
        AddNotification(NOTIFICATION_TYPE_INFO, "Atlas '%s' rebuilt", atlas->name->value);
    }

    EndRow();
    EndContainer();

    // Stats row: show attached mesh count
    BeginContainer();
    BeginRow({.align=ALIGN_CENTER, .spacing=4});
    char stats[64];
    snprintf(stats, sizeof(stats), "%d meshes", impl->rect_count);
    Label(stats, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_TEXT_COLOR(), .align=ALIGN_CENTER});
    EndRow();
    EndContainer();

    EndColumn();
    EndOverlay();
}

static void AtlasEditorOverlay() {
    AtlasEditorToolbar();
}

static void BeginAtlasEditor(AssetData*) {
    SetFocus(CANVAS_ID_OVERLAY, ATLAS_EDITOR_ID_REBUILD);
}

static void EndAtlasEditor() {
    // Nothing special needed for now
}

static void UpdateAtlasEditor() {
    // Handle input updates if needed
}

static void DrawAtlasEditor() {
    AtlasData* atlas = GetAtlasData();
    AssetData* a = atlas;

    // Draw the atlas itself (texture, bounds, etc)
    a->vtable.draw(a);

    // Draw export mesh outlines (single mesh for all rects)
    Mesh* outline = GetAtlasOutlineMesh(atlas);
    if (outline) {
        BindMaterial(g_view.editor_material);
        BindColor(COLOR_YELLOW);
        BindDepth(0.1f);  // In front of grid (at 0)
        DrawMesh(outline, Translate(a->position));
    }
}

void InitAtlasEditor(AtlasData* atlas) {
    atlas->vtable.editor_bounds = GetAtlasBounds;
    atlas->vtable.editor_begin = BeginAtlasEditor;
    atlas->vtable.editor_end = EndAtlasEditor;
    atlas->vtable.editor_update = UpdateAtlasEditor;
    atlas->vtable.editor_draw = DrawAtlasEditor;
    atlas->vtable.editor_overlay = AtlasEditorOverlay;
}
