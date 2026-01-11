//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"
#include "../editor.h"

using namespace noz::editor::shape;

namespace noz::editor {

    constexpr float SPRITE_EDITOR_EDGE_WIDTH = 0.02f;
    constexpr float SPRITE_EDITOR_VERTEX_SIZE = 0.12f;
    constexpr Color SPRITE_EDITOR_VERTEX_COLOR = COLOR_BLACK;
    constexpr Color SPRITE_EDITOR_VERTEX_SELECTED_COLOR = Color32ToColor(255, 121, 0, 255);
    constexpr Color SPRITE_EDITOR_EDGE_COLOR = COLOR_BLACK;
    constexpr Color SPRITE_EDITOR_EDGE_SELECTED_COLOR = Color32ToColor(253, 151, 11, 255);

    constexpr ElementId SPRITE_EDITOR_ID_TOOLBAR = OVERLAY_BASE_ID + 0;
    constexpr ElementId SPRITE_EDITOR_ID_EXPAND = OVERLAY_BASE_ID + 1;
    constexpr ElementId SPRITE_EDITOR_ID_TILE = OVERLAY_BASE_ID + 2;
    constexpr ElementId SPRITE_EDITOR_ID_ATLAS = OVERLAY_BASE_ID + 3;
    constexpr ElementId SPRITE_EDITOR_ID_VERTEX_MODE = OVERLAY_BASE_ID + 4;
    constexpr ElementId SPRITE_EDITOR_ID_EDGE_MODE = OVERLAY_BASE_ID + 5;
    constexpr ElementId SPRITE_EDITOR_ID_FACE_MODE = OVERLAY_BASE_ID + 6;
    constexpr ElementId SPRITE_EDITOR_ID_WEIGHT_MODE = OVERLAY_BASE_ID + 7;

    constexpr ElementId SPRITE_EDITOR_ID_OPACITY = static_cast<ElementId>(OVERLAY_BASE_ID + 8);
    constexpr ElementId SPRITE_EDITOR_ID_PALETTES = static_cast<ElementId>(SPRITE_EDITOR_ID_OPACITY + 12);
    constexpr ElementId SPRITE_EDITOR_ID_COLORS = static_cast<ElementId>(SPRITE_EDITOR_ID_PALETTES + MAX_PALETTES);

    // Toolbar-specific IDs
    constexpr ElementId SPRITE_EDITOR_ID_FILL = static_cast<ElementId>(OVERLAY_BASE_ID + 20);
    constexpr ElementId SPRITE_EDITOR_ID_STROKE = static_cast<ElementId>(OVERLAY_BASE_ID + 21);
    constexpr ElementId SPRITE_EDITOR_ID_APPLY = static_cast<ElementId>(OVERLAY_BASE_ID + 22);
    constexpr ElementId SPRITE_EDITOR_ID_PALETTE_TOGGLE = static_cast<ElementId>(OVERLAY_BASE_ID + 23);
    constexpr ElementId SPRITE_EDITOR_ID_OPACITY_TOGGLE = static_cast<ElementId>(OVERLAY_BASE_ID + 24);

    struct SpriteEditor {
        u16 current_frame = 0;
        int zoom_version = 0;
        Mesh* mesh;

        // UI state
        int selection_color = 0;
        int selection_opacity = 0; // 0..10 (like mesh editor)
        bool show_palette_picker = false;
        bool show_opacity_popup = false;
        bool edit_fill = true; // true=fill, false=stroke
        bool is_playing = false;
    };

    static SpriteEditor g_sprite_editor = {};

    static SpriteDocument* GetSpriteDocument() {
        return (SpriteDocument*)g_editor.active_document;
    }

    static void UpdateSpriteEditorMesh(SpriteDocument* sdoc, bool hide_selected) {
        const float line_width = SPRITE_EDITOR_EDGE_WIDTH * g_workspace.zoom_ref_scale;
        const float selected_line_width = line_width * 2.5f;
        const float vertex_size = SPRITE_EDITOR_VERTEX_SIZE * g_workspace.zoom_ref_scale;

        MeshBuilder* builder = g_editor.mesh_builder;
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];

        // Draw paths - each path has its own anchors, segments are implicit between consecutive anchors
        for (u16 p_idx = 0; p_idx < f->shape.path_count; p_idx++) {
            const Path* p = &f->shape.paths[p_idx];
            bool path_selected = shape::IsSelected(p);

            if (p->anchor_count < 2) continue;

            // Draw segments between consecutive anchors (closed path)
            for (u16 a_idx = 0; a_idx < p->anchor_count; a_idx++) {
                const Anchor* a0 = GetAnchor(&f->shape, p, a_idx);
                const Anchor* a1 = GetAnchor(&f->shape, p, (a_idx + 1) % p->anchor_count);

                bool anchor_selected = shape::IsSelected(a0) || shape::IsSelected(a1);
                Color seg_color = (anchor_selected || path_selected)
                    ? SPRITE_EDITOR_EDGE_SELECTED_COLOR
                    : SPRITE_EDITOR_EDGE_COLOR;
                float seg_width = (anchor_selected || path_selected) ? selected_line_width : line_width;

                // Check if this segment is curved (anchor's curve defines offset for segment to next point)
                if (Abs(a0->curve) < FLT_EPSILON) {
                    // Straight line
                    AddEditorLine(builder, a0->position, a1->position, seg_width, seg_color);
                } else {
                    // Curved segment - curve is offset from center perpendicular to the line
                    Vec2 mid = (a0->position + a1->position) * 0.5f;
                    Vec2 dir = a1->position - a0->position;
                    Vec2 perp = Normalize(Vec2{-dir.y, dir.x});
                    Vec2 cp = mid + perp * a0->curve;

                    // Sample the quadratic bezier
                    Vec2 v0 = a0->position;
                    for (int i = 1; i <= SHAPE_MAX_SEGMENT_SAMPLES; i++) {
                        float t = static_cast<float>(i) / static_cast<float>(SHAPE_MAX_SEGMENT_SAMPLES + 1);
                        float u = 1.0f - t;
                        Vec2 v1 = (a0->position * (u * u)) + (cp * (2.0f * u * t)) + (a1->position * (t * t));
                        AddEditorLine(builder, v0, v1, seg_width, seg_color);
                        v0 = v1;
                    }
                    AddEditorLine(builder, v0, a1->position, seg_width, seg_color);
                }
            }

            // Draw anchors for this path
            for (u16 a_idx = 0; a_idx < p->anchor_count; a_idx++) {
                const shape::Anchor* a = GetAnchor(&f->shape, (Path*)p, a_idx);
                Color vcol = shape::IsSelected(a)
                    ? SPRITE_EDITOR_VERTEX_SELECTED_COLOR
                    : SPRITE_EDITOR_VERTEX_COLOR;
                AddEditorSquare(builder, a->position, vertex_size, vcol);
            }
        }

        Free(g_sprite_editor.mesh);
        g_sprite_editor.mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, nullptr, true);
        Clear(g_editor.mesh_builder);
    }

    static void BeginSpriteEditor(Document* doc) {
        g_sprite_editor.current_frame = 0;
        g_sprite_editor.zoom_version = -1;

        // Initialize UI state to sensible defaults for this document
        g_sprite_editor.selection_color = 0;
        g_sprite_editor.selection_opacity = 0;
        g_sprite_editor.show_palette_picker = false;
        g_sprite_editor.show_opacity_popup = false;
        g_sprite_editor.edit_fill = true;
    }

    static void EndSpriteEditor() {
        Free(g_sprite_editor.mesh);
        g_sprite_editor.mesh = nullptr;
    }
    

    static bool Palette(int palette_index, bool* selected_colors) {
        BeginContainer({
            .padding=EdgeInsetsAll(COLOR_PICKER_BORDER_WIDTH),
            .id = static_cast<ElementId>(SPRITE_EDITOR_ID_PALETTES + palette_index)
        });

        bool hovered = !selected_colors && IsHovered();
        if (hovered) {
            BeginContainer({
                .margin=EdgeInsetsAll(-COLOR_PICKER_BORDER_WIDTH*3),
                .padding=EdgeInsetsAll(COLOR_PICKER_BORDER_WIDTH*3),
                .border={.radius=STYLE_OVERLAY_CONTENT_BORDER_RADIUS,.width=COLOR_PICKER_BORDER_WIDTH,.color=STYLE_SELECTION_COLOR()}
            });
            EndContainer();
        }

        BeginColumn();

        Label(g_editor.palettes[palette_index].name, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_OVERLAY_TEXT_SIZE,
            .color=hovered ? STYLE_OVERLAY_ACCENT_TEXT_COLOR() : STYLE_OVERLAY_TEXT_COLOR(),
            .align=ALIGN_LEFT});

        Spacer(2.0f);

        BeginGrid({.columns=32, .cell={COLOR_PICKER_COLOR_SIZE, COLOR_PICKER_COLOR_SIZE}});
        for (int i=0; i<COLOR_COUNT; i++) {
            bool selected = (selected_colors && selected_colors[i]);
            Color color = g_editor.palettes[palette_index].colors[i];
            BeginContainer({
                .width=COLOR_PICKER_COLOR_SIZE,
                .height=COLOR_PICKER_COLOR_SIZE,
                .id=selected_colors ? static_cast<ElementId>(SPRITE_EDITOR_ID_COLORS + i) : ELEMENT_ID_NONE
            });

            if (selected)
                Container({
                    .width=COLOR_PICKER_COLOR_SIZE + 2,
                    .height=COLOR_PICKER_COLOR_SIZE + 2,
                    .align=ALIGN_CENTER,
                    .margin=EdgeInsetsAll(-2),
                    .border={.radius=8.0f,.width=2.5f,.color=STYLE_SELECTION_COLOR()}
                });

            Container({
                .width=COLOR_PICKER_COLOR_SIZE - 4,
                .height=COLOR_PICKER_COLOR_SIZE - 4,
                .align=ALIGN_CENTER,
                .color=color.a > 0 ? color : COLOR_BLACK_10PCT,
                .border={.radius=6.0f,},
            });

            if (selected_colors && WasPressed()) {
#if 0                
                if (IsShiftDown()) {
                    ClearSelection(&f->shape);
                    // Shift+click: select all faces with this color
                    bool any_selected = false;
                    for (u16 face_index = 0; face_index < frame->geom.face_count; face_index++) {
                        FaceData* f = GetFace(frame, face_index);
                        if (f->color == i) {
                            SetFlags(f, FACE_FLAG_SELECTED, FACE_FLAG_SELECTED);
                            any_selected = true;
                        }
                    }
                    if (any_selected)
                        UpdateSelection();
                } else {
                    MeshFrameData* frame = GetCurrentFrame(m);
                    if (frame->selected_face_count > 0) {
                        RecordUndo(m);
                        SetFaceColor(m, static_cast<u8>(i));
                        MarkModified(m);
                        UpdateSelectionColor();
                    } else {
                        g_sprite_editor.selection_color = i;
                    }
                }
#endif                
            }
            EndContainer();
        }
        EndGrid();
        EndColumn();

        bool pressed = !selected_colors && WasPressed();

        EndContainer();

        return pressed;
    }

    void UpdateSelectionColor() {
    }

    void UpdateSelection() {
    }

    static bool OpacityPopup() {
        SpriteDocument* m = GetSpriteDocument();
        SpriteFrame* f = GetFrame(m, g_sprite_editor.current_frame);
        bool open = true;

        BeginPopup({.anchor=ALIGN_TOP_LEFT, .align=ALIGN_BOTTOM_LEFT});
        open = !IsClosed();

        BeginContainer({
            .padding=EdgeInsetsAll(6),
            .color=STYLE_OVERLAY_BACKGROUND_COLOR(),
            .border{.radius=STYLE_OVERLAY_CONTENT_BORDER_RADIUS}});
        BeginColumn({.spacing=3});
        for (int i=0; i<=10; i++) {
            float opacity = 1.0f - (i / 10.0f);
            BeginContainer({.id=static_cast<ElementId>(SPRITE_EDITOR_ID_OPACITY + i + 1)});
            if (WasPressed()) {
#if 0                
                if (f->selected_face_count > 0) {
                    RecordUndo(m);
                    SetFaceOpacity(m, opacity);
                    MarkModified(m);
                    UpdateSelectionColor();
                } else {
                    g_sprite_editor.selection_opacity = i;
                }
#endif                

                open = false;
            }

            BeginContainer({
                .width=STYLE_BUTTON_HEIGHT,
                .height=STYLE_BUTTON_HEIGHT,
                .border={.radius=STYLE_BUTTON_BORDER_RADIUS},
                .clip=false
            });

            if (IsHovered() || g_sprite_editor.selection_opacity == 10 - i)
                Container({
                    .margin=EdgeInsetsAll(-2),
                    .padding=EdgeInsetsAll(2),
                    .color=STYLE_SELECTION_COLOR(),
                    .border={.radius=STYLE_BUTTON_BORDER_RADIUS},
                });

            BeginContainer();
            Image(MESH_ICON_OPACITY, {.align=ALIGN_CENTER, .material=g_workspace.editor_mesh_material});
            Image(MESH_ICON_OPACITY_OVERLAY, {
                .align=ALIGN_CENTER,
                .color=SetAlpha(COLOR_WHITE, opacity),
                .material=g_workspace.editor_mesh_material});
            EndContainer();

            EndContainer();
            EndContainer();
        }
        EndColumn();
        EndContainer();
        EndPopup();

        return open;
    }

    static void OpacityContent() {
        Image(MESH_ICON_OPACITY, {.align=ALIGN_CENTER, .material=g_workspace.editor_mesh_material});
        Image(MESH_ICON_OPACITY_OVERLAY, {
            .align=ALIGN_CENTER,
            .color=SetAlpha(COLOR_WHITE, g_sprite_editor.selection_opacity / 10.0f),
            .material=g_workspace.editor_mesh_material
        });
    }

    static void ColorPicker(){
        // todo: we could cache this when the mesh is modified or selection changes update it
        static bool selected_colors[COLOR_COUNT] = {};
        memset(selected_colors, 0, sizeof(selected_colors));
        SpriteDocument* mdoc = GetSpriteDocument();
        SpriteFrame* frame = GetFrame(mdoc, g_sprite_editor.current_frame);
#if 0        
        if (frame->selected_face_count > 0) {
            for (u16 face_index=0; face_index<frame->geom.face_count; face_index++) {
                FaceData* f = GetFace(frame, face_index);
                if (!IsSelected(f)) continue;
                selected_colors[f->color] = true;
            }
        } else {
            selected_colors[g_sprite_editor.selection_color] = true;
        }
#endif        

        // palettes
        BeginContainer({.padding=EdgeInsetsAll(4), .color=STYLE_OVERLAY_CONTENT_COLOR(), .border{.radius=STYLE_OVERLAY_CONTENT_BORDER_RADIUS}});
        BeginColumn();
        {
            int current_palette_index = g_editor.palette_map[mdoc->palette];
            if (g_sprite_editor.show_palette_picker) {
                for (int palette_index=0; palette_index<g_editor.palette_count; palette_index++) {
                    if (palette_index == current_palette_index) continue;
                    if (Palette(palette_index, nullptr)) {
                        RecordUndo(mdoc);
                        mdoc->palette = static_cast<u8>(g_editor.palettes[palette_index].id);
                        //MarkDirty(mdoc);
                        MarkModified(mdoc);
                    }

                    Spacer(2.0f);
                }
            }

            BeginRow();
            Palette(current_palette_index, g_sprite_editor.is_playing ? nullptr : selected_colors);
            BeginContainer({.align=ALIGN_BOTTOM_CENTER, .margin=EdgeInsetsAll(4)});
            if (EditorButton({
                .id=SPRITE_EDITOR_ID_OPACITY,
                .width = COLOR_PICKER_COLOR_SIZE * 2,
                .height = COLOR_PICKER_COLOR_SIZE * 2,
                .checked=g_sprite_editor.show_opacity_popup,
                .content_func=OpacityContent,
                .popup_func=OpacityPopup}))
                g_sprite_editor.show_opacity_popup = !g_sprite_editor.show_opacity_popup;
            EndContainer();

            EndRow();
        }
        EndColumn();
        EndContainer();
    }


    static void SpriteEditorToolbar() {
        SpriteDocument* sdoc = GetSpriteDocument();

        BeginOverlay(ELEMENT_ID_NONE, ALIGN_BOTTOM_CENTER);
        BeginColumn({.spacing=8});
        ColorPicker();
        EndColumn();
        EndOverlay();
    }

    static void DrawSpriteEditor() {
        SpriteDocument* doc = GetSpriteDocument();

        if (g_sprite_editor.zoom_version != g_workspace.zoom_version) {
            g_sprite_editor.zoom_version = g_workspace.zoom_version;
            UpdateSpriteEditorMesh(doc, false);
        }

        // On left click run shape hit test and update selection (convert mouse to document-local space)
        SpriteFrame* f = &doc->frames[g_sprite_editor.current_frame];
        if (WasButtonPressed(MOUSE_LEFT)) {
            shape::HitResult hr;
            Vec2 local_mouse = g_workspace.mouse_world_position - doc->position;

            // Clear existing selection
            ClearSelection(&f->shape);

            // if (HitTest(&f->shape, local_mouse, &hr)) {
            //     if (hr.anchor_index != U16_MAX) {
            //         SetFlags(&f->shape.anchors[hr.anchor_index], shape::ANCHOR_FLAG_SELECTED, shape::ANCHOR_FLAG_SELECTED);
            //     } else if (hr.segment_index != U16_MAX) {
            //         SetFlags(&f->shape.segments[hr.segment_index], shape::SEGMENT_FLAG_SELECTED, shape::SEGMENT_FLAG_SELECTED);
            //     } else if (hr.path_index != U16_MAX) {
            //         SetFlags(&f->shape.paths[hr.path_index], shape::PATH_FLAG_SELECTED, shape::PATH_FLAG_SELECTED);
            //     }
            // }

            g_sprite_editor.zoom_version = -1;
        }

        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindDepth(0.0f);
        DrawMesh(g_sprite_editor.mesh, Translate(doc->position));
    }

    void SpriteEditorOverlay() {
        SpriteEditorToolbar();
    }

    void InitSpriteEditor(SpriteDocument* sdoc) {
        sdoc->vtable.editor_begin = BeginSpriteEditor;
        sdoc->vtable.editor_end = EndSpriteEditor;
        sdoc->vtable.editor_draw = DrawSpriteEditor;
        sdoc->vtable.editor_overlay = SpriteEditorOverlay;
    }

    void InitSpriteEditor() {
    }    
}