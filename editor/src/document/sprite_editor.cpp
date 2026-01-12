//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../utils/pixel_data.h"
#include "sprite_doc.h"

using namespace noz::editor::shape;

namespace noz::editor {

    constexpr float SPRITE_EDITOR_EDGE_WIDTH = 0.02f;
    constexpr float SPRITE_EDITOR_VERTEX_SIZE = 0.12f;
    constexpr float SPRITE_EDITOR_MIDPOINT_SIZE = 0.08f;
    constexpr Color SPRITE_EDITOR_VERTEX_COLOR = COLOR_BLACK;
    constexpr Color SPRITE_EDITOR_VERTEX_SELECTED_COLOR = Color32ToColor(255, 121, 0, 255);
    constexpr Color SPRITE_EDITOR_EDGE_COLOR = COLOR_BLACK;
    constexpr Color SPRITE_EDITOR_EDGE_SELECTED_COLOR = Color32ToColor(253, 151, 11, 255);
    constexpr Color SPRITE_EDITOR_MIDPOINT_COLOR = Color32ToColor(80, 80, 80, 255);

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
        int raster_version = 0;
        Texture* texture;
        PixelData* pixels;
        Mesh* mesh;
        Mesh* raster_mesh;
        Material* raster_material;
        InputSet* input;

        int selection_color = 0;
        int selection_opacity = 0;
        bool show_palette_picker = false;
        bool show_opacity_popup = false;
        bool edit_fill = true;
        bool is_playing = false;

        Vec2 saved_positions[SHAPE_MAX_ANCHORS];
        float saved_curves[SHAPE_MAX_ANCHORS];
        bool select_on_up = false;
        u16 pending_anchor = U16_MAX;

        u16 curve_drag_anchor = U16_MAX;
        Vec2 curve_drag_start;

        u16 hovered_midpoint = U16_MAX;
    };

    static void DeleteSelected();

    static SpriteEditor g_sprite_editor = {};

    static Shortcut g_sprite_editor_shortcuts[] = {
        { KEY_X, false, false, false, DeleteSelected, "Delete" },
        { INPUT_CODE_NONE }
    };


    static SpriteDocument* GetSpriteDocument() {
        return (SpriteDocument*)g_editor.active_document;
    }

    static void UpdateSpriteEditorMesh(SpriteDocument* sdoc, bool hide_selected) {
        const float line_width = SPRITE_EDITOR_EDGE_WIDTH * g_workspace.zoom_ref_scale;
        const float selected_line_width = line_width * 2.5f;
        const float vertex_size = SPRITE_EDITOR_VERTEX_SIZE * g_workspace.zoom_ref_scale;
        const float midpoint_size = SPRITE_EDITOR_MIDPOINT_SIZE * g_workspace.zoom_ref_scale;

        MeshBuilder* builder = g_editor.mesh_builder;
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];

        for (u16 p_idx = 0; p_idx < f->shape.path_count; p_idx++) {
            const Path* p = &f->shape.paths[p_idx];
            bool path_selected = shape::IsSelected(p);

            if (p->anchor_count < 2) continue;

            for (u16 a_idx = 0; a_idx < p->anchor_count; a_idx++) {
                const Anchor* a0 = GetAnchor(&f->shape, p, a_idx);
                const Anchor* a1 = GetAnchor(&f->shape, p, (a_idx + 1) % p->anchor_count);

                bool segment_selected = (shape::IsSelected(a0) && shape::IsSelected(a1)) || path_selected;
                Color seg_color = segment_selected
                    ? SPRITE_EDITOR_EDGE_SELECTED_COLOR
                    : SPRITE_EDITOR_EDGE_COLOR;
                float seg_width = segment_selected ? selected_line_width : line_width;

                Vec2 v0 = a0->position;
                for (int i = 0; i < SHAPE_MAX_SEGMENT_SAMPLES; i++) {
                    AddEditorLine(builder, v0, a0->samples[i], seg_width, seg_color);
                    v0 = a0->samples[i];
                }
                AddEditorLine(builder, v0, a1->position, seg_width, seg_color);
            }

            for (u16 a_idx = 0; a_idx < p->anchor_count; a_idx++) {
                u16 global_idx = p->anchor_start + a_idx;
                if (global_idx == g_sprite_editor.hovered_midpoint) {
                    const Anchor* a = GetAnchor(&f->shape, p, a_idx);
                    AddEditorCircle(builder, a->midpoint, midpoint_size, SPRITE_EDITOR_MIDPOINT_COLOR);
                }
            }

            for (u16 a_idx = 0; a_idx < p->anchor_count; a_idx++) {
                const shape::Anchor* a = GetAnchor(&f->shape, (Path*)p, a_idx);
                if (!shape::IsSelected(a))
                    AddEditorSquare(builder, a->position, vertex_size, SPRITE_EDITOR_VERTEX_COLOR);
            }
        }

        for (u16 p_idx = 0; p_idx < f->shape.path_count; p_idx++) {
            const Path* p = &f->shape.paths[p_idx];
            for (u16 a_idx = 0; a_idx < p->anchor_count; a_idx++) {
                const shape::Anchor* a = GetAnchor(&f->shape, (Path*)p, a_idx);
                if (shape::IsSelected(a))
                    AddEditorSquare(builder, a->position, vertex_size, SPRITE_EDITOR_VERTEX_SELECTED_COLOR);
            }
        }

        Free(g_sprite_editor.mesh);
        g_sprite_editor.mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, nullptr, true);
        Clear(g_editor.mesh_builder);
    }

    static void UpdateRaster(SpriteDocument* sdoc) {
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;

        // Update bounds and samples
        UpdateSamples(shape);
        UpdateBounds(shape);

        RectInt& rb = shape->raster_bounds;
        if (rb.w <= 0 || rb.h <= 0) return;

        // Clear pixels and rasterize
        Clear(g_sprite_editor.pixels);

        // Get palette colors
        int palette_index = g_editor.palette_map[sdoc->palette];
        const Color* palette = g_editor.palettes[palette_index].colors;

        // Rasterize with offset that maps raster_bounds origin to texture origin (0,0)
        Rasterize(shape, g_sprite_editor.pixels, palette, Vec2Int{-rb.x, -rb.y});

        // Update texture with new pixel data
        UpdateTexture(g_sprite_editor.texture, g_sprite_editor.pixels->rgba);

        // Build raster mesh aligned to pixel grid
        float dpi = (float)g_editor.atlas.dpi;
        float inv_dpi = 1.0f / dpi;

        Vec2 quad_min = Vec2{(float)rb.x, (float)rb.y} * inv_dpi;
        Vec2 quad_max = Vec2{(float)(rb.x + rb.w), (float)(rb.y + rb.h)} * inv_dpi;

        // UV coordinates: pixels are written starting at (0,0) in texture
        float tex_size = (float)g_sprite_editor.pixels->size.x;
        Vec2 uv_min = {0.0f, 0.0f};
        Vec2 uv_max = Vec2{(float)rb.w, (float)rb.h} / tex_size;

        Free(g_sprite_editor.raster_mesh);

        PushScratch();
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
        AddVertex(builder, MeshVertex{.position = {quad_min.x, quad_min.y}, .uv = {uv_min.x, uv_min.y}});
        AddVertex(builder, MeshVertex{.position = {quad_max.x, quad_min.y}, .uv = {uv_max.x, uv_min.y}});
        AddVertex(builder, MeshVertex{.position = {quad_max.x, quad_max.y}, .uv = {uv_max.x, uv_max.y}});
        AddVertex(builder, MeshVertex{.position = {quad_min.x, quad_max.y}, .uv = {uv_min.x, uv_max.y}});
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);
        g_sprite_editor.raster_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        Free(builder);
        PopScratch();
    }

    static void BeginSpriteEditor(Document* doc) {
        g_sprite_editor.current_frame = 0;
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = 0;
        g_sprite_editor.selection_color = 0;
        g_sprite_editor.selection_opacity = 0;
        g_sprite_editor.show_palette_picker = false;
        g_sprite_editor.show_opacity_popup = false;
        g_sprite_editor.edit_fill = true;
        g_sprite_editor.select_on_up = false;
        g_sprite_editor.pending_anchor = U16_MAX;
        g_sprite_editor.hovered_midpoint = U16_MAX;

        PushInputSet(g_sprite_editor.input);
        UpdateSpriteEditorMesh(static_cast<SpriteDocument*>(doc), false);
        UpdateRaster(static_cast<SpriteDocument*>(doc));
    }

    static void EndSpriteEditor() {
        SpriteDocument* sdoc = GetSpriteDocument();

        sdoc->atlas_dirty |= sdoc && sdoc->modified;

        Free(g_sprite_editor.mesh);
        Free(g_sprite_editor.raster_mesh);

        g_sprite_editor.mesh = nullptr;
        g_sprite_editor.raster_mesh = nullptr;

        PopInputSet();
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
            Image(SPRITE_ICON_OPACITY, {.align=ALIGN_CENTER, .material=g_workspace.editor_mesh_material});
            Image(SPRITE_ICON_OPACITY_OVERLAY, {
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
        Image(SPRITE_ICON_OPACITY, {.align=ALIGN_CENTER, .material=g_workspace.editor_mesh_material});
        Image(SPRITE_ICON_OPACITY_OVERLAY, {
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

    static void SelectPath(Shape* shape, u16 path_index, bool add_to_selection) {
        if (!add_to_selection)
            ClearSelection(shape);

        Path* p = &shape->paths[path_index];
        SetFlags(p, PATH_FLAG_SELECTED, PATH_FLAG_SELECTED);
        for (u16 a_idx = 0; a_idx < p->anchor_count; ++a_idx) {
            SetFlags(GetAnchor(shape, p, a_idx), ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
        }
    }

    static void HandleBoxSelect(const Bounds2& bounds) {
        SpriteDocument* doc = GetSpriteDocument();
        SpriteFrame* f = &doc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;

        Vec2 local_min = bounds.min - doc->position;
        Vec2 local_max = bounds.max - doc->position;

        if (!IsShiftDown())
            ClearSelection(shape);

        for (u16 i = 0; i < shape->anchor_count; ++i) {
            Anchor* a = &shape->anchors[i];
            if (a->position.x >= local_min.x && a->position.x <= local_max.x &&
                a->position.y >= local_min.y && a->position.y <= local_max.y) {
                SetFlags(a, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
            }
        }

        g_sprite_editor.zoom_version = -1;
    }

    static void SaveAnchorState() {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;
        for (u16 i = 0; i < shape->anchor_count; ++i)
            g_sprite_editor.saved_positions[i] = shape->anchors[i].position;
    }

    static void UpdateMoveTool(const Vec2& delta) {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;
        bool pixel_snap = IsCtrlDown();

        for (u16 i = 0; i < shape->anchor_count; ++i) {
            Anchor* a = &shape->anchors[i];
            if (IsSelected(a)) {
                Vec2 new_pos = sdoc->position + g_sprite_editor.saved_positions[i] + delta;
                if (pixel_snap)
                    new_pos = SnapToGrid(new_pos);
                a->position = new_pos - sdoc->position;
            }
        }

        shape::UpdateSamples(shape);
        UpdateBounds(sdoc);
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    static void CommitMoveTool(const Vec2& delta) {
        MarkModified();
    }

    static void CancelMoveTool() {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;

        for (u16 i = 0; i < shape->anchor_count; ++i)
            shape->anchors[i].position = g_sprite_editor.saved_positions[i];

        shape::UpdateSamples(shape);
        CancelUndo();
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    static bool HasSelection() {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;
        for (u16 i = 0; i < shape->anchor_count; ++i)
            if (IsSelected(&shape->anchors[i]))
                return true;
        return false;
    }

    static void BeginMoveTool() {
        if (!HasSelection())
            return;
        g_sprite_editor.hovered_midpoint = U16_MAX;
        SaveAnchorState();
        RecordUndo();
        BeginMoveTool({.update=UpdateMoveTool, .commit=CommitMoveTool, .cancel=CancelMoveTool});
    }

    static void SaveCurveState() {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;
        for (u16 i = 0; i < shape->anchor_count; ++i)
            g_sprite_editor.saved_curves[i] = shape->anchors[i].curve;
    }

    static void UpdateCurveTool(const Vec2& delta) {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;
        u16 anchor_idx = g_sprite_editor.curve_drag_anchor;
        if (anchor_idx == U16_MAX) return;

        u16 path_idx = U16_MAX;
        u16 local_idx = 0;
        for (u16 p = 0; p < shape->path_count; ++p) {
            Path* path = &shape->paths[p];
            if (anchor_idx >= path->anchor_start && anchor_idx < path->anchor_start + path->anchor_count) {
                path_idx = p;
                local_idx = anchor_idx - path->anchor_start;
                break;
            }
        }
        if (path_idx == U16_MAX) return;

        Path* path = &shape->paths[path_idx];
        Anchor* a0 = GetAnchor(shape, path, local_idx);
        Anchor* a1 = GetAnchor(shape, path, (local_idx + 1) % path->anchor_count);

        Vec2 dir = a1->position - a0->position;
        Vec2 normal = Normalize(Vec2{-dir.y, dir.x});
        float curve_delta = Dot(delta, normal);
        a0->curve = g_sprite_editor.saved_curves[anchor_idx] + curve_delta;

        shape::UpdateSamples(shape);
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    static void CommitCurveTool(const Vec2& delta) {
        g_sprite_editor.curve_drag_anchor = U16_MAX;
        MarkModified();
    }

    static void CancelCurveTool() {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;

        for (u16 i = 0; i < shape->anchor_count; ++i)
            shape->anchors[i].curve = g_sprite_editor.saved_curves[i];

        shape::UpdateSamples(shape);
        g_sprite_editor.curve_drag_anchor = U16_MAX;
        CancelUndo();
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    static void BeginCurveTool(u16 anchor_index) {
        g_sprite_editor.hovered_midpoint = U16_MAX;
        g_sprite_editor.curve_drag_anchor = anchor_index;
        SaveCurveState();
        RecordUndo();
        BeginMoveTool({.update=UpdateCurveTool, .commit=CommitCurveTool, .cancel=CancelCurveTool});
    }

    static void InsertAnchorAtMidpoint(u16 anchor_index) {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = &sdoc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;

        g_sprite_editor.hovered_midpoint = U16_MAX;

        RecordUndo();
        u16 new_idx = shape::SplitSegment(shape, anchor_index);
        if (new_idx != U16_MAX) {
            ClearSelection(shape);
            SetFlags(&shape->anchors[new_idx], ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
            SaveAnchorState();
            BeginMoveTool({.update=UpdateMoveTool, .commit=CommitMoveTool, .cancel=CancelMoveTool});
        }

        UpdateBounds(sdoc);
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    static void DrawSpriteEditor() {
        SpriteDocument* doc = GetSpriteDocument();

        SpriteFrame* f = &doc->frames[g_sprite_editor.current_frame];
        Shape* shape = &f->shape;
        Vec2 local_mouse = g_workspace.mouse_world_position - doc->position;
        bool shift_down = IsShiftDown();
        bool selection_changed = false;

        if (!IsToolActive()) {
            shape::HitResult hover_hr;
            HitTest(shape, local_mouse, &hover_hr);
            u16 new_hovered = (hover_hr.segment_index != U16_MAX || hover_hr.midpoint_index != U16_MAX)
                ? (hover_hr.midpoint_index != U16_MAX ? hover_hr.midpoint_index : hover_hr.segment_index)
                : U16_MAX;
            if (new_hovered != g_sprite_editor.hovered_midpoint) {
                g_sprite_editor.hovered_midpoint = new_hovered;
                g_sprite_editor.zoom_version = -1;
            }
        }

        if (!IsToolActive() && g_workspace.drag_started && g_workspace.drag_button == MOUSE_LEFT) {
            shape::HitResult hr;
            HitTest(shape, local_mouse, &hr);
            bool alt_down = IsAltDown();

            if (hr.anchor_index != U16_MAX) {
                Anchor* a = &shape->anchors[hr.anchor_index];
                if (!IsSelected(a)) {
                    if (!shift_down)
                        ClearSelection(shape);
                    SetFlags(a, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                    selection_changed = true;
                }
                g_sprite_editor.select_on_up = false;
                g_sprite_editor.pending_anchor = U16_MAX;
                BeginMoveTool();
            } else if (hr.midpoint_index != U16_MAX) {
                g_sprite_editor.select_on_up = false;
                g_sprite_editor.pending_anchor = U16_MAX;
                if (alt_down) {
                    BeginCurveTool(hr.midpoint_index);
                } else {
                    InsertAnchorAtMidpoint(hr.midpoint_index);
                }
            } else if (hr.segment_index != U16_MAX) {
                for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
                    Path* p = &shape->paths[p_idx];
                    if (hr.segment_index >= p->anchor_start && hr.segment_index < p->anchor_start + p->anchor_count) {
                        u16 local_idx = hr.segment_index - p->anchor_start;
                        u16 next_local_idx = (local_idx + 1) % p->anchor_count;
                        Anchor* a0 = GetAnchor(shape, p, local_idx);
                        Anchor* a1 = GetAnchor(shape, p, next_local_idx);
                        if (!IsSelected(a0) || !IsSelected(a1)) {
                            if (!shift_down)
                                ClearSelection(shape);
                            SetFlags(a0, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                            SetFlags(a1, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                            selection_changed = true;
                        }
                        break;
                    }
                }
                g_sprite_editor.select_on_up = false;
                g_sprite_editor.pending_anchor = U16_MAX;
                BeginMoveTool();
            } else {
                g_sprite_editor.hovered_midpoint = U16_MAX;
                BeginBoxSelect(HandleBoxSelect);
                if (!shift_down) {
                    ClearSelection(shape);
                    selection_changed = true;
                }
            }
        }

        if (WasButtonPressed(MOUSE_LEFT) && !g_workspace.drag_started) {
            shape::HitResult hr;
            bool hit = HitTest(shape, local_mouse, &hr);
            bool double_click = WasButtonPressed(MOUSE_LEFT_DOUBLE_CLICK);

            g_sprite_editor.select_on_up = false;
            g_sprite_editor.pending_anchor = U16_MAX;

            if (double_click) {
                if (hit && hr.path_index != U16_MAX) {
                    SelectPath(shape, hr.path_index, shift_down);
                    selection_changed = true;
                }
            } else if (hit && hr.anchor_index != U16_MAX) {
                Anchor* clicked_anchor = &shape->anchors[hr.anchor_index];
                if (IsSelected(clicked_anchor) && !shift_down) {
                    g_sprite_editor.select_on_up = true;
                    g_sprite_editor.pending_anchor = hr.anchor_index;
                } else if (shift_down) {
                    if (IsSelected(clicked_anchor))
                        ClearFlags(clicked_anchor, ANCHOR_FLAG_SELECTED);
                    else
                        SetFlags(clicked_anchor, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                    selection_changed = true;
                } else {
                    ClearSelection(shape);
                    SetFlags(clicked_anchor, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                    selection_changed = true;
                }
            } else if (hit && hr.segment_index != U16_MAX) {
                // Segment click - select both connected anchors
                for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
                    Path* p = &shape->paths[p_idx];
                    if (hr.segment_index >= p->anchor_start && hr.segment_index < p->anchor_start + p->anchor_count) {
                        u16 local_idx = hr.segment_index - p->anchor_start;
                        u16 next_local_idx = (local_idx + 1) % p->anchor_count;
                        Anchor* a0 = GetAnchor(shape, p, local_idx);
                        Anchor* a1 = GetAnchor(shape, p, next_local_idx);
                        // If both anchors already selected, don't change selection immediately (might be starting a drag)
                        if (IsSelected(a0) && IsSelected(a1) && !shift_down) {
                            // Do nothing - wait to see if this becomes a drag
                        } else {
                            if (!shift_down)
                                ClearSelection(shape);
                            SetFlags(a0, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                            SetFlags(a1, ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                            selection_changed = true;
                        }
                        break;
                    }
                }
            } else if (!hit && !shift_down) {
                ClearSelection(shape);
                selection_changed = true;
            }
        }

        if (WasButtonReleased(g_sprite_editor.input, MOUSE_LEFT) && g_sprite_editor.select_on_up) {
            if (g_sprite_editor.pending_anchor != U16_MAX) {
                ClearSelection(shape);
                SetFlags(&shape->anchors[g_sprite_editor.pending_anchor], ANCHOR_FLAG_SELECTED, ANCHOR_FLAG_SELECTED);
                selection_changed = true;
            }
            g_sprite_editor.select_on_up = false;
            g_sprite_editor.pending_anchor = U16_MAX;
        }

        if (selection_changed)
            g_sprite_editor.zoom_version = -1;

        // Draw rasterized texture behind the shape
        if (g_sprite_editor.raster_mesh) {
            BindMaterial(g_sprite_editor.raster_material);
            BindColor(COLOR_WHITE);
            BindDepth(-0.1f);
            DrawMesh(g_sprite_editor.raster_mesh, Translate(doc->position));
        }

        // Draw shape controls on top
        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindDepth(0.0f);
        DrawMesh(g_sprite_editor.mesh, Translate(doc->position));
    }

    static void UpdateSpriteEditor() {
        SpriteDocument* sdoc = GetSpriteDocument();

//        CheckShortcuts(g_mesh_editor.animation_shortcuts, g_mesh_editor.input);
        CheckShortcuts(g_sprite_editor_shortcuts, g_sprite_editor.input);

        if (g_sprite_editor.zoom_version != g_workspace.zoom_version) {
            g_sprite_editor.zoom_version = g_workspace.zoom_version;
            UpdateSpriteEditorMesh(sdoc, false);
        }

        if (g_sprite_editor.raster_version < 0) {
            g_sprite_editor.raster_version = 0;
            UpdateRaster(sdoc);
        }
    }

    static void SpriteEditorOverlay() {
        SpriteEditorToolbar();
    }

    static void DeleteSelected() {
        SpriteDocument* sdoc = GetSpriteDocument();
        SpriteFrame* f = GetFrame(sdoc, g_sprite_editor.current_frame);
        RecordUndo();
        DeleteSelectedAnchors(&f->shape);
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    static void SpriteEditorHelp() {
        HelpGroup("Sprite", g_sprite_editor_shortcuts);
//        HelpGroup("Animation", g_animation_shortcuts);
    }

    static void SpriteUndoRedo(Document* doc) {
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);
        for (u16 fi = 0; fi < sdoc->frame_count; ++fi)
            shape::UpdateSamples(&sdoc->frames[fi].shape);
        g_sprite_editor.zoom_version = -1;
        g_sprite_editor.raster_version = -1;
    }

    void InitSpriteEditor(SpriteDocument* sdoc) {
        sdoc->vtable.undo_redo = SpriteUndoRedo;
        sdoc->vtable.editor_begin = BeginSpriteEditor;
        sdoc->vtable.editor_end = EndSpriteEditor;
        sdoc->vtable.editor_update = UpdateSpriteEditor;
        sdoc->vtable.editor_draw = DrawSpriteEditor;
        sdoc->vtable.editor_overlay = SpriteEditorOverlay;
        sdoc->vtable.editor_help = SpriteEditorHelp;
    }

    void InitSpriteEditor() {
        InputSet* input = g_sprite_editor.input = CreateInputSet(ALLOCATOR_DEFAULT);
        EnableModifiers(input);
        EnableButton(input, MOUSE_LEFT);
        EnableButton(input, MOUSE_LEFT_DOUBLE_CLICK);
        EnableButton(input, KEY_Q);
        EnableButton(input, KEY_E);
        EnableButton(input, KEY_O);
        EnableButton(input, KEY_SPACE);
        EnableButton(input, KEY_H);
        EnableShortcuts(g_sprite_editor_shortcuts, g_sprite_editor.input);
        EnableCommonShortcuts(g_sprite_editor.input);

        g_sprite_editor.texture = CreateTexture(
            ALLOCATOR_DEFAULT,
            g_editor.atlas.size,
            g_editor.atlas.size,
            TEXTURE_FORMAT_RGBA8,
            GetName("sprite_editor"),
            TEXTURE_FILTER_NEAREST);
        g_sprite_editor.pixels = CreatePixelData(
            ALLOCATOR_DEFAULT,
            Vec2Int{g_editor.atlas.size, g_editor.atlas.size});
        g_sprite_editor.raster_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_EDITOR_TEXTURE);
        SetTexture(g_sprite_editor.raster_material, g_sprite_editor.texture, 0);
    }    

    void ShutdownSpriteEditor() {
        Free(g_sprite_editor.texture);
        Free(g_sprite_editor.pixels);
        Free(g_sprite_editor.raster_mesh);
        Free(g_sprite_editor.raster_material);
        Free(g_sprite_editor.mesh);
        g_sprite_editor.texture = nullptr;
        g_sprite_editor.pixels = nullptr;
        g_sprite_editor.raster_mesh = nullptr;
        g_sprite_editor.raster_material = nullptr;
        g_sprite_editor.mesh = nullptr;
    }
}