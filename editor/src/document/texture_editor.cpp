//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {
    
    struct TextureEditor {
        InputSet* input;
        Shortcut* shortcuts;
        float saved_scale;
    };

    extern void DrawTextureDocument(Document* a);

    static TextureEditor g_texture_editor = {};

    inline TextureDocument* GetTextureDocument() {
        return static_cast<TextureDocument*>(GetActiveDocument());
    }

    static void BeginTextureEditor(Document*) {
        PushInputSet(g_texture_editor.input);
    }

    static void EndTextureEditor() {
        PopInputSet();
    }

    static void UpdateTextureEditor() {
        CheckShortcuts(g_texture_editor.shortcuts, g_texture_editor.input);
    }

    static void DrawTextureEditor() {
        TextureDocument* tdoc = GetTextureDocument();
        const MeshVertex* vertices = GetVertices(g_workspace.quad_mesh);
        DrawTextureDocument(tdoc);
        DrawBounds(tdoc, 0, COLOR_VERTEX_SELECTED);
        Vec2 size = GetSize(tdoc->bounds);
        DrawVertex(tdoc->position + vertices[0].position * size);
        DrawVertex(tdoc->position + vertices[1].position * size);
        DrawVertex(tdoc->position + vertices[2].position * size);
        DrawVertex(tdoc->position + vertices[3].position * size);
    }

    static void BeginTextureMove() {
    }

    static void UpdateTextureScaleTool(const Vec2& scale) {
        TextureDocument* tdoc = GetTextureDocument();
        tdoc->scale = g_texture_editor.saved_scale * scale.x;
        UpdateBounds(tdoc);
    }

    static void CommitTextureScaleTool(const Vec2&) {
        TextureDocument* tdoc = GetTextureDocument();
        MarkMetaModified(tdoc);
        MarkModified(tdoc);
    }

    static void CancelTextureScaleTool() {
        TextureDocument* tdoc = GetTextureDocument();
        tdoc->scale = g_texture_editor.saved_scale;
        UpdateBounds(tdoc);
    }

    static void BeginTextureScale() {
        TextureDocument* tdoc = GetTextureDocument();
        g_texture_editor.saved_scale = tdoc->scale;
        RecordUndo(tdoc);
        BeginScaleTool({
            .origin = tdoc->position,
            .update = UpdateTextureScaleTool,
            .commit = CommitTextureScaleTool,
            .cancel = CancelTextureScaleTool,
        });
    }

    void InitTextureEditor(TextureDocument* m) {
        m->vtable.editor_begin = m->editor_only ? BeginTextureEditor : nullptr;
        m->vtable.editor_end = EndTextureEditor;
        m->vtable.editor_update = UpdateTextureEditor;
        m->vtable.editor_draw = DrawTextureEditor;
    }

    void InitTextureEditor() {
        static Shortcut shortcuts[] = {
            { KEY_G, false, false, false, BeginTextureMove },
            { KEY_S, false, false, false, BeginTextureScale },
            { INPUT_CODE_NONE }
        };

        g_texture_editor.input = CreateInputSet(ALLOCATOR_DEFAULT);
        EnableCommonShortcuts(g_texture_editor.input);
        EnableButton(g_texture_editor.input, MOUSE_LEFT);
        EnableButton(g_texture_editor.input, MOUSE_SCROLL_Y);

        g_texture_editor.shortcuts = shortcuts;
        EnableShortcuts(shortcuts, g_texture_editor.input);
    }

}
