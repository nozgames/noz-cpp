//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {
    constexpr int ATLAS_EDITOR_ID_TOOLBAR = OVERLAY_BASE_ID + 0;
    constexpr int ATLAS_EDITOR_ID_REBUILD = OVERLAY_BASE_ID + 1;

    static AtlasDocument* GetAtlasDocument() {
        return static_cast<AtlasDocument*>(GetActiveDocument());
    }

    static void AtlasEditorToolbar() {
        AtlasDocument* adoc = GetAtlasDocument();

        BeginOverlay(ATLAS_EDITOR_ID_TOOLBAR, ALIGN_BOTTOM_CENTER);
        BeginColumn({.spacing=8});

        // Toolbar buttons row
        BeginContainer();
        BeginRow({.align=ALIGN_CENTER, .spacing=6});

        // Rebuild button - clears and reallocates all rects
        if (EditorButton(ATLAS_EDITOR_ID_REBUILD, SPRITE_ICON_LOOP, false)) {
            RebuildAtlas(adoc);
            MarkModified(adoc);
            AddNotification(NOTIFICATION_TYPE_INFO, "Atlas '%s' rebuilt", adoc->name->value);
        }

        EndRow();
        EndContainer();

        // Stats row: show attached mesh count
        BeginContainer();
        BeginRow({.align=ALIGN_CENTER, .spacing=4});
        char stats[64];
        snprintf(stats, sizeof(stats), "%d meshes", adoc->rect_count);
        Label(stats, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_TEXT_COLOR(), .align=ALIGN_CENTER});
        EndRow();
        EndContainer();

        EndColumn();
        EndOverlay();
    }

    static void AtlasEditorOverlay() {
        AtlasEditorToolbar();
    }

    static void BeginAtlasEditor(Document*) {
        SetFocus(CANVAS_ID_OVERLAY, ATLAS_EDITOR_ID_REBUILD);
    }

    static void EndAtlasEditor() {
        // Nothing special needed for now
    }

    static void UpdateAtlasEditor() {
        // Handle input updates if needed
    }

    static void DrawAtlasEditor() {
        AtlasDocument* atlas = GetAtlasDocument();
        Document* a = atlas;

        a->vtable.draw(a);

        Mesh* outline = GetAtlasOutlineMesh(atlas);
        if (outline) {
            BindMaterial(g_workspace.editor_material);
            BindColor(COLOR_YELLOW);
            BindDepth(0.1f);  // In front of grid (at 0)
            DrawMesh(outline, Translate(a->position));
        }
    }

    void InitAtlasEditor(AtlasDocument* adoc) {
        adoc->vtable.editor_begin = BeginAtlasEditor;
        adoc->vtable.editor_end = EndAtlasEditor;
        adoc->vtable.editor_update = UpdateAtlasEditor;
        adoc->vtable.editor_draw = DrawAtlasEditor;
        adoc->vtable.editor_overlay = AtlasEditorOverlay;
    }
}
