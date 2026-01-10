//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"

namespace noz::editor {

    constexpr float SPRITE_EDITOR_EDGE_WIDTH = 0.02f;
    constexpr float SPRITE_EDITOR_VERTEX_SIZE = 0.12f;
    constexpr Color SPRITE_EDITOR_VERTEX_SELECTED_COLOR = Color32ToColor(255, 121, 0, 255);
    constexpr Color SPRITE_EDITOR_VERTEX_VERTEX = COLOR_BLACK;

    struct SpriteEditor {
        int current_frame = 0;
        int zoom_version = 0;
        Mesh* mesh;
    };

    static SpriteEditor g_sprite_editor = {};

    static SpriteDocument* GetSpriteDocument() {
        return (SpriteDocument*)g_editor.active_document;
    }

    static void UpdateSpriteEditorMesh(SpriteDocument* sdoc, bool hide_selected) {
        const float line_width = SPRITE_EDITOR_EDGE_WIDTH * g_workspace.zoom_ref_scale;
        const float selected_line_width = line_width * 2.5f;
        const float vertex_size = SPRITE_EDITOR_VERTEX_SIZE * g_workspace.zoom_ref_scale;
        const float origin_size = 0.1f * g_workspace.zoom_ref_scale;

        MeshBuilder* builder = g_editor.mesh_builder;
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        for (u16 p_idx = 0; p_idx < f->shape.path_count; p_idx++) {
            const shape::Path* p = &f->shape.paths[p_idx];
            for (u16 s_idx = 0; s_idx < p->segment_count; s_idx++) {
                const shape::Segment* s = &f->shape.segments[s_idx];
                shape::UpdateSegmentSamples(&f->shape, s_idx);

                if (s->sample_count == 0) {
                    AddEditorLine(
                        builder, f->shape.anchors[s->anchor0].position, f->shape.anchors[s->anchor1].position,
                        line_width, hide_selected && sdoc->selected ? Color{1, 1, 1, 0.25f} : Color{1, 1, 1, 1}
                    );
                } else {
                    Vec2 v0 = f->shape.anchors[s->anchor0].position;
                    for (u16 sample_idx = 0; sample_idx < s->sample_count - 1; sample_idx++) {
                        Vec2 v1 = s->samples[sample_idx + 1];
                        AddEditorLine(
                            builder, v0, v1,
                            line_width, hide_selected && sdoc->selected ? Color{1, 1, 1, 0.25f} : Color{1, 1, 1, 1}
                        );
                        v0 = v1;
                    }

                    AddEditorLine(
                        builder,
                        v0, f->shape.anchors[s->anchor1].position,
                        line_width,
                        hide_selected && sdoc->selected ? Color{1, 1, 1, 0.25f} : Color{1, 1, 1, 1}
                    );
                }
            }
        }

        for (u16 a_idx = 0; a_idx < f->shape.anchor_count; a_idx++) {
            const shape::Anchor* a = f->shape.anchors + a_idx;
            //Color color = IsSelected(a) ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
            AddEditorSquare(builder, a->position, vertex_size, SPRITE_EDITOR_VERTEX_VERTEX);
        }
        
        Free(g_sprite_editor.mesh);
        g_sprite_editor.mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, nullptr, true);
        Clear(g_editor.mesh_builder);
    }

    static void BeginSpriteEditor(Document* doc) {
        SpriteDocument* sdoc = (SpriteDocument*)doc;
        g_sprite_editor.current_frame = 0;
        g_sprite_editor.zoom_version = -1;
    }

    static void EndSpriteEditor() {
        Free(g_sprite_editor.mesh);
        g_sprite_editor.mesh = nullptr;
    }

    static void DrawSpriteEditor() {
        SpriteDocument* doc = GetSpriteDocument();

        if (g_sprite_editor.zoom_version != g_workspace.zoom_version) {
            g_sprite_editor.zoom_version = g_workspace.zoom_version;
            UpdateSpriteEditorMesh(doc, false);
        }

        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindDepth(0.0f);
        DrawMesh(g_sprite_editor.mesh, Translate(doc->position));       
    }

    void InitSpriteEditor(SpriteDocument* sdoc) {
        sdoc->vtable.editor_begin = BeginSpriteEditor;
        sdoc->vtable.editor_end = EndSpriteEditor;
        sdoc->vtable.editor_draw = DrawSpriteEditor;
    }

    void InitSpriteEditor() {
    }    
}