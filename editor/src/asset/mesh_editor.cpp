//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

extern Font* FONT_SEGUISB;
extern Mesh* MESH_COLOR_PICKER_COLOR;
extern Mesh* MESH_COLOR_PICKER_PALETTE;

constexpr float COLOR_PICKER_BORDER_WIDTH = 4.0f;
constexpr float COLOR_PICKER_COLOR_SIZE = 26.0f;
constexpr float COLOR_PICKER_WIDTH = COLOR_PICKER_COLOR_SIZE * 64 + COLOR_PICKER_BORDER_WIDTH * 2;
constexpr float COLOR_PICKER_HEIGHT = COLOR_PICKER_COLOR_SIZE + COLOR_PICKER_BORDER_WIDTH * 2;
constexpr float COLOR_PICKER_SELECTION_BORDER_WIDTH = 3.0f;
constexpr Color COLOR_PICKER_SELECTION_BORDER_COLOR = COLOR_VERTEX_SELECTED;

constexpr int MESH_EDITOR_ID_TOOLBAR = OVERLAY_BASE_ID + 0;
constexpr int MESH_EDITOR_ID_EXPAND = OVERLAY_BASE_ID + 1;
constexpr int MESH_EDITOR_ID_TILE = OVERLAY_BASE_ID + 2;
constexpr int MESH_EDITOR_ID_PALETTES = OVERLAY_BASE_ID + 3;
constexpr int MESH_EDITOR_ID_COLORS = MESH_EDITOR_ID_PALETTES + MAX_PALETTES;

enum MeshEditorMode {
    MESH_EDITOR_MODE_CURRENT=-1,
    MESH_EDITOR_MODE_VERTEX,
    MESH_EDITOR_MODE_EDGE,
    MESH_EDITOR_MODE_FACE,
    MESH_EDITOR_MODE_WEIGHT
};

struct MeshEditor {
    MeshEditorMode mode;
    Vec2 selection_drag_start;
    Vec2 selection_center;
    Material* color_material;
    bool clear_selection_on_up;
    bool clear_weight_bone_on_up;
    Vec2 state_mouse;
    bool use_fixed_value;
    bool ignore_up;
    bool use_negative_fixed_value;
    float fixed_value;
    Shortcut* shortcuts;
    VertexData saved[MAX_VERTICES];
    InputSet* input;
    Mesh* color_picker_mesh;
    MeshData* mesh_data;
    int weight_bone;
    bool xray;
    bool show_palette_picker;
    bool show_tiling;
    Mesh* editor_mesh;
};

static MeshEditor g_mesh_editor = {};

extern int SplitFaces(MeshData* m, int v0, int v1);
static void HandleBoxSelect(const Bounds2& bounds);

inline MeshData* GetMeshData() {
    AssetData* a = g_mesh_editor.mesh_data;
    assert(a->type == ASSET_TYPE_MESH);
    return (MeshData*)a;
}


static void UpdateVertexSelection(MeshData* m) {
    MeshDataImpl* impl = m->impl;
    impl->selected_vertex_count = 0;
    for (int vertex_index=0; vertex_index<impl->vertex_count; vertex_index++) {
        const VertexData& v = impl->vertices[vertex_index];
        if (!v.selected) continue;
        impl->selected_vertex_count++;
    }

    impl->selected_edge_count = 0;
    for (int edge_index=0; edge_index<impl->edge_count; edge_index++) {
        EdgeData& e = impl->edges[edge_index];
        VertexData& v0 = impl->vertices[e.v0];
        VertexData& v1 = impl->vertices[e.v1];
        e.selected = v0.selected && v1.selected;
        if (e.selected)
            impl->selected_edge_count++;
    }

    impl->selected_face_count = 0;
    for (int face_index=0; face_index<impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        f.selected = true;
        for (int face_vertex_index=0; f.selected && face_vertex_index<f.vertex_count; face_vertex_index++)
            f.selected &= impl->vertices[f.vertices[face_vertex_index]].selected;

        if (f.selected)
            impl->selected_face_count++;
    }
}

static void UpdateEdgeSelection(MeshData* m) {
    MeshDataImpl* impl = m->impl;
    impl->selected_vertex_count = 0;
    for (int vertex_index=0; vertex_index<impl->vertex_count; vertex_index++) {
        VertexData& v = impl->vertices[vertex_index];
        v.selected = false;
    }

    impl->selected_face_count = 0;
    for (int face_index=0; face_index<impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        f.selected = false;
    }

    impl->selected_edge_count = 0;
    for (int edge_index=0; edge_index<impl->edge_count; edge_index++) {
        EdgeData& e = impl->edges[edge_index];
        if (!e.selected) continue;
        impl->selected_edge_count++;

        VertexData& v0 = impl->vertices[e.v0];
        VertexData& v1 = impl->vertices[e.v1];

        if (!v0.selected) {
            v0.selected = true;
            impl->selected_vertex_count++;
        }

        if (!v1.selected) {
            v1.selected = true;
            impl->selected_vertex_count++;
        }
    }

    for (int face_index=0; face_index<impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];

        int selected_edge_count = 0;
        for (int edge_index=0; selected_edge_count < f.vertex_count - 1 && edge_index < impl->edge_count; edge_index++) {
            EdgeData& e = impl->edges[edge_index];
            if (!e.selected) continue;
            bool selected =
                e.face_count > 0 && e.face_index[0] == face_index ||
                e.face_count > 1 && e.face_index[1] == face_index;
            if (!selected) continue;
            selected_edge_count++;
        }

        if (selected_edge_count == f.vertex_count - 1) {
            f.selected = true;
            impl->selected_face_count++;
        }
    }
}

static void UpdateFaceSelection(MeshData* m) {
    MeshDataImpl* impl = m->impl;
    impl->selected_vertex_count = 0;
    for (int vertex_index=0; vertex_index<impl->vertex_count; vertex_index++) {
        VertexData& v = impl->vertices[vertex_index];
        v.selected = false;
    }

    impl->selected_edge_count = 0;
    for (int edge_index=0; edge_index<impl->edge_count; edge_index++) {
        EdgeData& e = impl->edges[edge_index];
        e.selected = false;
    }

    impl->selected_face_count = 0;
    for (int face_index=0; face_index<impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        if (!f.selected)
            continue;

        impl->selected_face_count++;
        for (int face_vertex_index=0; face_vertex_index<f.vertex_count; face_vertex_index++) {
            int vertex_index = f.vertices[face_vertex_index];
            VertexData& v = impl->vertices[vertex_index];
            if (v.selected) continue;

            v.selected = true;
            impl->selected_vertex_count++;
        }
    }

    for (int edge_index=0; edge_index<impl->edge_count; edge_index++) {
        EdgeData& e = impl->edges[edge_index];
        if (e.selected) continue;
        bool selected =
            e.face_count > 0 && impl->faces[e.face_index[0]].selected ||
            e.face_count > 1 && impl->faces[e.face_index[1]].selected;
        if (!selected)
            continue;
        e.selected = selected;
        impl->selected_edge_count++;
    }

}

static void UpdateSelectionCenter() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    Bounds2 bounds = {VEC2_ZERO, VEC2_ZERO};
    int vertex_index = 0;
    for (; vertex_index<impl->vertex_count; vertex_index++) {
        VertexData& v = impl->vertices[vertex_index];
        if (!v.selected) continue;
        bounds = {v.position, v.position};
        break;
    }

    for (;vertex_index<impl->vertex_count; vertex_index++) {
        VertexData& v = impl->vertices[vertex_index];
        if (!v.selected) continue;
        bounds = Union(bounds, {v.position, v.position});
    }

    if (impl->selected_vertex_count > 0)
        g_mesh_editor.selection_center = GetCenter(bounds);
    else
        g_mesh_editor.selection_center = VEC2_ZERO;
}

static void UpdateSelection(MeshEditorMode mode=MESH_EDITOR_MODE_CURRENT) {
    MeshData* m = GetMeshData();

    if (mode == MESH_EDITOR_MODE_CURRENT)
        mode = g_mesh_editor.mode;

    if (mode == MESH_EDITOR_MODE_FACE) {
        UpdateFaceSelection(m);
    } else if (mode == MESH_EDITOR_MODE_EDGE) {
        UpdateEdgeSelection(m);
    } else {
        UpdateVertexSelection(m);
    }

    UpdateSelectionCenter();
}

void RefreshMeshEditorSelection() {
    UpdateSelection();
}

static void ClearSelection() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    for (int vertex_index=0; vertex_index<impl->vertex_count; vertex_index++)
        impl->vertices[vertex_index].selected = false;

    for (int edge_index=0; edge_index<impl->edge_count; edge_index++)
        impl->edges[edge_index].selected = false;

    for (int face_index=0; face_index<impl->face_count; face_index++)
        impl->faces[face_index].selected = false;

    UpdateSelection();
}

static void SelectAll(MeshData* m) {
    MeshDataImpl* impl = m->impl;

    if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) {
        for (int face_index=0; face_index<impl->face_count; face_index++)
            impl->faces[face_index].selected = true;
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) {
        for (int edge_index=0; edge_index<impl->edge_count; edge_index++)
            impl->edges[edge_index].selected = true;
    } else {
        for (int vertex_index=0; vertex_index<impl->vertex_count; vertex_index++)
            impl->vertices[vertex_index].selected = true;
    }

    UpdateSelection();
}

static void SelectVertex(int vertex_index, bool selected) {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    assert(vertex_index >= 0 && vertex_index < impl->vertex_count);
    impl->vertices[vertex_index].selected = selected;
    UpdateSelection(MESH_EDITOR_MODE_VERTEX);
}

static void SelectEdge(int edge_index, bool selected) {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    assert(edge_index >= 0 && edge_index < impl->edge_count);

    EdgeData& e = impl->edges[edge_index];
    e.selected = selected;
    UpdateSelection(MESH_EDITOR_MODE_EDGE);
}

static void SelectFace(int face_index, bool selected) {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    assert(face_index >= 0 && face_index < impl->face_count);
    FaceData& f = impl->faces[face_index];
    f.selected = selected;
    UpdateSelection(MESH_EDITOR_MODE_FACE);
}

static void SaveMeshState() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    for (int i=0; i<impl->vertex_count; i++)
        g_mesh_editor.saved[i] = impl->vertices[i];
}

static void RevertMeshState() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    for (int i=0; i < impl->vertex_count; i++)
        impl->vertices[i] = g_mesh_editor.saved[i];

    MarkDirty(m);
    MarkModified(m);
    UpdateSelection();
}

static bool TrySelectVertex() {
    assert(g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX || g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT);

    MeshData* m = GetMeshData();
    int vertex_index = HitTestVertex(m, g_view.mouse_world_position);
    if (vertex_index == -1)
        return false;

    bool shift = IsShiftDown();
    if (!shift)
        ClearSelection();

    MeshDataImpl* impl = m->impl;
    VertexData& v = impl->vertices[vertex_index];
    if (!shift || !v.selected)
        SelectVertex(vertex_index, true);
    else
        SelectVertex(vertex_index, false);

    return true;
}

static bool TrySelectEdge() {
    assert(g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE);

    MeshData* m = GetMeshData();
    int edge_index = HitTestEdge(m, g_view.mouse_world_position);
    if (edge_index == -1)
        return false;

    bool shift = IsShiftDown();
    if (!shift)
        ClearSelection();

    MeshDataImpl* impl = m->impl;
    EdgeData& e = impl->edges[edge_index];
    if (!shift || !e.selected)
        SelectEdge(edge_index, true);
    else
        SelectEdge(edge_index, false);

    return true;
}

static bool TrySelectFace() {
    assert(g_mesh_editor.mode == MESH_EDITOR_MODE_FACE);

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    int hit_faces[MAX_FACES];
    int hit_count = HitTestFaces(
        m,
        Translate(m->position),
        g_view.mouse_world_position,
        hit_faces,
        MAX_FACES);

    if (hit_count == 0)
        return false;

    bool shift = IsShiftDown();
    int hit_index = 0;
    if (shift) {
        for (;hit_index<hit_count; hit_index++)
            if (impl->faces[hit_faces[hit_index]].selected)
                break;

        if (hit_index == hit_count)
            hit_index = 0;
    } else {
        for (hit_index=hit_count-1;hit_index>=0; hit_index--)
            if (impl->faces[hit_faces[hit_index]].selected)
                break;

        if (hit_index < 1)
            hit_index = hit_count - 1;
        else
            hit_index--;
    }

    int face_index = hit_faces[hit_index];
    if (!shift)
        ClearSelection();

    if (!shift || !impl->faces[face_index].selected)
        SelectFace(face_index, true);
    else
        SelectFace(face_index, false);

    return true;
}

static bool TrySelectBone() {
    assert(g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT);

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->skeleton == nullptr)
        return false;

    int hit[MAX_BONES];
    int hit_count = HitTestBones(impl->skeleton, Translate(GetMeshData()->position), g_view.mouse_world_position, hit, MAX_BONES);
    if (hit_count == 0) {
        g_mesh_editor.weight_bone = -1;
        return false;
    }

    if (g_mesh_editor.weight_bone != -1) {
        for (int hit_index=0; hit_index<hit_count; hit_index++) {
            int bone_index = hit[hit_index];
            if (bone_index >= g_mesh_editor.weight_bone)
                continue;

            g_mesh_editor.weight_bone = bone_index;
            return true;
        }
    }

    g_mesh_editor.weight_bone = hit[0];
    return true;
}

static void InsertVertex() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_VERTEX)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);

    if (impl->selected_vertex_count >= 3) {
        int face_index = CreateFace(m);
        if (face_index == -1)
            CancelUndo();

        return;
    }

    int vertex_index = HitTestVertex(m, g_view.mouse_world_position, 0.1f);
    if (vertex_index != -1)
        return;

    float edge_pos;
    int edge_index = HitTestEdge(m, g_view.mouse_world_position, &edge_pos);
    if (edge_index < 0)
        return;


    int new_vertex_index = SplitEdge(m, edge_index, edge_pos);
    if (new_vertex_index == -1)
        return;

    ClearSelection();
    SelectVertex(new_vertex_index, true);
}

static void DissolveSelected() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT)
        return;
    if (impl->selected_vertex_count == 0)
        return;

    RecordUndo(m);

    // Track selected vertices before dissolving (for orphan cleanup)
    bool was_selected[MAX_VERTICES] = {};
    for (int i=0; i<impl->vertex_count; i++)
        was_selected[i] = impl->vertices[i].selected;

    if (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX) {
        DissolveSelectedVertices(m);
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) {
        for (int i=impl->edge_count-1; i>=0; i--)
            if (impl->edges[i].selected)
                DissolveEdge(m, i);
        ClearSelection();
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) {
        DissolveSelectedFaces(m);
    }

    // Remove vertices that were selected and are now orphaned
    for (int i=impl->vertex_count-1; i>=0; i--) {
        if (was_selected[i] && impl->vertices[i].ref_count == 0)
            DeleteVertex(m, i);
    }

    MarkDirty(m);
    MarkModified(m);
    UpdateSelection();
}

static bool TryDoubleClickSelectFaceVertices() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_VERTEX && g_mesh_editor.mode != MESH_EDITOR_MODE_WEIGHT)
        return false;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    int vertex_index = HitTestVertex(m, g_view.mouse_world_position);
    if (vertex_index == -1)
        return false;

    // Find all faces containing this vertex
    bool shift = IsShiftDown();
    bool all_selected = true;

    for (int face_index = 0; face_index < impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        bool contains_vertex = false;
        for (int i = 0; i < f.vertex_count; i++) {
            if (f.vertices[i] == vertex_index) {
                contains_vertex = true;
                break;
            }
        }
        if (!contains_vertex)
            continue;

        // Check if all vertices in this face are selected (excluding clicked vertex,
        // since single-click may have toggled it before double-click fired)
        for (int i = 0; i < f.vertex_count; i++) {
            if (f.vertices[i] == vertex_index)
                continue;
            if (!impl->vertices[f.vertices[i]].selected) {
                all_selected = false;
                break;
            }
        }
    }

    // If shift and all selected, deselect. Otherwise select.
    bool should_select = !(shift && all_selected);

    if (!shift)
        ClearSelection();

    for (int face_index = 0; face_index < impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        bool contains_vertex = false;
        for (int i = 0; i < f.vertex_count; i++) {
            if (f.vertices[i] == vertex_index) {
                contains_vertex = true;
                break;
            }
        }
        if (!contains_vertex)
            continue;

        for (int i = 0; i < f.vertex_count; i++)
            impl->vertices[f.vertices[i]].selected = should_select;
    }

    UpdateSelection();
    return true;
}

static void UpdateDefaultState() {
    // In edge mode, drag on edge starts curve tool
    if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE && !IsToolActive() && g_view.drag_started) {
        int edge = HitTestEdge(GetMeshData(), g_view.drag_world_position);
        if (edge != -1) {
            BeginCurveTool(GetMeshData(), edge);
            return;
        }
    }

    if (!IsToolActive() && g_view.drag_started) {
        BeginBoxSelect(HandleBoxSelect);
        return;
    }

    // Double-click to select all vertices of face
    if (WasButtonPressed(g_mesh_editor.input, MOUSE_LEFT_DOUBLE_CLICK)) {
        if (TryDoubleClickSelectFaceVertices()) {
            g_mesh_editor.ignore_up = true;
            return;
        }
    }

    // Select
    if (!g_mesh_editor.ignore_up && !g_view.drag && WasButtonReleased(g_mesh_editor.input, MOUSE_LEFT)) {
        g_mesh_editor.clear_selection_on_up = false;
        g_mesh_editor.clear_weight_bone_on_up = false;

        if (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX && TrySelectVertex()) return;
        if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE && TrySelectEdge()) return;
        if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE && TrySelectFace()) return;
        if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT) {
            if (IsCtrlDown() && TrySelectBone()) return;
            if (!IsCtrlDown() && TrySelectVertex()) return;
        }

        g_mesh_editor.clear_selection_on_up = !IsCtrlDown();
        g_mesh_editor.clear_weight_bone_on_up = IsCtrlDown();
    }

    g_mesh_editor.ignore_up &= !WasButtonReleased(g_mesh_editor.input, MOUSE_LEFT);

    if (WasButtonReleased(g_mesh_editor.input, MOUSE_LEFT)) {
        if (g_mesh_editor.clear_selection_on_up)
            ClearSelection();

        if (g_mesh_editor.clear_weight_bone_on_up)
             g_mesh_editor.weight_bone = -1;
    }
}

constexpr int   STAT_FONT_SIZE = 14;
constexpr float STAT_SPACING = 4.0f;
constexpr float STAT_HEIGHT = 18.0f;
constexpr float STAT_LABEL_WIDTH = 100.0f;
constexpr float STAT_VALUE_WIDTH = 60.0f;

static void AddStat(const char* name, int value) {
    BeginContainer({.height=STAT_HEIGHT});
    BeginRow();
    {
        // label
        BeginContainer({.width=STAT_LABEL_WIDTH});
        Label(name, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_TEXT_COLOR()});
        EndContainer();

        // value
        BeginContainer({.width=STAT_VALUE_WIDTH});
        Text text;
        Format(text, "%d", value);
        Label(text, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_ACCENT_TEXT_COLOR()});
        EndContainer();
    }
    EndRow();
    EndContainer();
}

static void MeshStats() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    BeginOverlay(ELEMENT_ID_NONE, ALIGN_TOP_RIGHT);
    BeginColumn({.spacing=STAT_SPACING});
    {
        AddStat("Vertices", impl->vertex_count);
        AddStat("Edges", impl->edge_count);
        AddStat("Faces", impl->face_count);
        if (impl->mesh)
            AddStat("Triangles", GetIndexCount(impl->mesh) / 3);
    }
    EndColumn();
    EndOverlay();
}

static bool Palette(int palette_index, bool* selected_colors) {
    int col_count = 64;
    float max_color_width = GetUISize().x * 0.9f;
    while (col_count * COLOR_PICKER_COLOR_SIZE > max_color_width)
        col_count/=2;

    Label(g_editor.palettes[palette_index].name, {
        .font=FONT_SEGUISB,
        .font_size=STYLE_OVERLAY_TEXT_SIZE,
        .color=STYLE_OVERLAY_TEXT_COLOR(),
        .align=ALIGN_LEFT});

    Spacer(2.0f);

    BeginContainer({.id = static_cast<ElementId>(MESH_EDITOR_ID_PALETTES + palette_index)});
    BeginGrid({.columns=col_count, .cell={COLOR_PICKER_COLOR_SIZE, COLOR_PICKER_COLOR_SIZE}});
    for (int i=0; i<COLOR_COUNT; i++) {
        BeginContainer({
            .width=COLOR_PICKER_COLOR_SIZE,
            .height=COLOR_PICKER_COLOR_SIZE,
            .border={
                .width=(selected_colors && selected_colors[i])?2.0f:0.0f,
                .color=COLOR_VERTEX_SELECTED
            },
            .id=static_cast<ElementId>(MESH_EDITOR_ID_COLORS + i)
        });
        Image(g_view.edge_mesh, {
            .stretch = IMAGE_STRETCH_UNIFORM,
            .color_offset=Vec2Int{i,g_editor.palettes[palette_index].id}
        });
        if (!g_mesh_editor.show_palette_picker && WasPressed()) {
            RecordUndo(GetMeshData());
            SetSelecteFaceColor(GetMeshData(), i);
            MarkModified(GetMeshData());
        }
        EndContainer();
    }
    EndGrid();

    bool pressed = false;
    if (g_mesh_editor.show_palette_picker) {
#if 0
        BeginContainer({
            .width=100,
            .height=COLOR_PICKER_HEIGHT - COLOR_PICKER_BORDER_WIDTH * 2,
            .margin=EdgeInsetsLeft(-106),
            .padding=EdgeInsetsRight(COLOR_PICKER_BORDER_WIDTH),
            .color=STYLE_BACKGROUND_COLOR()});
        Label(g_editor.palettes[palette_index].name, {
            .font=FONT_SEGUISB,
            .font_size=STYLE_TEXT_FONT_SIZE,
            .color=STYLE_TEXT_COLOR,
            .align=ALIGN_CENTER_RIGHT});
        EndContainer();
#endif

        pressed |= WasPressed();
    }

    EndContainer();

    return pressed;
}

static void ColorPicker(){
    static bool selected_colors[COLOR_COUNT] = {};
    memset(selected_colors, 0, sizeof(selected_colors));
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    for (int face_index=0; face_index<impl->face_count; face_index++) {
        FaceData* f = &impl->faces[face_index];
        if (!f->selected) continue;
        selected_colors[f->color] = true;
    }

    bool show_palette_picker = g_mesh_editor.show_palette_picker;
    int current_palette_index = g_editor.palette_map[impl->palette];

    BeginContainer({.padding=EdgeInsetsAll(4), .color=STYLE_OVERLAY_CONTENT_COLOR()});
    BeginColumn();
    {
        // Show all palettes when palette picker is active
        if (g_mesh_editor.show_palette_picker) {
            for (int palette_index=0; palette_index<g_editor.palette_count; palette_index++) {
                if (palette_index == current_palette_index) continue;
                if (Palette(palette_index, nullptr)) {
                    RecordUndo(m);
                    impl->palette = g_editor.palettes[palette_index].id;
                    MarkDirty(m);
                    MarkModified(m);
                    show_palette_picker = false;
                }

                Spacer(2.0f);
            }
        }

        // Colors
        if (Palette(current_palette_index, selected_colors))
            show_palette_picker = false;
    }
    EndColumn();
    EndContainer();

    g_mesh_editor.show_palette_picker = show_palette_picker;
}

static void MeshEditorToolbar() {
    bool show_palette_picker = g_mesh_editor.show_palette_picker;

    BeginOverlay(MESH_EDITOR_ID_TOOLBAR, ALIGN_BOTTOM_CENTER);
    BeginColumn({.spacing=8});

    // Expander
    BeginContainer({
        .height=16,
        .align=ALIGN_TOP_CENTER,
        .padding=EdgeInsetsAll(5),
        .id=MESH_EDITOR_ID_EXPAND});
    {
        if (WasPressed())
            show_palette_picker = !g_mesh_editor.show_palette_picker;

        Image(g_mesh_editor.show_palette_picker ? MESH_ICON_EXPAND_DOWN : MESH_ICON_EXPAND_UP, {
            .align=ALIGN_TOP_CENTER,
            .color=STYLE_OVERLAY_ICON_COLOR(),
        });
    }
    EndContainer();

    // Buttons
    BeginContainer();
    BeginRow({.align=ALIGN_RIGHT});
    if (EditorToggleButton(MESH_EDITOR_ID_TILE, MESH_ICON_TILING, g_mesh_editor.show_tiling))
        g_mesh_editor.show_tiling = !g_mesh_editor.show_tiling;
    EndRow();
    EndContainer();

    ColorPicker();

    EndColumn();
    EndOverlay();

    g_mesh_editor.show_palette_picker = show_palette_picker;
}

static void MeshEditorOverlay() {
    MeshStats();
    MeshEditorToolbar();
}

static Bounds2 GetMeshEditorBounds() {
    MeshData* em = GetMeshData();
    MeshDataImpl* impl = em->impl;
    Bounds2 bounds = BOUNDS2_ZERO;
    bool first = true;
    for (int i = 0; i < impl->vertex_count; i++)
    {
        const VertexData& ev = impl->vertices[i];
        if (!ev.selected)
            continue;

        if (first)
            bounds = {ev.position, ev.position};
        else
            bounds = Union(bounds, ev.position);

        first = false;
    }

    if (first)
        return GetBounds(GetMeshData());

    return bounds;
}

static void HandleBoxSelect(const Bounds2& bounds) {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    bool shift = IsShiftDown();
    if (!shift)
        ClearSelection();

    switch (g_mesh_editor.mode) {
    case MESH_EDITOR_MODE_VERTEX:
    case MESH_EDITOR_MODE_WEIGHT:
        for (int i=0; i<impl->vertex_count; i++) {
            VertexData& v = impl->vertices[i];
            Vec2 vpos = v.position + m->position;

            if (vpos.x >= bounds.min.x && vpos.x <= bounds.max.x &&
                vpos.y >= bounds.min.y && vpos.y <= bounds.max.y) {
                v.selected = true;
            }
        }
        break;

    case MESH_EDITOR_MODE_EDGE:
        for (int edge_index=0; edge_index<impl->edge_count; edge_index++) {
            EdgeData& e = impl->edges[edge_index];
            Vec2 ev0 = impl->vertices[e.v0].position + m->position;
            Vec2 ev1 = impl->vertices[e.v1].position + m->position;
            if (Intersects(bounds, ev0, ev1)) {
                e.selected = true;
            }
        }
        break;

    case MESH_EDITOR_MODE_FACE:
        for (int face_index=0; face_index<impl->face_count; face_index++) {
            FaceData& f = impl->faces[face_index];
            for (int vertex_index=0; vertex_index<f.vertex_count; vertex_index++) {
                int v0 = f.vertices[vertex_index];
                int v1 = f.vertices[(vertex_index + 1) % f.vertex_count];
                Vec2 v0p = impl->vertices[v0].position + m->position;
                Vec2 v1p = impl->vertices[v1].position + m->position;
                if (Intersects(bounds, v0p, v1p)) {
                    SelectFace(face_index, true);
                    f.selected = true;
                }
            }
        }
        break;

    default:
        break;
    }

    UpdateSelection();
}

static void CancelMeshTool() {
    CancelUndo();
    RevertMeshState();
}

static void CommitMoveTool(const Vec2& delta) {
    (void)delta;
    UpdateSelectionCenter();
}

static void UpdateMoveTool(const Vec2& delta) {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    bool snap = IsCtrlDown(GetInputSet());
    for (int i=0; i<impl->vertex_count; i++) {
        VertexData& v = impl->vertices[i];
        VertexData& s = g_mesh_editor.saved[i];
        if (v.selected)
            v.position = snap ? SnapToGrid(m->position + s.position + delta) - m->position : s.position + delta;
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void BeginMoveTool() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->selected_vertex_count == 0)
        return;

    SaveMeshState();
    RecordUndo(m);
    BeginMoveTool({.update=UpdateMoveTool, .commit=CommitMoveTool, .cancel=CancelMeshTool});
}

static void UpdateRotateTool(float angle) {
    if (IsCtrlDown()) {
        int angle_step = (int)(angle / 15.0f);
        angle = angle_step * 15.0f;
    }

    float cos_angle = Cos(Radians(angle));
    float sin_angle = Sin(Radians(angle));

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    for (i32 i=0; i<impl->vertex_count; i++) {
        VertexData& ev = impl->vertices[i];
        if (!ev.selected)
            continue;

        VertexData& s = g_mesh_editor.saved[i];
        Vec2 relative_pos = s.position - g_mesh_editor.selection_center;

        Vec2 rotated_pos;
        rotated_pos.x = relative_pos.x * cos_angle - relative_pos.y * sin_angle;
        rotated_pos.y = relative_pos.x * sin_angle + relative_pos.y * cos_angle;
        ev.position = g_mesh_editor.selection_center + rotated_pos;
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void BeginRotateTool() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->selected_vertex_count == 0 || (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX && impl->selected_vertex_count == 1))
        return;

    SaveMeshState();
    RecordUndo(m);
    BeginRotateTool({.origin=g_mesh_editor.selection_center+m->position, .update=UpdateRotateTool, .cancel=CancelMeshTool});
}

static void UpdateScaleTool(const Vec2& scale) {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    Vec2 center = g_mesh_editor.selection_center;
    if (IsCtrlDown())
        center = HitTestSnap(m, g_view.drag_world_position - m->position);

    SetScaleToolOrigin(center+m->position);

    for (i32 i=0; i<impl->vertex_count; i++) {
        VertexData& v = impl->vertices[i];
        if (!v.selected)
            continue;

        Vec2 dir = g_mesh_editor.saved[i].position - center;
        v.position = center + dir * scale;
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void BeginScaleTool() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->selected_vertex_count == 0)
        return;

    SaveMeshState();
    RecordUndo(m);
    BeginScaleTool({.origin=g_mesh_editor.selection_center+m->position, .update=UpdateScaleTool, .cancel=CancelMeshTool});
}

static void UpdateWeightToolVertex(float weight, void* user_data) {
    int vertex_index = (int)(i64)user_data;
    SetVertexWeight(GetMeshData(), vertex_index, g_mesh_editor.weight_bone, weight);
}

static void UpdateWeightTool() {
    MeshData* m = GetMeshData();
    UpdateEdges(m);
    MarkDirty(m);
    MarkModified();
}

static void BeginWeightTool() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_WEIGHT)
        return;

    if (g_mesh_editor.weight_bone == -1)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->selected_vertex_count == 0)
        return;

    WeightToolOptions options = {
        .vertex_count = 0,
        .min_weight = 0,
        .max_weight = 1,
        .update = UpdateWeightTool,
        .cancel = CancelMeshTool,
        .update_vertex = UpdateWeightToolVertex,
    };

    for (int i=0; i<impl->vertex_count; i++) {
        VertexData& v = impl->vertices[i];
        if (!v.selected)
            continue;

        options.vertices[options.vertex_count++] = {
            .position = v.position + GetMeshData()->position,
            .weight = GetVertexWeight(m, i, g_mesh_editor.weight_bone),
            .user_data = (void*)(i64)i,
        };
    }

    if (!options.vertex_count)
        return;

    SaveMeshState();
    RecordUndo(m);
    BeginWeightTool(options);
}

static void SelectAll() {
    SelectAll(GetMeshData());
}

static bool MeshViewAllowTextInput() {
    return false;
}

static void SetVertexMode() {
    g_mesh_editor.mode = MESH_EDITOR_MODE_VERTEX;
}

static void SetEdgeMode() {
    g_mesh_editor.mode = MESH_EDITOR_MODE_EDGE;
}

static void SetFaceMode() {
    g_mesh_editor.mode = MESH_EDITOR_MODE_FACE;
}

static void SetWeightMode() {
    g_mesh_editor.mode = MESH_EDITOR_MODE_WEIGHT;
}

static void CenterMesh() {
    Center(GetMeshData());
}

static void CircleMesh() {
    if (GetMeshData()->impl->selected_vertex_count < 2)
        return;

    BeginSelectTool({.commit= [](const Vec2& position ) {
        MeshData* m = GetMeshData();
        MeshDataImpl* impl = m->impl;
        Vec2 center = position - m->position;
        float total_distance = 0.0f;
        int count = 0;
        for (int i=0; i<impl->vertex_count; i++) {
            VertexData& v = impl->vertices[i];
            if (!v.selected)
                continue;
            total_distance += Length(v.position - center);
            count++;
        }

        float radius = total_distance / (float)count;

        RecordUndo(m);

        // move selected vertices to form a circle around the center point
        for (int i=0; i<impl->vertex_count; i++) {
            VertexData& v = impl->vertices[i];
            if (!v.selected)
                continue;
            Vec2 dir = Normalize(v.position - center);
            v.position = center + dir * radius;
        }

        UpdateEdges(m);
        MarkDirty(m);
        MarkModified(m);
    }});
}

static bool ExtrudeSelectedEdges(MeshData* m) {
    MeshDataImpl* impl = m->impl;
    if (impl->edge_count == 0)
        return false;

    int selected_edges[MAX_EDGES];
    int selected_edge_count = 0;
    for (int i = 0; i < impl->edge_count; i++)
        if (impl->edges[i].selected && selected_edge_count < MAX_EDGES)
            selected_edges[selected_edge_count++] = i;

    if (selected_edge_count == 0)
        return false;

    // Find all unique vertices that are part of selected edges
    bool vertex_needs_extrusion[MAX_VERTICES] = {};
    for (int i = 0; i < selected_edge_count; i++) {
        const EdgeData& ee = impl->edges[selected_edges[i]];
        vertex_needs_extrusion[ee.v0] = true;
        vertex_needs_extrusion[ee.v1] = true;
    }

    // Create mapping from old vertex indices to new vertex indices
    int vertex_mapping[MAX_VERTICES];
    for (int i = 0; i < MAX_VERTICES; i++)
        vertex_mapping[i] = -1;

    // Create new vertices for each unique vertex that needs extrusion
    for (int i = 0; i < impl->vertex_count; i++) {
        if (!vertex_needs_extrusion[i])
            continue;

        if (impl->vertex_count >= MAX_VERTICES)
            return false;

        int new_vertex_index = impl->vertex_count++;
        vertex_mapping[i] = new_vertex_index;

        // Copy vertex properties and offset position along edge normal
        VertexData& old_vertex = impl->vertices[i];
        VertexData& new_vertex = impl->vertices[new_vertex_index];

        new_vertex = old_vertex;

        // Don't offset the position - the new vertex should start at the same position
        // The user will move it in move mode
        new_vertex.selected = false;
    }

    // Store vertex pairs for the new edges we want to select
    int new_edge_vertex_pairs[MAX_EDGES][2];
    int new_edge_count = 0;

    // Create new edges for the extruded geometry
    for (int i = 0; i < selected_edge_count; i++) {
        const EdgeData& original_edge = impl->edges[selected_edges[i]];
        int old_v0 = original_edge.v0;
        int old_v1 = original_edge.v1;
        int new_v0 = vertex_mapping[old_v0];
        int new_v1 = vertex_mapping[old_v1];

        if (new_v0 == -1 || new_v1 == -1)
            continue;

        // Create connecting edges between old and new vertices
        if (impl->edge_count + 3 >= MAX_EDGES)
            return false;

        if (impl->face_count + 2 >= MAX_FACES)
            return false;

        GetOrAddEdge(m, old_v0, new_v0, -1);
        GetOrAddEdge(m, old_v1, new_v1, -1);
        GetOrAddEdge(m, new_v0, new_v1, -1);

        // Store the vertex pair for the new edge we want to select
        if (new_edge_count < MAX_EDGES) {
            new_edge_vertex_pairs[new_edge_count][0] = new_v0;
            new_edge_vertex_pairs[new_edge_count][1] = new_v1;
            new_edge_count++;
        }

        // Find the face that contains this edge to inherit its color and determine orientation
        int face_color = 0; // Default color
        Vec3 face_normal = {0, 0, 1}; // Default normal
        bool edge_reversed = false; // Track if edge direction is reversed in the face
        bool found_face = false;

        for (int face_idx = 0; !found_face && face_idx < impl->face_count; face_idx++) {
            const FaceData& f = impl->faces[face_idx];

            // Check if this face contains the edge using the face_vertices array
            for (int vertex_index = 0; !found_face && vertex_index < f.vertex_count; vertex_index++) {
                int v0_idx = f.vertices[vertex_index];
                int v1_idx = f.vertices[(vertex_index + 1) % f.vertex_count];

                if (v0_idx == old_v0 && v1_idx == old_v1) {
                    face_color = f.color;
                    face_normal = f.normal;
                    edge_reversed = false;
                    found_face = true;
                } else if (v0_idx == old_v1 && v1_idx == old_v0) {
                    face_color = f.color;
                    face_normal = f.normal;
                    edge_reversed = true;
                    found_face = true;
                }
            }
        }

        FaceData& quad = impl->faces[impl->face_count++];
        quad.color = face_color;
        quad.normal = face_normal;
        quad.selected = false;
        quad.vertex_count = 4;

        if (!edge_reversed) {
            quad.vertices[0] = old_v0;
            quad.vertices[1] = new_v0;
            quad.vertices[2] = new_v1;
            quad.vertices[3] = old_v1;
        } else {
            quad.vertices[0] = old_v1;
            quad.vertices[1] = new_v1;
            quad.vertices[2] = new_v0;
            quad.vertices[3] = old_v0;
        }
    }

    UpdateEdges(m);
    MarkDirty(m);

    // update selection
    ClearSelection();
    for (int i = 0; i < new_edge_count; i++) {
        int v0 = new_edge_vertex_pairs[i][0];
        int v1 = new_edge_vertex_pairs[i][1];
        int edge_index = GetEdge(m, v0, v1);
        assert(edge_index != -1);
        impl->edges[edge_index].selected = true;
    }

    UpdateSelection(MESH_EDITOR_MODE_EDGE);

    return true;
}

static void ExtrudeSelected() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    if (g_mesh_editor.mode != MESH_EDITOR_MODE_EDGE || impl->selected_vertex_count <= 0)
        return;

    RecordUndo(m);
    if (!ExtrudeSelectedEdges(m)) {
        CancelUndo();
        return;
    }

    BeginMoveTool();
}

static void NewFace() {
    if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);

    int face_index = -1;
    if ((g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX || g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) && impl->selected_vertex_count >= 3) {
        face_index = CreateFace(m);
    } else {
        float edge_size = g_config->GetFloat("mesh", "default_edge_size", 1.0f);

        impl->vertex_count += 4;
        impl->vertices[impl->vertex_count - 4] = { .position = { -0.25f, -0.25f }, .edge_size = edge_size };
        impl->vertices[impl->vertex_count - 3] = { .position = {  0.25f, -0.25f }, .edge_size = edge_size };
        impl->vertices[impl->vertex_count - 2] = { .position = {  0.25f,  0.25f }, .edge_size = edge_size };
        impl->vertices[impl->vertex_count - 1] = { .position = { -0.25f,  0.25f }, .edge_size = edge_size };

        FaceData& f = impl->faces[impl->face_count++];
        f = {
            .color = 0,
            .normal = { 0, 0, 0 },
            .vertex_count = 4
        };
        f.vertices[0] = impl->vertex_count - 4;
        f.vertices[1] = impl->vertex_count - 3;
        f.vertices[2] = impl->vertex_count - 2;
        f.vertices[3] = impl->vertex_count - 1;

        face_index = impl->face_count-1;
    }

    if (face_index == -1) {
        CancelUndo();
        return;
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);

    ClearSelection();
    SelectFace(face_index, true);
}

static void UpdateMeshEditor() {
    CheckShortcuts(g_mesh_editor.shortcuts, g_mesh_editor.input);
    UpdateDefaultState();
}

static void DrawSkeleton() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    SkeletonData* s = impl->skeleton;
    if (!s)
        return;

    BindDepth(0.0f);
    BindMaterial(g_view.vertex_material);

    // Determine which bones are used by selected vertices
    bool bone_used[MAX_BONES] = {};
    for (int vertex_index=0; vertex_index<impl->vertex_count; vertex_index++) {
        VertexData& v = impl->vertices[vertex_index];
        if (!v.selected) continue;
        for (int weight_index=0; weight_index<MESH_MAX_VERTEX_WEIGHTS; weight_index++) {
            VertexWeight& w = v.weights[weight_index];
            if (w.bone_index != -1 && w.weight > F32_EPSILON)
                bone_used[w.bone_index] = true;
        }
    }

    Mat3 transform = Translate(m->position);
    BindColor(SetAlpha(STYLE_SKELETON_BONE_COLOR, 0.5f));
    for (int bone_index=0; bone_index<s->impl->bone_count; bone_index++)
        if (!bone_used[bone_index] && bone_index != g_mesh_editor.weight_bone)
            DrawBone(transform * s->impl->bones[bone_index].local_to_world, s->impl->bones[bone_index].length);

    BindColor(COLOR_WHITE);
    for (int bone_index=0; bone_index<s->impl->bone_count; bone_index++)
        if (bone_used[bone_index] && bone_index != g_mesh_editor.weight_bone)
            DrawBone(transform * s->impl->bones[bone_index].local_to_world, s->impl->bones[bone_index].length);

    BindColor(COLOR_VERTEX_SELECTED);
    if (g_mesh_editor.weight_bone != -1)
        DrawBone(transform * s->impl->bones[g_mesh_editor.weight_bone].local_to_world, s->impl->bones[g_mesh_editor.weight_bone].length);

}

static void DrawXRay() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_WEIGHT && g_mesh_editor.xray) {
        DrawSkeleton();
    }
}

static void BuildEditorMesh(MeshBuilder* builder, MeshData* m, bool hide_selected) {
    MeshDataImpl* impl = m->impl;
    float line_width = STYLE_MESH_EDGE_WIDTH * g_view.zoom_ref_scale;
    float vertex_size = STYLE_MESH_VERTEX_SIZE * g_view.zoom_ref_scale;
    float origin_size = 0.1f * g_view.zoom_ref_scale;

    for (int edge_index = 0; edge_index < impl->edge_count; edge_index++) {
        const EdgeData& e = impl->edges[edge_index];
        Vec2 v0 = impl->vertices[e.v0].position + m->position;
        Vec2 v1 = impl->vertices[e.v1].position + m->position;

        if (IsEdgeCurved(m, edge_index)) {
            // Draw curved edge as line segments
            Vec2 control = GetEdgeControlPoint(m, edge_index) + m->position;
            constexpr int segments = 8;
            Vec2 prev = v0;
            for (int s = 1; s <= segments; s++) {
                float t = (float)s / (float)segments;
                Vec2 curr = EvalQuadraticBezier(v0, control, v1, t);
                AddEditorLine(builder, prev, curr, line_width, COLOR_EDGE);
                prev = curr;
            }
        } else {
            AddEditorLine(builder, v0, v1, line_width, COLOR_EDGE);
        }
    }

    if (hide_selected) {
        for (int i = 0; i < impl->vertex_count; i++) {
            const VertexData& v = impl->vertices[i];
            if (v.selected) continue;
            AddEditorSquare(builder, v.position + m->position, vertex_size, COLOR_VERTEX);
        }
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX) {
        for (int i = 0; i < impl->vertex_count; i++) {
            const VertexData& v = impl->vertices[i];
            Color color = v.selected ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
            AddEditorSquare(builder, v.position + m->position, vertex_size, color);
        }
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) {
        for (int edge_index = 0; edge_index < impl->edge_count; edge_index++) {
            const EdgeData& e = impl->edges[edge_index];
            if (!e.selected) continue;
            Vec2 v0 = impl->vertices[e.v0].position + m->position;
            Vec2 v1 = impl->vertices[e.v1].position + m->position;

            if (IsEdgeCurved(m, edge_index)) {
                Vec2 control = GetEdgeControlPoint(m, edge_index) + m->position;
                constexpr int segments = 8;
                Vec2 prev = v0;
                for (int s = 1; s <= segments; s++) {
                    float t = (float)s / (float)segments;
                    Vec2 curr = EvalQuadraticBezier(v0, control, v1, t);
                    AddEditorLine(builder, prev, curr, line_width, COLOR_EDGE_SELECTED);
                    prev = curr;
                }
            } else {
                AddEditorLine(builder, v0, v1, line_width, COLOR_EDGE_SELECTED);
            }
        }
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) {
        for (int face_index = 0; face_index < impl->face_count; face_index++) {
            const FaceData& f = impl->faces[face_index];
            if (!f.selected) continue;
            for (int vi = 0; vi < f.vertex_count; vi++) {
                int v0_idx = f.vertices[vi];
                int v1_idx = f.vertices[(vi + 1) % f.vertex_count];
                Vec2 v0 = impl->vertices[v0_idx].position + m->position;
                Vec2 v1 = impl->vertices[v1_idx].position + m->position;

                int edge_index = GetEdge(m, v0_idx, v1_idx);
                if (edge_index != -1 && IsEdgeCurved(m, edge_index)) {
                    Vec2 control = GetEdgeControlPoint(m, edge_index) + m->position;
                    constexpr int segments = 8;
                    Vec2 prev = v0;
                    for (int s = 1; s <= segments; s++) {
                        float t = (float)s / (float)segments;
                        Vec2 curr = EvalQuadraticBezier(v0, control, v1, t);
                        AddEditorLine(builder, prev, curr, line_width, COLOR_VERTEX_SELECTED);
                        prev = curr;
                    }
                } else {
                    AddEditorLine(builder, v0, v1, line_width, COLOR_VERTEX_SELECTED);
                }
            }
        }
        for (int face_index = 0; face_index < impl->face_count; face_index++) {
            const FaceData& f = impl->faces[face_index];
            Vec2 center = GetFaceCenter(m, face_index) + m->position;
            Color color = f.selected ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
            AddEditorSquare(builder, center, vertex_size, color);
        }
    } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT) {
        float outline_size = STYLE_MESH_WEIGHT_OUTLINE_SIZE * g_view.zoom_ref_scale;
        float control_size = STYLE_MESH_WEIGHT_SIZE * g_view.zoom_ref_scale;
        float stroke_thickness = 0.02f * g_view.zoom_ref_scale;

        for (int vi = 0; vi < impl->vertex_count; vi++) {
            VertexData& v = impl->vertices[vi];
            Vec2 pos = v.position + m->position;
            float weight = GetVertexWeight(m, vi, g_mesh_editor.weight_bone);

            if (!v.selected) {
                AddEditorCircleStroke(builder, pos, outline_size, stroke_thickness, SetAlpha(COLOR_BLACK, 0.5f));
                if (weight > 0.0f) {
                    AddEditorArc(builder, pos, control_size, weight, SetAlpha(COLOR_BLACK, 0.5f));
                }
            }
        }

        for (int vi = 0; vi < impl->vertex_count; vi++) {
            VertexData& v = impl->vertices[vi];
            Vec2 pos = v.position + m->position;
            float weight = GetVertexWeight(m, vi, g_mesh_editor.weight_bone);

            if (v.selected) {
                AddEditorSquare(builder, pos, control_size, SetAlpha(COLOR_BLACK, 0.5f));
                if (weight > 0.0f) {
                    AddEditorArc(builder, pos, control_size, weight, COLOR_VERTEX_SELECTED);
                }
                AddEditorCircleStroke(builder, pos, outline_size, stroke_thickness, COLOR_VERTEX_SELECTED);
            }
        }

        for (int i = 0; i < impl->vertex_count; i++) {
            const VertexData& v = impl->vertices[i];
            Color color = v.selected ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
            AddEditorCircle(builder, v.position + m->position, vertex_size * 0.5f, color);
        }
    }

    AddEditorCircle(builder, m->position, origin_size, COLOR_ORIGIN);
}

static void DrawMeshEditor() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;

    BindColor(COLOR_WHITE, Vec2Int{0,impl->palette});
    DrawMesh(m, Translate(m->position));

    // Draw tiled copies if tiling preview is enabled
    if (g_mesh_editor.show_tiling) {
        Bounds2 bounds = GetBounds(m);
        Vec2 size = GetSize(bounds);

        // 8 surrounding offsets
        Vec2 offsets[8] = {
            {-size.x, -size.y}, // bottom-left
            {   0.0f, -size.y}, // bottom
            { size.x, -size.y}, // bottom-right
            {-size.x,    0.0f}, // left
            { size.x,    0.0f}, // right
            {-size.x,  size.y}, // top-left
            {   0.0f,  size.y}, // top
            { size.x,  size.y}, // top-right
        };

        BindColor(SetAlpha(COLOR_WHITE, 0.5f), Vec2Int{0,impl->palette});
        for (int i = 0; i < 8; i++) {
            Vec2 offset = offsets[i];
            DrawMesh(m, Translate(m->position + offset));
        }
        BindColor(COLOR_WHITE, Vec2Int{0,impl->palette});
    }

    bool hide_selected = DoesToolHideSelected();

    if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT && !hide_selected) {
        DrawSkeleton();
    }

    constexpr u16 MAX_EDITOR_VERTS = U16_MAX;
    constexpr u16 MAX_EDITOR_INDICES = U16_MAX;

    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MAX_EDITOR_VERTS, MAX_EDITOR_INDICES);
    BuildEditorMesh(builder, m, hide_selected);

    if (GetVertexCount(builder) > 0) {
        if (!g_mesh_editor.editor_mesh)
            g_mesh_editor.editor_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        else
            UpdateMeshFromBuilder(g_mesh_editor.editor_mesh, builder);

        BindDepth(0.0f);
        BindMaterial(g_view.editor_material);
        BindColor(COLOR_WHITE);
        BindTransform(MAT3_IDENTITY);
        DrawMesh(g_mesh_editor.editor_mesh);
    }
    PopScratch();

    if (!hide_selected) {
        DrawXRay();
    }
}

static void SubDivide() {
    MeshData* m = GetMeshData();
    RecordUndo(m);

    int selected_edges[MAX_EDGES];
    int selected_edge_count = GetSelectedEdges(m, selected_edges);

    for (int edge_index=0; edge_index<selected_edge_count; edge_index++) {
        int new_vertex = SplitEdge(GetMeshData(), selected_edges[edge_index], 0.5f, false);
        SelectVertex(new_vertex, true);
    }

    UpdateEdges(m);
    UpdateSelection();
    MarkModified(m);
}

static void ToggleAnchor() {
    BeginSelectTool({.commit= [](const Vec2& position ) {
        MeshData* m = GetMeshData();
        RecordUndo(m);

        int anchor_index = HitTestTag(m, position - m->position);
        if (anchor_index == -1)
            AddTag(m, position - m->position);
        else
            RemoveTag(m, anchor_index);

        UpdateEdges(m);
        MarkDirty(m);
        MarkModified(m);
    }});
}

static void BeginKnifeCut() {
    MeshData* m = GetMeshData();
    // In face mode with selected faces, restrict knife to selected faces only
    bool restrict_to_selected = (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) && (m->impl->selected_face_count > 0);
    BeginKnifeTool(m, restrict_to_selected);
}

static void SendBackward() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);

    for (int face_index=1; face_index<impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        FaceData& p = impl->faces[face_index - 1];
        if (!f.selected || p.selected)
            continue;

        SwapFace(m, face_index, face_index - 1);
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void BringForward() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);

    for (int face_index=impl->face_count-2; face_index>=0; face_index--) {
        FaceData& f = impl->faces[face_index];
        FaceData& n = impl->faces[face_index + 1];
        if (!f.selected || n.selected)
            continue;

        SwapFace(m, face_index, face_index + 1);
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void BringToFront() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);

    int write_index = 0;
    for (int read_index = 0; read_index < impl->face_count; read_index++) {
        if (!impl->faces[read_index].selected) {
            if (write_index != read_index) {
                FaceData temp = impl->faces[write_index];
                impl->faces[write_index] = impl->faces[read_index];
                impl->faces[read_index] = temp;
            }
            write_index++;
        }
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void SendToBack() {
    if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);

    int write_index = impl->face_count - 1;
    for (int read_index = impl->face_count - 1; read_index >= 0; read_index--) {
        if (!impl->faces[read_index].selected) {
            if (write_index != read_index) {
                FaceData temp = impl->faces[write_index];
                impl->faces[write_index] = impl->faces[read_index];
                impl->faces[read_index] = temp;
            }
            write_index--;
        }
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void ToggleXRay() {
    g_mesh_editor.xray = !g_mesh_editor.xray;
}

static void CommitParentTool(const Vec2& position) {
    AssetData* hit_asset = HitTestAssets(position);
    if (!hit_asset || hit_asset->type != ASSET_TYPE_SKELETON)
        return;

    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    RecordUndo(m);
    impl->skeleton = static_cast<SkeletonData*>(hit_asset);
    impl->skeleton_name = hit_asset->name;
    MarkModified(m);
}

static void BeginParentTool() {
    BeginSelectTool({.commit=CommitParentTool});
}

static void FlipHorizontal() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->selected_vertex_count == 0)
        return;

    RecordUndo(m);

    // Flip selected vertices horizontally around their center
    for (int i=0; i<impl->vertex_count; i++) {
        VertexData& v = impl->vertices[i];
        if (!v.selected)
            continue;
        v.position.x = g_mesh_editor.selection_center.x - (v.position.x - g_mesh_editor.selection_center.x);
    }

    // Reverse winding order of selected faces
    for (int face_index=0; face_index<impl->face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        if (!f.selected)
            continue;

        for (int i=0; i<f.vertex_count / 2; i++) {
            int temp = f.vertices[i];
            f.vertices[i] = f.vertices[f.vertex_count - 1 - i];
            f.vertices[f.vertex_count - 1 - i] = temp;
        }
    }

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified(m);
}

static void DuplicateSelected() {
    MeshData* m = GetMeshData();
    MeshDataImpl* impl = m->impl;
    if (impl->face_count + impl->selected_face_count > MAX_FACES)
        return;
    if (impl->vertex_count + impl->selected_vertex_count > MAX_VERTICES)
        return;

    RecordUndo(m);

    int old_face_count = impl->face_count;
    int old_vertex_count = impl->vertex_count;

    int vertex_map[MESH_MAX_VERTICES];
    for (int vertex_index=0; vertex_index<old_vertex_count; vertex_index++) {
        VertexData& v = impl->vertices[vertex_index];
        if (!v.selected) continue;
        vertex_map[vertex_index] = impl->vertex_count;
        impl->vertices[impl->vertex_count++] = v;
    }

    for (int face_index=0; face_index<old_face_count; face_index++) {
        FaceData& f = impl->faces[face_index];
        if (!f.selected) continue;
        int new_face_index = impl->face_count++;
        FaceData& nf = impl->faces[new_face_index];
        nf = f;
        f.selected = false;
        nf.selected = true;

        for (int vertex_index=0; vertex_index<f.vertex_count; vertex_index++)
            nf.vertices[vertex_index] = vertex_map[nf.vertices[vertex_index]];
    }

    MarkDirty(m);
    UpdateEdges(m);
    UpdateSelection(MESH_EDITOR_MODE_FACE);
    MarkModified(m);
    BeginMoveTool();
}

static void BeginMeshEditor(AssetData* a) {
    g_mesh_editor.mesh_data = static_cast<MeshData*>(a);
    g_view.vtable = {
        .allow_text_input = MeshViewAllowTextInput
    };

    PushInputSet(g_mesh_editor.input);

    SelectAll();

    g_mesh_editor.mode = MESH_EDITOR_MODE_VERTEX;
    g_mesh_editor.weight_bone = -1;

    SetFocus(CANVAS_ID_OVERLAY, MESH_EDITOR_ID_EXPAND);
}

static void EndMeshEditor() {
    g_mesh_editor.mesh_data = nullptr;
    PopInputSet();
}

void ShutdownMeshEditor() {
    if (g_mesh_editor.editor_mesh)
        Free(g_mesh_editor.editor_mesh);
    g_mesh_editor = {};
}

void InitMeshEditor() {
    g_mesh_editor.color_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);

    static Shortcut shortcuts[] = {
        { KEY_D, false, true, false, DuplicateSelected },
        { KEY_G, false, false, false, BeginMoveTool },
        { KEY_R, false, false, false, BeginRotateTool },
        { KEY_R, false, false, true, FlipHorizontal },
        { KEY_S, false, false, false, BeginScaleTool },
        { KEY_S, false, false, true, SubDivide },
        { KEY_W, false, false, false, BeginWeightTool },
        { KEY_A, false, false, false, SelectAll },
        { KEY_A, false, false, true, ToggleAnchor },
        { KEY_X, false, false, false, DissolveSelected },
        { KEY_V, false, false, false, InsertVertex },
        { KEY_1, false, false, false, SetVertexMode },
        { KEY_2, false, false, false, SetEdgeMode },
        { KEY_3, false, false, false, SetFaceMode },
        { KEY_4, false, false, false, SetWeightMode },
        { KEY_C, false, false, false, CenterMesh },
        { KEY_C, false, false, true, CircleMesh },
        { KEY_E, false, true, false, ExtrudeSelected },
        { KEY_N, false, false, false, NewFace },
        { KEY_V, false, false, true, BeginKnifeCut },
        { KEY_X, true, false, false, ToggleXRay },

        { KEY_LEFT_BRACKET, false, false, false, SendBackward },
        { KEY_RIGHT_BRACKET, false, false, false, BringForward },
        { KEY_RIGHT_BRACKET, false, true, false, BringToFront },
        { KEY_LEFT_BRACKET, false, true, false, SendToBack },
        { KEY_P, false, false, false, BeginParentTool },

        { INPUT_CODE_NONE }
    };

    g_mesh_editor.input = CreateInputSet(ALLOCATOR_DEFAULT);
    EnableModifiers(g_mesh_editor.input);
    EnableButton(g_mesh_editor.input, MOUSE_LEFT);
    EnableButton(g_mesh_editor.input, MOUSE_LEFT_DOUBLE_CLICK);

    g_mesh_editor.shortcuts = shortcuts;
    EnableButton(g_mesh_editor.input, KEY_Q);
    EnableButton(g_mesh_editor.input, KEY_O);
    EnableButton(g_mesh_editor.input, KEY_SPACE);
    EnableButton(g_mesh_editor.input, KEY_H);
    EnableShortcuts(shortcuts, g_mesh_editor.input);
    EnableCommonShortcuts(g_mesh_editor.input);

    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, Vec2{0,   0.5f}, Vec2{0,0});
    AddVertex(builder, Vec2{64,  0.5f}, Vec2{1,0});
    AddVertex(builder, Vec2{64, -0.5f}, Vec2{1,0.25f});
    AddVertex(builder, Vec2{0,  -0.5f}, Vec2{0,0.25f});
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    g_mesh_editor.color_picker_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, GetName("ColorPicker"));
    PopScratch();
}

void InitMeshEditor(MeshData* m) {
    m->vtable.editor_begin = BeginMeshEditor;
    m->vtable.editor_end = EndMeshEditor;
    m->vtable.editor_draw = DrawMeshEditor;
    m->vtable.editor_update = UpdateMeshEditor;
    m->vtable.editor_bounds = GetMeshEditorBounds;
    m->vtable.editor_overlay = MeshEditorOverlay;
}