//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "utils/rasterizer.h"
#include "utils/pixel_data.h"

namespace noz::editor {
    constexpr float FRAME_SIZE_X = 20;
    constexpr float FRAME_SIZE_Y = 40;
    constexpr float FRAME_BORDER_SIZE = 1;
    constexpr Color FRAME_BORDER_COLOR = Color24ToColor(32,32,32);
    constexpr float DOPESHEET_FRAME_DOT_SIZE = 5;
    constexpr float DOPESHEET_FRAME_DOT_OFFSET_X = FRAME_SIZE_X * 0.5f - DOPESHEET_FRAME_DOT_SIZE * 0.5f;
    constexpr float DOPESHEET_FRAME_DOT_OFFSET_Y = 5;
    constexpr Color DOPESHEET_FRAME_DOT_COLOR = FRAME_BORDER_COLOR;
    constexpr Color FRAME_COLOR = Color32ToColor(100, 100, 100, 255);
    constexpr Color DOPESHEET_SELECTED_FRAME_COLOR = COLOR_VERTEX_SELECTED;

    constexpr int MESH_EDITOR_ID_TOOLBAR = OVERLAY_BASE_ID + 0;
    constexpr int MESH_EDITOR_ID_EXPAND = OVERLAY_BASE_ID + 1;
    constexpr int MESH_EDITOR_ID_TILE = OVERLAY_BASE_ID + 2;
    constexpr int MESH_EDITOR_ID_ATLAS = OVERLAY_BASE_ID + 3;
    constexpr int MESH_EDITOR_ID_VERTEX_MODE = OVERLAY_BASE_ID + 4;
    constexpr int MESH_EDITOR_ID_EDGE_MODE = OVERLAY_BASE_ID + 5;
    constexpr int MESH_EDITOR_ID_FACE_MODE = OVERLAY_BASE_ID + 6;
    constexpr int MESH_EDITOR_ID_WEIGHT_MODE = OVERLAY_BASE_ID + 7;

    constexpr int MESH_EDITOR_ID_OPACITY = OVERLAY_BASE_ID + 8;
    constexpr int MESH_EDITOR_ID_PALETTES = MESH_EDITOR_ID_OPACITY + 12;
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
        MeshDocument* mesh_data;
        Mesh* mesh;
        Vec2 selection_drag_start;
        Vec2 selection_center;
        int selection_opacity;
        int selection_color;
        Material* color_material;
        bool clear_selection_on_up;
        bool clear_weight_bone_on_up;
        Vec2 state_mouse;
        bool use_fixed_value;
        bool ignore_up;
        bool use_negative_fixed_value;
        float fixed_value;
        Shortcut* shortcuts;
        VertexData saved_verts[MESH_MAX_VERTICES];
        EdgeData saved_edges[MESH_MAX_EDGES];
        InputSet* input;
        Mesh* color_picker_mesh;
        int weight_bone;
        bool xray;
        bool show_palette_picker;
        bool show_tiling;
        Shortcut* animation_shortcuts;
        int cached_frame;              // For onion skin cache invalidation
        Mesh* prev_frame_mesh;         // Onion skin: previous frame edges
        Mesh* next_frame_mesh;         // Onion skin: next frame edges
        bool onion_skin_enabled;       // Show onion skin overlay
        bool has_clipboard;
        float playback_time;           // Animation preview time
        bool is_playing;               // Animation playing
        bool show_opacity_popup;

        struct {
            VertexData vertices[MESH_MAX_VERTICES];
            FaceData faces[MESH_MAX_FACES];
            int vertex_count;
            int face_count;
            Vec2 center;
            bool has_content;
        } clipboard;

        struct {
            Material* material;
            Texture* texture;
            PixelData* pixels;
            Bounds2 bounds;
            Mesh* mesh;
            Vec2Int size;
            Rasterizer* rasterizer;
        } draw;

        Geometry geom;

        bool dirty;
    };

    static MeshEditor g_mesh_editor = {};

    extern int SplitFaces(MeshDocument* m, int v0, int v1);
    static void HandleBoxSelect(const Bounds2& bounds);

    static void Dopesheet();
    static void DrawOnionSkin();

    inline MeshDocument* GetMeshDocument() {
        Document* a = g_mesh_editor.mesh_data;
        assert(a->def->type == ASSET_TYPE_MESH);
        return (MeshDocument*)a;
    }

#if 0
    static Bounds2 RenderFrameEditTexture(MeshDocument* m, int frame_index) {
        float dpi = (float)g_editor.atlas.dpi;
        int padding = g_editor.atlas.padding;

        MeshFrameData* frame = GetFrame(m, frame_index);

        // Get bounds for current frame (must match RenderMeshPreview calculation)
        Bounds2 bounds;
        if (frame->geom.vert_count == 0) {
            bounds = {{-0.5f, -0.5f}, {0.5f, 0.5f}};
        } else {
            bounds = {frame->geom.verts[0].position, frame->geom.verts[0].position};
            for (int i = 1; i < frame->geom.vert_count; i++) {
                bounds.min.x = Min(bounds.min.x, frame->geom.verts[i].position.x);
                bounds.min.y = Min(bounds.min.y, frame->geom.verts[i].position.y);
                bounds.max.x = Max(bounds.max.x, frame->geom.verts[i].position.x);
                bounds.max.y = Max(bounds.max.y, frame->geom.verts[i].position.y);
            }
            // Include curve control points (must match RenderMeshPreview)
            for (int i = 0; i < frame->geom.edge_count; i++) {
                EdgeData& e = frame->geom.edges[i];
                if (LengthSqr(e.curve.offset) > 0.0001f) {
                    Vec2 p0 = frame->geom.verts[e.v0].position;
                    Vec2 p1 = frame->geom.verts[e.v1].position;
                    Vec2 control = (p0 + p1) * 0.5f + e.curve.offset;
                    bounds.min.x = Min(bounds.min.x, control.x);
                    bounds.min.y = Min(bounds.min.y, control.y);
                    bounds.max.x = Max(bounds.max.x, control.x);
                    bounds.max.y = Max(bounds.max.y, control.y);
                }
            }
        }

        // Snap bounds to pixel grid to prevent pixel shifting when bounds change
        float pixel_size = 1.0f / dpi;
        bounds.min.x = floorf(bounds.min.x / pixel_size) * pixel_size;
        bounds.min.y = floorf(bounds.min.y / pixel_size) * pixel_size;
        bounds.max.x = ceilf(bounds.max.x / pixel_size) * pixel_size;
        bounds.max.y = ceilf(bounds.max.y / pixel_size) * pixel_size;

        // Use roundf since bounds are snapped to pixel grid - avoids floating point errors
        int content_width = (int)roundf((bounds.max.x - bounds.min.x) * dpi);
        int content_height = (int)roundf((bounds.max.y - bounds.min.y) * dpi);
        Vec2Int size = {
            Max(content_width + padding * 2, 1),
            Max(content_height + padding * 2, 1)
        };

        if (size != frame->edit_size) {
            Free(frame->edit_pixels);
            Free(frame->edit_texture);
            frame->edit_texture = nullptr;
            frame->edit_pixels = static_cast<Color32*>(Alloc(ALLOCATOR_DEFAULT, size.x * size.y *sizeof(Color32)));
        }

        RenderMeshPreview(m, frame->edit_pixels, size, &bounds);

        // Upload to GPU
        if (!frame->edit_texture) {
            frame->edit_texture = CreateTexture(
                ALLOCATOR_DEFAULT, frame->edit_pixels, width, height,
                TEXTURE_FORMAT_RGBA8, GetName("frame_edit"),
                g_editor.atlas.filter);
        } else {
            UpdateTexture(frame->edit_texture, frame->edit_pixels);
        }

        g_mesh_editor.mesh_dirty = false;
        return bounds;
    }
#endif

    static void UpdateVertexSelection(MeshDocument* m) {
        MeshFrameData* frame = GetCurrentFrame(m);
        frame->selected_vertex_count = 0;
        frame->selected_edge_count = 0;
        frame->selected_face_count = 0;

        for (int vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++) {
            const VertexData* v = frame->geom.verts + vertex_index;
            if (!IsSelected(v)) continue;
            frame->selected_vertex_count++;
        }

        for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            VertexData* v0 = GetVertex(frame, e->v0);
            VertexData* v1 = GetVertex(frame, e->v1);
            SetFlags(e, IsSelected(v0) && IsSelected(v1) ? EDGE_FLAG_SELECTED : EDGE_FLAG_NONE, EDGE_FLAG_SELECTED);
            if (IsSelected(e))
                frame->selected_edge_count++;
        }

        for (u16 face_index=0; face_index<frame->geom.face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            u16 face_vertices[MESH_MAX_FACE_VERTICES];
            u16 vertex_count = GetFaceVertices(frame, f, face_vertices);

            bool selected = true;
            for (int face_vertex_index=0; selected && face_vertex_index<vertex_count; face_vertex_index++)
                selected &= IsVertexSelected(frame, face_vertices[face_vertex_index]);

            if (selected) {
                frame->selected_face_count++;
                SetFlags(f, FACE_FLAG_SELECTED, FACE_FLAG_SELECTED);
            } else {
                SetFlags(f, FACE_FLAG_NONE, FACE_FLAG_SELECTED);
            }
        }
    }

    static void UpdateEdgeSelection(MeshDocument* m) {
        MeshFrameData* frame = GetCurrentFrame(m);
        frame->selected_vertex_count = 0;
        frame->selected_face_count = 0;
        frame->selected_edge_count = 0;

        for (u16 vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++)
            SetFlags(GetVertex(frame, vertex_index), VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);

        for (u16 face_index=0; face_index<frame->geom.face_count; face_index++)
            SetFlags(GetFace(frame, face_index), FACE_FLAG_NONE, FACE_FLAG_SELECTED);

        for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            if (!IsSelected(e)) continue;

            frame->selected_edge_count++;

            VertexData* v0 = frame->geom.verts + e->v0;
            VertexData* v1 = frame->geom.verts + e->v1;

            if (!IsSelected(v0)) {
                SetFlags(v0, VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
                frame->selected_vertex_count++;
            }

            if (!IsSelected(v1)) {
                SetFlags(v1, VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
                frame->selected_vertex_count++;
            }
        }

        for (u16 face_index=0; face_index<frame->geom.face_count; face_index++) {
            FaceData* f = frame->geom.faces + face_index;

            u16 selected_edge_count = 0;
            for (u16 edge_index=0; selected_edge_count < f->edge_count; edge_index++) {
                EdgeData* e = GetEdge(frame, edge_index);
                if (!IsSelected(e)) continue;
                selected_edge_count++;
            }

            if (selected_edge_count == f->edge_count) {
                SetFlags(f, FACE_FLAG_SELECTED, FACE_FLAG_SELECTED);
                frame->selected_face_count++;
            }
        }
    }

    static void UpdateFaceSelection(MeshDocument* m) {
        MeshFrameData* frame = GetCurrentFrame(m);
        frame->selected_vertex_count = 0;
        frame->selected_edge_count = 0;
        frame->selected_face_count = 0;

        for (u16 vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++)
            SetFlags(frame->geom.verts + vertex_index, VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);

        for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++)
            SetFlags(GetEdge(frame, edge_index), EDGE_FLAG_NONE, EDGE_FLAG_SELECTED);

        for (u16 face_index=0; face_index<frame->geom.face_count; face_index++) {
            FaceData* f = frame->geom.faces + face_index;
            if (!IsSelected(f)) continue;

            frame->selected_face_count++;

            for (u16 face_edge_index=0; face_edge_index<f->edge_count; face_edge_index++) {
                EdgeData* e = GetFaceEdge(frame, f, face_edge_index);
                if (IsSelected(e)) continue;

                SetFlags(e, EDGE_FLAG_SELECTED, EDGE_FLAG_SELECTED);
                frame->selected_edge_count++;

                VertexData* v0 = frame->geom.verts + e->v0;
                if (!IsSelected(v0)) {
                    SetFlags(v0, VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
                    frame->selected_vertex_count++;
                }

                VertexData* v1 = frame->geom.verts + e->v1;
                if (!IsSelected(v1)) {
                    SetFlags(v1, VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
                    frame->selected_vertex_count++;
                }
            }
        }
    }

    static void UpdateSelectionCenter() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        Bounds2 bounds = {VEC2_ZERO, VEC2_ZERO};
        u16 vertex_index = 0;
        for (; vertex_index<frame->geom.vert_count; vertex_index++) {
            VertexData* v = GetVertex(frame, vertex_index);
            if (!IsSelected(v)) continue;
            bounds = {v->position, v->position};
            break;
        }

        for (;vertex_index<frame->geom.vert_count; vertex_index++) {
            VertexData* v = GetVertex(frame, vertex_index);
            if (!IsSelected(v)) continue;
            bounds = Union(bounds, {v->position, v->position});
        }

        if (frame->selected_vertex_count > 0)
            g_mesh_editor.selection_center = GetCenter(bounds);
        else
            g_mesh_editor.selection_center = VEC2_ZERO;
    }

    static void UpdateSelectionColor() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        for (u16 face_index=0; face_index<frame->geom.face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            if (!IsSelected(f)) continue;
            g_mesh_editor.selection_opacity = Clamp((int)(f->opacity * 10.0f), 0, 10);
            g_mesh_editor.selection_color = f->color;
            return;
        }
    }

    static void UpdateSelection(MeshEditorMode mode=MESH_EDITOR_MODE_CURRENT) {
        MeshDocument* m = GetMeshDocument();

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
        UpdateSelectionColor();
    }

    void RefreshMeshEditorSelection() {
        UpdateSelection();
    }

    static void ClearSelection() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        for (u16 vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++)
            SetFlags(GetVertex(frame, vertex_index), VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);

        for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++)
            SetFlags(GetEdge(frame, edge_index), EDGE_FLAG_NONE, EDGE_FLAG_SELECTED);

        for (u16 face_index=0; face_index<frame->geom.face_count; face_index++)
            SetFlags(GetFace(frame, face_index), FACE_FLAG_NONE, FACE_FLAG_SELECTED);

        UpdateSelection();
    }

    static void SelectAll(MeshDocument* m) {
        MeshFrameData* frame = GetCurrentFrame(m);

        if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) {
            for (u16 face_index=0; face_index<frame->geom.face_count; face_index++)
                SetFlags(GetFace(frame, face_index), FACE_FLAG_SELECTED, FACE_FLAG_SELECTED);
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) {
            for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++)
                SetFlags(GetEdge(frame, edge_index), EDGE_FLAG_SELECTED, EDGE_FLAG_SELECTED);
        } else {
            for (u16 vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++)
                SetFlags(GetVertex(frame, vertex_index), VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
        }

        UpdateSelection();
    }

    static void SelectVertex(u16 vertex_index, bool selected) {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        assert(vertex_index < frame->geom.vert_count);
        SetFlags(GetVertex(frame, vertex_index), selected ? VERTEX_FLAG_SELECTED : VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);
        UpdateSelection(MESH_EDITOR_MODE_VERTEX);
    }

    void SelectEdge(u16 edge_index, bool selected) {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        assert(edge_index < frame->geom.edge_count);

        EdgeData* e = GetEdge(frame, edge_index);
        SetFlags(e, selected ? EDGE_FLAG_SELECTED : EDGE_FLAG_NONE, EDGE_FLAG_SELECTED);
        UpdateSelection(MESH_EDITOR_MODE_EDGE);
    }

    void SelectFace(u16 face_index, bool selected) {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        assert(face_index < frame->geom.face_count);
        FaceData* f = GetFace(frame, face_index);
        SetFlags(f, selected ? FACE_FLAG_SELECTED : FACE_FLAG_NONE, FACE_FLAG_SELECTED);
        UpdateSelection(MESH_EDITOR_MODE_FACE);
    }

    static void SaveMeshState() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        for (u16 i=0; i<frame->geom.vert_count; i++)
            g_mesh_editor.saved_verts[i] = frame->geom.verts[i];
        for (u16 i=0; i<frame->geom.edge_count; i++)
            g_mesh_editor.saved_edges[i] = frame->geom.edges[i];
    }

    static void RevertMeshState() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        for (u16 i=0; i < frame->geom.vert_count; i++)
            frame->geom.verts[i] = g_mesh_editor.saved_verts[i];
        for (u16 i=0; i < frame->geom.edge_count; i++)
            frame->geom.edges[i] = g_mesh_editor.saved_edges[i];

        MarkDirty(m);
        MarkModified(m);
        UpdateSelection();
    }

    static bool TrySelectVertex() {
        assert(g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX || g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT);

        MeshDocument* m = GetMeshDocument();
        int vertex_index = HitTestVertex(m, g_workspace.mouse_world_position);
        if (vertex_index == -1)
            return false;

        bool shift = IsShiftDown();
        if (!shift)
            ClearSelection();

        VertexData* v = GetVertex(GetCurrentFrame(m), static_cast<u16>(vertex_index));
        if (!shift || !IsSelected(v))
            SetFlags(v, VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
        else
            SetFlags(v, VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);

        return true;
    }

    static bool TrySelectEdge() {
        assert(g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE);

        MeshDocument* m = GetMeshDocument();
        int edge_index = HitTestEdge(m, g_workspace.mouse_world_position);
        if (edge_index == -1)
            return false;

        bool shift = IsShiftDown();
        if (!shift)
            ClearSelection();

        EdgeData* e = GetEdge(GetCurrentFrame(m), static_cast<u16>(edge_index));
        if (!shift || !IsSelected(e))
            SetFlags(e, EDGE_FLAG_SELECTED, EDGE_FLAG_SELECTED);
        else
            SetFlags(e, EDGE_FLAG_NONE, EDGE_FLAG_SELECTED);

        return true;
    }

    static bool TrySelectFace() {
#if 0
        assert(g_mesh_editor.mode == MESH_EDITOR_MODE_FACE);

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        int hit_faces[MESH_MAX_FACES];
        int hit_count = HitTestFaces(
            m,
            Translate(m->position),
            g_workspace.mouse_world_position,
            hit_faces,
            MESH_MAX_FACES);

        if (hit_count == 0)
            return false;

        bool shift = IsShiftDown();
        int hit_index = 0;
        if (shift) {
            // With shift: cycle through overlapping faces to toggle selection
            for (;hit_index<hit_count; hit_index++)
                if (IsSelected(GetFace(fram,e  GetCurrentFrame(m)->faces[hit_faces[hit_index]].selected)
                    break;

            if (hit_index == hit_count)
            hit_index = 0;
    }
        // Without shift: always select topmost face (hit_index stays 0)

        int face_index = hit_faces[hit_index];
        if (!shift)
            ClearSelection();

        if (!shift || !GetCurrentFrame(m)->faces[face_index].selected)
            SelectFace(face_index, true);
        else
            SelectFace(face_index, false);

        return true;
#else
        return false;
#endif
    }

    static bool TrySelectBone() {
        assert(g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT);

        MeshDocument* mdoc = GetMeshDocument();
        if (mdoc->skeleton == nullptr)
            return false;

        int hit[MAX_BONES];
        int hit_count = HitTestBones(mdoc->skeleton, Translate(GetMeshDocument()->position), g_workspace.mouse_world_position, hit, MAX_BONES);
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

        MeshDocument* m = GetMeshDocument();
        RecordUndo(m);

        if (GetCurrentFrame(m)->selected_vertex_count >= 3) {
            int face_index = CreateFace(m);
            if (face_index == -1)
                CancelUndo();

            return;
        }

        int vertex_index = HitTestVertex(m, g_workspace.mouse_world_position, 0.1f);
        if (vertex_index != -1)
            return;

        float edge_pos;
        int edge_index = HitTestEdge(m, g_workspace.mouse_world_position, &edge_pos);
        if (edge_index < 0)
            return;


        int new_vertex_index = SplitEdge(m, edge_index, edge_pos);
        if (new_vertex_index == -1)
            return;

        ClearSelection();
        SelectVertex(static_cast<u16>(new_vertex_index), true);
    }

    static void DissolveSelected() {
#if 0
        MeshDocument* m = GetMeshDocument();

        if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT)
            return;
        if (GetCurrentFrame(m)->selected_vertex_count == 0)
            return;

        RecordUndo(m);

        // Track selected vertices before dissolving (for orphan cleanup)
        bool was_selected[MESH_MAX_VERTICES] = {};
        for (int i=0; i<GetCurrentFrame(m)->vertex_count; i++)
            was_selected[i] = IsSelected(GetVertex(GetCurrentFrame(m), static_cast<u16>(i)));

        if (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX) {
            DeleteSelectedVertices(m);
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) {
            for (int i=GetCurrentFrame(m)->edge_count-1; i>=0; i--)
                if (IsSelected(GetEdge(GetCurrentFrame(m), static_cast<u16>(i))))
                    DeleteEdge(m, i);
            ClearSelection();
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) {
            DeleteSelectedFaces(m);
        }

        // Remove vertices that were selected and are now orphaned
        for (int i=GetCurrentFrame(m)->vertex_count-1; i>=0; i--) {
            if (was_selected[i] && GetCurrentFrame(m)->vertices[i].ref_count == 0)
                DeleteVertex(m, i);
        }

        MarkDirty(m);
        MarkModified(m);
        UpdateSelection();
#endif
    }

    static bool TryDoubleClickSelectFaceVertices() {
#if 0
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_VERTEX && g_mesh_editor.mode != MESH_EDITOR_MODE_WEIGHT)
            return false;

        MeshDocument* m = GetMeshDocument();
        int vertex_index = HitTestVertex(m, g_workspace.mouse_world_position);
        if (vertex_index == -1)
            return false;

        // Find all faces containing this vertex
        bool shift = IsShiftDown();
        bool all_selected = true;

        for (int face_index = 0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
            FaceData& f = GetCurrentFrame(m)->faces[face_index];
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
                if (!GetCurrentFrame(m)->vertices[f.vertices[i]].selected) {
                    all_selected = false;
                    break;
                }
            }
        }

        // If shift and all selected, deselect. Otherwise select.
        bool should_select = !(shift && all_selected);

        if (!shift)
            ClearSelection();

        for (int face_index = 0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
            FaceData& f = GetCurrentFrame(m)->faces[face_index];
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
                GetCurrentFrame(m)->vertices[f.vertices[i]].selected = should_select;
        }

        UpdateSelection();
        return true;
#else
        return false;
#endif
    }

    static void UpdateDefaultState() {
        // Only start box select for left-drag, not right-drag (which is for panning)
        if (!IsToolActive() && g_workspace.drag_started && g_workspace.drag_button == MOUSE_LEFT) {
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

        // Select (skip if right-drag is active or just ended to prevent accidental deselection after panning)
        bool right_drag_active = g_workspace.drag && g_workspace.drag_button == MOUSE_RIGHT;
        bool right_released = WasButtonReleased(GetInputSet(), MOUSE_RIGHT);
        if (!g_mesh_editor.ignore_up && !right_drag_active && !right_released && WasButtonReleased(g_mesh_editor.input, MOUSE_LEFT)) {
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

        if (!right_drag_active && !right_released && WasButtonReleased(g_mesh_editor.input, MOUSE_LEFT)) {
            if (g_mesh_editor.clear_selection_on_up)
                ClearSelection();

            if (g_mesh_editor.clear_weight_bone_on_up)
                g_mesh_editor.weight_bone = -1;
        }
    }

    constexpr int   STAT_FONT_SIZE = 14;
    constexpr float STAT_SPACING = 4.0f;
    constexpr float DEBUG_UI_PROPERTY_HEIGHT = 18.0f;
    constexpr float DEBUG_UI_PROPERTY_NAME_WIDTH = 100.0f;
    constexpr float DEBUG_UI_PROPERTY_VALUE_WIDTH = 60.0f;

    static void AddStat(const char* name, int value) {
        BeginContainer({.height=DEBUG_UI_PROPERTY_HEIGHT});
        BeginRow();
        {
            // label
            BeginContainer({.width=DEBUG_UI_PROPERTY_NAME_WIDTH});
            Label(name, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_TEXT_COLOR()});
            EndContainer();

            // value
            BeginContainer({.width=DEBUG_UI_PROPERTY_VALUE_WIDTH});
            Text text;
            Format(text, "%d", value);
            Label(text, {.font=FONT_SEGUISB, .font_size=STYLE_OVERLAY_TEXT_SIZE, .color=STYLE_OVERLAY_ACCENT_TEXT_COLOR()});
            EndContainer();
        }
        EndRow();
        EndContainer();
    }

    static void MeshStats() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        BeginOverlay(ELEMENT_ID_NONE, ALIGN_TOP_RIGHT);
        BeginColumn({.spacing=STAT_SPACING});
        {
            AddStat("Vertices", frame->geom.vert_count);
            AddStat("Edges", frame->geom.edge_count);
            AddStat("Faces", frame->geom.face_count);
            if (frame->mesh)
                AddStat("Triangles", GetIndexCount(frame->mesh) / 3);
        }
        EndColumn();
        EndOverlay();
    }

    static bool Palette(int palette_index, bool* selected_colors) {
        BeginContainer({
            .padding=EdgeInsetsAll(COLOR_PICKER_BORDER_WIDTH),
            .id = static_cast<ElementId>(MESH_EDITOR_ID_PALETTES + palette_index)
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
                .id=selected_colors ? static_cast<ElementId>(MESH_EDITOR_ID_COLORS + i) : ELEMENT_ID_NONE
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
                if (IsShiftDown()) {
                    ClearSelection();
                    // Shift+click: select all faces with this color
                    MeshDocument* m = GetMeshDocument();
                    MeshFrameData* frame = GetCurrentFrame(m);
                    bool any_selected = false;
                    for (u16 face_index = 0; face_index < frame->geom.face_count; face_index++) {
                        FaceData* f = GetFace(frame, face_index);
                        if (f->color == i) {
                            SetFlags(f, FACE_FLAG_SELECTED, FACE_FLAG_SELECTED);
                            any_selected = true;
                        }
                    }
                    if (any_selected)
                        UpdateSelection(MESH_EDITOR_MODE_FACE);
                } else {
                    MeshDocument* m = GetMeshDocument();
                    MeshFrameData* frame = GetCurrentFrame(m);
                    if (frame->selected_face_count > 0) {
                        RecordUndo(m);
                        SetFaceColor(m, static_cast<u8>(i));
                        MarkModified(m);
                        UpdateSelectionColor();
                    } else {
                        g_mesh_editor.selection_color = i;
                    }
                }
            }
            EndContainer();
        }
        EndGrid();
        EndColumn();

        bool pressed = !selected_colors && WasPressed();

        EndContainer();

        return pressed;
    }


    static bool OpacityPopup() {
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
            BeginContainer({.id=static_cast<ElementId>(MESH_EDITOR_ID_OPACITY + i + 1)});
            if (WasPressed()) {
                MeshDocument* m = GetMeshDocument();
                MeshFrameData* frame = GetCurrentFrame(m);
                if (frame->selected_face_count > 0) {
                    RecordUndo(m);
                    SetFaceOpacity(m, opacity);
                    MarkModified(m);
                    MarkDirty(m);
                    UpdateSelectionColor();
                } else {
                    g_mesh_editor.selection_opacity = i;
                }

                open = false;
            }

            BeginContainer({
                .width=STYLE_BUTTON_HEIGHT,
                .height=STYLE_BUTTON_HEIGHT,
                .border={.radius=STYLE_BUTTON_BORDER_RADIUS},
                .clip=false
            });

            if (IsHovered() || g_mesh_editor.selection_opacity == 10 - i)
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
            .color=SetAlpha(COLOR_WHITE, g_mesh_editor.selection_opacity / 10.0f),
            .material=g_workspace.editor_mesh_material
        });
    }

    static void ColorPicker(){
        // todo: we could cache this when the mesh is modified or selection changes update it
        static bool selected_colors[COLOR_COUNT] = {};
        memset(selected_colors, 0, sizeof(selected_colors));
        MeshDocument* mdoc = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(mdoc);
        if (frame->selected_face_count > 0) {
            for (u16 face_index=0; face_index<frame->geom.face_count; face_index++) {
                FaceData* f = GetFace(frame, face_index);
                if (!IsSelected(f)) continue;
                selected_colors[f->color] = true;
            }
        } else {
            selected_colors[g_mesh_editor.selection_color] = true;
        }

        // palettes
        BeginContainer({.padding=EdgeInsetsAll(4), .color=STYLE_OVERLAY_CONTENT_COLOR(), .border{.radius=STYLE_OVERLAY_CONTENT_BORDER_RADIUS}});
        BeginColumn();
        {
            int current_palette_index = g_editor.palette_map[mdoc->palette];
            if (g_mesh_editor.show_palette_picker) {
                for (int palette_index=0; palette_index<g_editor.palette_count; palette_index++) {
                    if (palette_index == current_palette_index) continue;
                    if (Palette(palette_index, nullptr)) {
                        RecordUndo(mdoc);
                        mdoc->palette = g_editor.palettes[palette_index].id;
                        MarkDirty(mdoc);
                        MarkModified(mdoc);
                    }

                    Spacer(2.0f);
                }
            }

            BeginRow();
            Palette(current_palette_index, g_mesh_editor.is_playing ? nullptr : selected_colors);
            BeginContainer({.align=ALIGN_BOTTOM_CENTER, .margin=EdgeInsetsAll(4)});
            if (EditorButton({
                .id=MESH_EDITOR_ID_OPACITY,
                .width = COLOR_PICKER_COLOR_SIZE * 2,
                .height = COLOR_PICKER_COLOR_SIZE * 2,
                .checked=g_mesh_editor.show_opacity_popup,
                .content_func=OpacityContent,
                .popup_func=OpacityPopup}))
                g_mesh_editor.show_opacity_popup = !g_mesh_editor.show_opacity_popup;
            EndContainer();

            EndRow();
        }
        EndColumn();
        EndContainer();
    }

    static void MeshEditorToolbar() {
        MeshDocument* mdoc = GetMeshDocument();
        bool show_palette_picker = g_mesh_editor.show_palette_picker;

        BeginOverlay(MESH_EDITOR_ID_TOOLBAR, ALIGN_BOTTOM_CENTER);
        BeginColumn({.spacing=8});

        Dopesheet();

        // Buttons
        BeginContainer();
        BeginRow({.align=ALIGN_LEFT, .spacing=4});

        if (EditorButton(MESH_EDITOR_ID_VERTEX_MODE, MESH_ICON_VERTEX_MODE, g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX))
            g_mesh_editor.mode = MESH_EDITOR_MODE_VERTEX;
        if (EditorButton(MESH_EDITOR_ID_EDGE_MODE, MESH_ICON_EDGE_MODE, g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE))
            g_mesh_editor.mode = MESH_EDITOR_MODE_EDGE;
        if (EditorButton(MESH_EDITOR_ID_FACE_MODE, MESH_ICON_FACE_MODE, g_mesh_editor.mode == MESH_EDITOR_MODE_FACE))
            g_mesh_editor.mode = MESH_EDITOR_MODE_FACE;
        if (EditorButton(MESH_EDITOR_ID_WEIGHT_MODE, MESH_ICON_WEIGHT_MODE, g_mesh_editor.mode == MESH_EDITOR_MODE_FACE, mdoc->skeleton == nullptr))
            g_mesh_editor.mode = MESH_EDITOR_MODE_WEIGHT;

        EndRow();

        BeginRow({.align=ALIGN_RIGHT, .spacing=6});
        if (EditorButton(MESH_EDITOR_ID_TILE, MESH_ICON_TILING, g_mesh_editor.show_tiling))
            g_mesh_editor.show_tiling = !g_mesh_editor.show_tiling;
        if (EditorButton(MESH_EDITOR_ID_EXPAND, SPRITE_ICON_PALETTE, g_mesh_editor.show_palette_picker, g_editor.palette_count < 2))
            show_palette_picker = !g_mesh_editor.show_palette_picker;
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
        MeshDocument* em = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(em);
        Bounds2 bounds = BOUNDS2_ZERO;
        bool first = true;
        for (u16 i = 0; i < frame->geom.vert_count; i++) {
            const VertexData* v = GetVertex(frame, i);
            if (!IsSelected(v)) continue;

            if (first)
                bounds = {v->position, v->position};
            else
                bounds = Union(bounds, v->position);

            first = false;
        }

        if (first)
            return GetBounds(GetMeshDocument());

        return bounds;
    }

    static void HandleBoxSelect(const Bounds2& bounds) {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        bool shift = IsShiftDown();
        if (!shift)
            ClearSelection();

        switch (g_mesh_editor.mode) {
            case MESH_EDITOR_MODE_VERTEX:
            case MESH_EDITOR_MODE_WEIGHT:
                for (u16 i=0; i<frame->geom.vert_count; i++) {
                    VertexData* v = GetVertex(frame, i);
                    Vec2 vpos = v->position + m->position;

                    if (vpos.x >= bounds.min.x && vpos.x <= bounds.max.x &&
                        vpos.y >= bounds.min.y && vpos.y <= bounds.max.y) {
                        SetFlags(v, VERTEX_FLAG_SELECTED, VERTEX_FLAG_SELECTED);
                        }
                }
                break;

            case MESH_EDITOR_MODE_EDGE:
                for (u16 edge_index=0; edge_index < frame->geom.edge_count; edge_index++) {
                    EdgeData* e = GetEdge(frame, edge_index);
                    Vec2 ev0 = frame->geom.verts[e->v0].position + m->position;
                    Vec2 ev1 = frame->geom.verts[e->v1].position + m->position;
                    if (Intersects(bounds, ev0, ev1)) {
                        SetFlags(e, EDGE_FLAG_SELECTED, EDGE_FLAG_SELECTED);
                    }
                }
                break;

            case MESH_EDITOR_MODE_FACE:
#if 0
                for (u16 face_index=0; face_index<GetCurrentFrame(m)->face_count; face_index++) {
                    FaceData* f = GetFace(GetCurrentFrame(m), face_index);
                    for (u16 vertex_index=0; vertex_index<f.vertex_count; vertex_index++) {
                        int v0 = f.vertices[vertex_index];
                        int v1 = f.vertices[(vertex_index + 1) % f.vertex_count];
                        Vec2 v0p = GetCurrentFrame(m)->vertices[v0].position + m->position;
                        Vec2 v1p = GetCurrentFrame(m)->vertices[v1].position + m->position;
                        if (Intersects(bounds, v0p, v1p)) {
                            SelectFace(face_index, true);
                            f.selected = true;
                        }
                    }
                }
#endif
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
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        bool coarse_snap = IsCtrlDown(GetInputSet());
        for (u16 i=0; i<frame->geom.vert_count; i++) {
            VertexData* v = GetVertex(frame, i);
            VertexData& s = g_mesh_editor.saved_verts[i];
            if (IsSelected(v)) {
                Vec2 new_pos = m->position + s.position + delta;
                new_pos = coarse_snap ? SnapToGrid(new_pos) : SnapToPixelGrid(new_pos);
                v->position = new_pos - m->position;
            }
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void BeginMoveTool() {
        MeshDocument* m = GetMeshDocument();
        if (GetCurrentFrame(m)->selected_vertex_count == 0)
            return;

        SaveMeshState();
        RecordUndo(m);
        BeginMoveTool({.update=UpdateMoveTool, .commit=CommitMoveTool, .cancel=CancelMeshTool});
    }

    static void BeginCurveToolFromSelection() {
        MeshDocument* m = GetMeshDocument();
        if (GetCurrentFrame(m)->selected_edge_count == 0)
            return;

        u16 selected_edges[MESH_MAX_EDGES];
        u16 count = GetSelectedEdges(m, selected_edges);
        if (count == 0)
            return;

        BeginCurveTool(m, selected_edges, count);
    }

    static void UpdateRotateTool(float angle) {
        if (IsCtrlDown()) {
            int angle_step = (int)(angle / 15.0f);
            angle = angle_step * 15.0f;
        }

        float cos_angle = Cos(Radians(angle));
        float sin_angle = Sin(Radians(angle));

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        for (i16 i=0; i<frame->geom.vert_count; i++) {
            VertexData* v = GetVertex(frame, i);
            if (!IsSelected(v)) continue;

            VertexData& s = g_mesh_editor.saved_verts[i];
            Vec2 relative_pos = s.position - g_mesh_editor.selection_center;

            Vec2 rotated_pos;
            rotated_pos.x = relative_pos.x * cos_angle - relative_pos.y * sin_angle;
            rotated_pos.y = relative_pos.x * sin_angle + relative_pos.y * cos_angle;
            v->position = g_mesh_editor.selection_center + rotated_pos;
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void BeginRotateTool() {
        MeshDocument* m = GetMeshDocument();
        if (GetCurrentFrame(m)->selected_vertex_count == 0 || (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX && GetCurrentFrame(m)->selected_vertex_count == 1))
            return;

        SaveMeshState();
        RecordUndo(m);
        BeginRotateTool({.origin=g_mesh_editor.selection_center+m->position, .update=UpdateRotateTool, .cancel=CancelMeshTool});
    }

    static void UpdateScaleTool(const Vec2& scale) {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        Vec2 center = g_mesh_editor.selection_center;
        if (IsCtrlDown())
            center = HitTestSnap(m, g_workspace.drag_world_position - m->position);

        SetScaleToolOrigin(center+m->position);

        for (u16 vertex_index=0; vertex_index < frame->geom.vert_count; vertex_index++) {
            VertexData* v = GetVertex(frame, vertex_index);
            if (!IsSelected(v)) continue;
            Vec2 dir = g_mesh_editor.saved_verts[vertex_index].position - center;
            v->position = center + dir * scale;
        }

        for (u16 edge_index=0; edge_index < frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            VertexData* v0 = GetVertex(frame, e->v0);
            VertexData* v1 = GetVertex(frame, e->v1);
            if (!IsSelected(v0) || !IsSelected(v1))
                continue;

            // CurveData& c = frame->curves[e.curve];
            // c.offset = g_mesh_editor.saved_curves[edge_index].offset * scale;
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void BeginScaleTool() {
        MeshDocument* m = GetMeshDocument();
        if (GetCurrentFrame(m)->selected_vertex_count == 0)
            return;

        SaveMeshState();
        RecordUndo(m);
        BeginScaleTool({.origin=g_mesh_editor.selection_center+m->position, .update=UpdateScaleTool, .cancel=CancelMeshTool});
    }

    // Weight painting disabled - meshes now render as textured quads
    // Bone selection still works via TrySelectBone() for picking bones
#if 0
    static void UpdateWeightToolVertex(float weight, void* user_data) {
        int vertex_index = (int)(i64)user_data;
        SetVertexWeight(GetMeshData(), vertex_index, g_mesh_editor.weight_bone, weight);
    }

    static void UpdateWeightTool() {
        MeshDocument* m = GetMeshData();
        Update(m);
        MarkDirty(m);
        MarkModified();
    }

    static void BeginWeightTool() {
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_WEIGHT)
            return;

        if (g_mesh_editor.weight_bone == -1)
            return;

        MeshDocument* m = GetMeshData();
        if (GetCurrentFrame(m)->selected_vertex_count == 0)
            return;

        WeightToolOptions options = {
            .vertex_count = 0,
            .min_weight = 0,
            .max_weight = 1,
            .update = UpdateWeightTool,
            .cancel = CancelMeshTool,
            .update_vertex = UpdateWeightToolVertex,
        };

        for (int i=0; i<GetCurrentFrame(m)->vertex_count; i++) {
            VertexData& v = GetCurrentFrame(m)->vertices[i];
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
#endif

    static void SelectAll() {
        SelectAll(GetMeshDocument());
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
        if (GetMeshDocument()->skeleton == nullptr)
            return;

        g_mesh_editor.mode = MESH_EDITOR_MODE_WEIGHT;
    }

    static void CenterMesh() {
        Center(GetMeshDocument());
    }

#if 0
    static void CircleMesh() {
        if (GetMeshData()->GetCurrentFrame(m)->selected_vertex_count < 2)
            return;

        BeginSelectTool({.commit= [](const Vec2& position ) {
            MeshDocument* m = GetMeshData();
            MeshDataImpl* impl = m->impl;
            Vec2 center = position - m->position;
            float total_distance = 0.0f;
            int count = 0;
            for (int i=0; i<GetCurrentFrame(m)->vertex_count; i++) {
                VertexData& v = GetCurrentFrame(m)->vertices[i];
                if (!v.selected)
                    continue;
                total_distance += Length(v.position - center);
                count++;
            }

            float radius = total_distance / (float)count;

            RecordUndo(m);

            // move selected vertices to form a circle around the center point
            for (int i=0; i<GetCurrentFrame(m)->vertex_count; i++) {
                VertexData& v = GetCurrentFrame(m)->vertices[i];
                if (!v.selected)
                    continue;
                Vec2 dir = Normalize(v.position - center);
                v.position = center + dir * radius;
            }

            Update(m);
            MarkDirty(m);
            MarkModified(m);
        }});
    }
#endif

    static bool ExtrudeSelectedEdges(MeshDocument* m) {
#if 0
        MeshFrameData* frame = GetCurrentFrame(m);

        if (frame->geom.edge_count == 0)
            return false;

        u16 selected_edges[MESH_MAX_EDGES];
        u16 selected_edge_count = 0;
        for (u16 edge_index = 0; edge_index < frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            if (IsSelected(e) && selected_edge_count < MESH_MAX_EDGES)
                selected_edges[selected_edge_count++] = edge_index;
        }

        if (selected_edge_count == 0)
            return false;

        // Find all unique vertices that are part of selected edges
        bool vertex_needs_extrusion[MESH_MAX_VERTICES] = {};
        for (int i = 0; i < selected_edge_count; i++) {
            const EdgeData& ee = GetCurrentFrame(m)->edges[selected_edges[i]];
            vertex_needs_extrusion[ee.v0] = true;
            vertex_needs_extrusion[ee.v1] = true;
        }

        // Create mapping from old vertex indices to new vertex indices
        int vertex_mapping[MESH_MAX_VERTICES];
        for (int i = 0; i < MESH_MAX_VERTICES; i++)
            vertex_mapping[i] = -1;

        // Create new vertices for each unique vertex that needs extrusion
        for (u16 vertex_index = 0; vertex_index < frame->geom.vert_count; vertex_index++) {
            if (!vertex_needs_extrusion[vertex_index])
                continue;

            if (GetCurrentFrame(m)->vertex_count >= MESH_MAX_VERTICES)
                return false;

            u16 new_vertex_index = frame->geom.vert_count++;
            vertex_mapping[vertex_index] = new_vertex_index;

            // Copy vertex properties and offset position along edge normal
            VertexData* old_vertex = GetVertex(frame, vertex_index);
            VertexData* new_vertex = GetVertex(frame, new_vertex_index);

            *new_vertex = *old_vertex;

            SetFlags(new_vertex, VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);
        }

        // Store vertex pairs for the new edges we want to select
        int new_edge_vertex_pairs[MESH_MAX_EDGES][2];
        int new_edge_count = 0;

        // Create new edges for the extruded geometry
        for (int i = 0; i < selected_edge_count; i++) {
            const EdgeData& original_edge = GetCurrentFrame(m)->edges[selected_edges[i]];
            int old_v0 = original_edge.v0;
            int old_v1 = original_edge.v1;
            int new_v0 = vertex_mapping[old_v0];
            int new_v1 = vertex_mapping[old_v1];

            if (new_v0 == -1 || new_v1 == -1)
                continue;

            // Create connecting edges between old and new vertices
            if (GetCurrentFrame(m)->edge_count + 3 >= MESH_MAX_EDGES)
                return false;

            if (GetCurrentFrame(m)->face_count + 2 >= MESH_MAX_FACES)
                return false;

            // GetOrAddEdge(m, old_v0, new_v0, -1);
            // GetOrAddEdge(m, old_v1, new_v1, -1);
            // GetOrAddEdge(m, new_v0, new_v1, -1);

            // Store the vertex pair for the new edge we want to select
            if (new_edge_count < MESH_MAX_EDGES) {
                new_edge_vertex_pairs[new_edge_count][0] = new_v0;
                new_edge_vertex_pairs[new_edge_count][1] = new_v1;
                new_edge_count++;
            }

            // Find the face that contains this edge to inherit its color/opacity and determine orientation
            int face_color = 0; // Default color
            float face_opacity = 1.0f; // Default opacity
            Vec2 face_normal = {0, 0}; // Default normal
            bool edge_reversed = false; // Track if edge direction is reversed in the face
            bool found_face = false;

            for (int face_idx = 0; !found_face && face_idx < GetCurrentFrame(m)->face_count; face_idx++) {
                const FaceData& f = GetCurrentFrame(m)->faces[face_idx];

                // Check if this face contains the edge using the face_vertices array
                for (u16 vertex_index = 0; !found_face && vertex_index < f.vertex_count; vertex_index++) {
                    int v0_idx = f.vertices[vertex_index];
                    int v1_idx = f.vertices[(vertex_index + 1) % f.vertex_count];

                    if (v0_idx == old_v0 && v1_idx == old_v1) {
                        face_color = f.color;
                        face_opacity = f.opacity;
                        edge_reversed = false;
                        found_face = true;
                    } else if (v0_idx == old_v1 && v1_idx == old_v0) {
                        face_color = f.color;
                        face_opacity = f.opacity;
                        edge_reversed = true;
                        found_face = true;
                    }
                }
            }

            FaceData& quad = GetCurrentFrame(m)->faces[GetCurrentFrame(m)->face_count++];
            quad.color = face_color;
            quad.opacity = face_opacity;
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

        Update(m);
        MarkDirty(m);

        // update selection
        ClearSelection();
        for (int i = 0; i < new_edge_count; i++) {
            int v0 = new_edge_vertex_pairs[i][0];
            int v1 = new_edge_vertex_pairs[i][1];
            int edge_index = GetEdge(m, v0, v1);
            assert(edge_index != -1);
            GetCurrentFrame(m)->edges[edge_index].selected = true;
        }

        UpdateSelection(MESH_EDITOR_MODE_EDGE);

#endif

        return true;
    }

    static void ExtrudeSelected() {
        MeshDocument* m = GetMeshDocument();

        if (g_mesh_editor.mode != MESH_EDITOR_MODE_EDGE || GetCurrentFrame(m)->selected_vertex_count <= 0)
            return;

        RecordUndo(m);
        if (!ExtrudeSelectedEdges(m)) {
            CancelUndo();
            return;
        }

        BeginMoveTool();
    }

    static void NewFace() {
#if 0
        if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT)
            return;

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        RecordUndo(m);

        int face_index = -1;
        if ((g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX || g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) && GetCurrentFrame(m)->selected_vertex_count >= 3) {
            face_index = CreateFace(m);
        } else {
            frame->geom.vert_count += 4;
            frame->geom.verts[frame->geom.vert_count - 4] = { .position = { -0.25f, -0.25f }};
            frame->geom.verts[frame->geom.vert_count - 3] = { .position = {  0.25f, -0.25f }};
            frame->geom.verts[frame->geom.vert_count - 2] = { .position = {  0.25f,  0.25f }};
            frame->geom.verts[frame->geom.vert_count - 1] = { .position = { -0.25f,  0.25f }};

            FaceData* f = frame->geom.faces[frame->geom.face_count++];
            f = { .vertex_count=4, .color=0, .opacity=1.0f };
            f.vertices[0] = frame->geom.vert_count - 4;
            f.vertices[1] = frame->geom.vert_count - 3;
            f.vertices[2] = frame->geom.vert_count - 2;
            fvertices[3] = frame->geom.vert_count - 1;

            face_index = frame->geom.face_count-1;
        }

        if (face_index == -1) {
            CancelUndo();
            return;
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);

        ClearSelection();
        SelectFace(face_index, true);
#endif
    }

    // Get total animation duration in frames (accounting for hold)
    static int GetTotalAnimationFrames(MeshDocument* mdoc) {
        int total = 0;
        for (int i = 0; i < mdoc->frame_count; i++)
            total += 1 + mdoc->frames[i].hold;
        return total;
    }

    // Get which frame index to display for a given playback frame (accounting for hold)
    static int GetFrameForPlaybackTime(MeshDocument* mdoc, int playback_frame) {
        int accumulated = 0;
        for (int i = 0; i < mdoc->frame_count; i++) {
            int frame_duration = 1 + mdoc->frames[i].hold;
            if (playback_frame < accumulated + frame_duration)
                return i;
            accumulated += frame_duration;
        }
        return mdoc->frame_count - 1;
    }

    static void UpdateMeshEditor() {
        CheckShortcuts(g_mesh_editor.animation_shortcuts, g_mesh_editor.input);
        CheckShortcuts(g_mesh_editor.shortcuts, g_mesh_editor.input);

        // Update animation playback
        if (g_mesh_editor.is_playing) {
            MeshDocument* mdoc = GetMeshDocument();
            if (mdoc->frame_count > 1) {
                float frame_rate = (float)g_editor.config->GetInt("animation", "frame_rate", 12);
                g_mesh_editor.playback_time += GetFrameTime();

                int total_frames = GetTotalAnimationFrames(mdoc);
                float total_duration = total_frames / frame_rate;

                // Loop animation
                while (g_mesh_editor.playback_time >= total_duration)
                    g_mesh_editor.playback_time -= total_duration;

                int playback_frame = (int)(g_mesh_editor.playback_time * frame_rate);
                int frame_index = GetFrameForPlaybackTime(mdoc, playback_frame);
                SetCurrentFrame(mdoc, frame_index);
            }
        }

        UpdateDefaultState();
    }

    static void DrawSkeleton() {
#if 0
        MeshDocument* m = GetMeshDocument();
        MeshDataImpl* impl = m->impl;
        MeshFrameData* frame = GetCurrentFrame(m);
        SkeletonData* s = impl->skeleton;
        if (!s)
            return;

        BindDepth(0.0f);
        BindMaterial(g_workspace.vertex_material);

        // Determine which bones are used by selected vertices
        bool bone_used[MAX_BONES] = {};
        for (u16 vertex_index=0; vertex_index<GetCurrentFrame(m)->vertex_count; vertex_index++) {
            VertexData* v = GetVertex(frame, vertex_index);
            if (!IsSelected(v)) continue;
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
#endif
    }

    static void DrawXRay() {
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_WEIGHT && g_mesh_editor.xray) {
            DrawSkeleton();
        }
    }


#if 0
    static void DrawTiledMesh() {
        MeshDocument* m = GetMeshData();
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
        MeshVertex v = {};
        v.depth = 0.5f;
        v.opacity = 1.0f;
        v.position = quad_min; v.uv = {u_min, v_min}; AddVertex(builder, v);
        v.position = {quad_max.x, quad_min.y}; v.uv = {u_max, v_min}; AddVertex(builder, v);
        v.position = quad_max; v.uv = {u_max, v_max}; AddVertex(builder, v);
        v.position = {quad_min.x, quad_max.y}; v.uv = {u_min, v_max}; AddVertex(builder, v);
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);

        Mesh* tile_quad = CreateMesh(ALLOCATOR_SCRATCH, builder, nullptr, false);
        Free(builder);

        // 8 surrounding offsets
        Vec2 offsets[8] = {
            {-tile_size.x, -tile_size.y},
            {        0.0f, -tile_size.y},
            { tile_size.x, -tile_size.y},
            {-tile_size.x,         0.0f},
            { tile_size.x,         0.0f},
            {-tile_size.x,  tile_size.y},
            {        0.0f,  tile_size.y},
            { tile_size.x,  tile_size.y},
        };

        BindMaterial(tile_material);
        BindColor(SetAlpha(COLOR_WHITE, 0.5f));
        for (int i = 0; i < 8; i++) {
            DrawMesh(tile_quad, Translate(m->position + offsets[i]));
        }
        BindColor(COLOR_WHITE);
    }
#endif

    static void UpdateTexture() {
        MeshDocument* mdoc = GetMeshDocument();

        Update(mdoc);

        Rasterize(
            mdoc,
            mdoc->current_frame,
            g_mesh_editor.draw.rasterizer,
            g_mesh_editor.draw.pixels,
            Vec2Int{1,1}, 1);

        UpdateTexture(g_mesh_editor.draw.texture, g_mesh_editor.draw.pixels->rgba);

        Free(g_mesh_editor.draw.mesh);

        PushScratch();
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
        Vec2 uv = {
            1.0f / (float)g_mesh_editor.draw.pixels->size.x,
            1.0f / (float)g_mesh_editor.draw.pixels->size.y
        };
        Vec2 st = {
            (mdoc->size.x + 1) / (float)g_mesh_editor.draw.pixels->size.x,
            (mdoc->size.y + 1) / (float)g_mesh_editor.draw.pixels->size.y
        };

        AddVertex(builder, MeshVertex {
            .position = {mdoc->bounds.min.x, mdoc->bounds.min.y},
            .uv = {uv.x, uv.y},
        });
        AddVertex(builder, MeshVertex {
            .position = {mdoc->bounds.max.x, mdoc->bounds.min.y},
            .uv = {st.x, uv.y},
        });
        AddVertex(builder, MeshVertex {
            .position = {mdoc->bounds.max.x, mdoc->bounds.max.y},
            .uv = {st.x, st.y},
        });
        AddVertex(builder, MeshVertex {
            .position = {mdoc->bounds.min.x, mdoc->bounds.max.y},
            .uv = {uv.x, st.y},
        });

        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);
        g_mesh_editor.draw.mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        Free(builder);
        PopScratch();
    }

    static void BuildEditorMesh(MeshDocument* m, bool hide_selected) {
        MeshBuilder* builder = g_editor.mesh_builder;
        MeshFrameData* frame = GetCurrentFrame(m);

        float line_width = STYLE_MESH_EDGE_WIDTH * g_workspace.zoom_ref_scale;
        float selected_line_width = line_width * 2.5f;
        float vertex_size = STYLE_MESH_VERTEX_SIZE * g_workspace.zoom_ref_scale;
        float origin_size = 0.1f * g_workspace.zoom_ref_scale;

        for (u16 edge_index = 0; edge_index < frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            const Vec2& v0 = GetVertex(frame, e->v0)->position + m->position;
            const Vec2& v1 = GetVertex(frame, e->v1)->position + m->position;

            if (HasCurve(e)) {
#if 0
                CurveData& curve = frame->curves[e.curve];
                // Draw curved edge as line segments
                Vec2 control = GetEdgeControlPoint(m, edge_index) + m->position;
                float weight = curve.weight;
                constexpr int segments = 8;
                Vec2 prev = v0;
                for (int s = 1; s <= segments; s++) {
                    float t = (float)s / (float)segments;
                    Vec2 curr = EvalQuadraticBezier(v0, control, v1, t, weight);
                    AddEditorLine(builder, prev, curr, line_width, COLOR_EDGE);
                    prev = curr;
                }
#endif
            } else {
                AddEditorLine(builder, v0, v1, line_width, COLOR_EDGE);
            }
        }

        if (hide_selected) {
            for (u16 i = 0; i < frame->geom.vert_count; i++) {
                const VertexData* v = GetVertex(frame, i);
                if (IsSelected(v)) continue;
                AddEditorSquare(builder, v->position + m->position, vertex_size, COLOR_VERTEX);
            }
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX) {
            for (u16 i = 0; i < frame->geom.vert_count; i++) {
                const VertexData* v = GetVertex(frame, i);
                Color color = IsSelected(v) ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
                AddEditorSquare(builder, v->position + m->position, vertex_size, color);
            }
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_EDGE) {
            for (u16 edge_index = 0; edge_index < frame->geom.edge_count; edge_index++) {
                const EdgeData* e = GetEdge(frame, edge_index);
                if (!IsSelected(e)) continue;
                Vec2 v0 = frame->geom.verts[e->v0].position + m->position;
                Vec2 v1 = frame->geom.verts[e->v1].position + m->position;

                if (HasCurve(e)) {
#if 0
                    CurveData& curve = frame->curves[e.curve];
                    Vec2 control = GetEdgeControlPoint(m, edge_index) + m->position;
                    float weight = curve.weight;
                    constexpr int segments = 8;
                    Vec2 prev = v0;
                    for (int s = 1; s <= segments; s++) {
                        float t = (float)s / (float)segments;
                        Vec2 curr = EvalQuadraticBezier(v0, control, v1, t, weight);
                        AddEditorLine(builder, prev, curr, selected_line_width, COLOR_EDGE_SELECTED);
                        prev = curr;
                    }
#endif
                } else {
                    AddEditorLine(builder, v0, v1, selected_line_width, COLOR_EDGE_SELECTED);
                }
            }
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) {
#if 0
            for (int face_index = 0; face_index < frame->geom.face_count; face_index++) {
                const FaceData* f = GetFace(frame, face_index);
                if (!IsSelected(f)) continue;
                for (u16 edge_index=0; edge_index < f->edge_count; edge_index++) {
                    int v0_idx = f.vertices[edge_index];
                    int v1_idx = f.vertices[(edge_index + 1) % f.vertex_count];
                    Vec2 v0 = frame->geom.verts[v0_idx].position + m->position;
                    Vec2 v1 = frame->geom.verts[v1_idx].position + m->position;

                    int edge_index = GetEdge(m, v0_idx, v1_idx);
                    if (edge_index != -1 && IsEdgeCurved(m, edge_index)) {
                        CurveData& curve = frame->curves[frame->geom.edges[edge_index].curve];
                        Vec2 control = GetEdgeControlPoint(m, edge_index) + m->position;
                        float weight = curve.weight;
                        constexpr int segments = 8;
                        Vec2 prev = v0;
                        for (int s = 1; s <= segments; s++) {
                            float t = (float)s / (float)segments;
                            Vec2 curr = EvalQuadraticBezier(v0, control, v1, t, weight);
                            AddEditorLine(builder, prev, curr, selected_line_width, COLOR_VERTEX_SELECTED);
                            prev = curr;
                        }
                    } else {
                        AddEditorLine(builder, v0, v1, selected_line_width, COLOR_VERTEX_SELECTED);
                    }
                }
            }
            for (int face_index = 0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
                const FaceData& f = frame->geom.faces[face_index];
                Vec2 center = GetFaceCenter(m, face_index) + m->position;
                Color color = f.selected ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
                AddEditorSquare(builder, center, vertex_size, color);
            }
#endif
        } else if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT) {
#if 0
            float outline_size = STYLE_MESH_WEIGHT_OUTLINE_SIZE * g_workspace.zoom_ref_scale;
            float control_size = STYLE_MESH_WEIGHT_SIZE * g_workspace.zoom_ref_scale;
            float stroke_thickness = 0.02f * g_workspace.zoom_ref_scale;

            for (int vi = 0; vi < GetCurrentFrame(m)->vertex_count; vi++) {
                VertexData& v = GetCurrentFrame(m)->vertices[vi];
                Vec2 pos = v.position + m->position;
                float weight = GetVertexWeight(m, vi, g_mesh_editor.weight_bone);

                if (!v.selected) {
                    AddEditorCircleStroke(builder, pos, outline_size, stroke_thickness, SetAlpha(COLOR_BLACK, 0.5f));
                    if (weight > 0.0f) {
                        AddEditorArc(builder, pos, control_size, weight, SetAlpha(COLOR_BLACK, 0.5f));
                    }
                }
            }

            for (int vi = 0; vi < GetCurrentFrame(m)->vertex_count; vi++) {
                VertexData& v = GetCurrentFrame(m)->vertices[vi];
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

            for (int i = 0; i < GetCurrentFrame(m)->vertex_count; i++) {
                const VertexData& v = GetCurrentFrame(m)->vertices[i];
                Color color = v.selected ? COLOR_VERTEX_SELECTED : COLOR_VERTEX;
                AddEditorCircle(builder, v.position + m->position, vertex_size * 0.5f, color);
            }
#endif
        }

        AddEditorCircle(builder, m->position, origin_size, COLOR_ORIGIN);

        if (!g_mesh_editor.mesh)
            g_mesh_editor.mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        else
            UpdateMesh(builder, g_mesh_editor.mesh);
    }

    static void DrawMeshEditor() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        if (g_mesh_editor.dirty) {
            UpdateTexture();
            BuildEditorMesh(m, DoesToolHideSelected());
        }

        // Pixels
        BindColor(COLOR_WHITE);
        BindTexture(g_mesh_editor.draw.texture);
        BindShader(SHADER_EDITOR_TEXTURE);
        BindDepth(0.0f);
        DrawMesh(g_mesh_editor.draw.mesh, Translate(m->position));

        DrawOnionSkin();

        // controls
        BindDepth(0.0f);
        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindTransform(MAT3_IDENTITY);
        DrawMesh(g_mesh_editor.mesh);

        //bool hide_selected = DoesToolHideSelected();

#if 0
        if (frame->edit_texture && frame->edit_width > 0) {
            float dpi = (float)g_editor.atlas.dpi;
            Vec2 texture_size = {
                (float)frame->edit_width / dpi,
                (float)frame->edit_height / dpi
            };
            float padding_world = (float)g_editor.atlas.padding / dpi;
            Vec2 quad_min = edit_bounds.min - Vec2{padding_world, padding_world};
            Vec2 quad_max = quad_min + texture_size;
            Vec2 center = (quad_min + quad_max) * 0.5f;

            // Create material on demand for this frame's texture
            static Material* s_edit_material = nullptr;
            if (!s_edit_material) {
                s_edit_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
            }
            SetTexture(s_edit_material, frame->edit_texture, 0);

            BindColor(COLOR_WHITE);
            BindMaterial(s_edit_material);
            BindDepth(-0.1f);
            DrawMesh(g_workspace.quad_mesh, Translate(m->position + center) * Scale(Vec2{texture_size.x, -texture_size.y}));
        }

        // Keep track of atlas for tiling preview (but don't use it for main rendering)
        AtlasRect* rect = nullptr;
        AtlasData* atlas = FindAtlasForMesh(m->name, &rect);

        // Skip editor overlays when playing animation
        if (g_mesh_editor.is_playing)
            return;

        // Draw tiled copies if tiling preview is enabled
        if (g_mesh_editor.show_tiling) {
            MeshFrameData* tile_frame = GetCurrentFrame(m);
            Vec2 tile_size = {}, quad_min = {}, quad_max = {};
            Material* tile_material = nullptr;
            float u_min = 0, v_min = 0, u_max = 1, v_max = 1;

            if (atlas && rect && atlas->impl->material) {
                // Use atlas geometry for tiling
                GetExportQuadGeometry(atlas, *rect, &quad_min, &quad_max, &u_min, &v_min, &u_max, &v_max);
                tile_size = GetSize(rect->mesh_bounds);
                tile_material = atlas->impl->material;
            } else if (tile_frame->edit_texture && tile_frame->edit_width > 0) {
                // Use per-frame edit texture for tiling
                float dpi = (float)g_editor.atlas.dpi;
                Vec2 texture_size = {
                    (float)tile_frame->edit_width / dpi,
                    (float)tile_frame->edit_height / dpi
                };
                float padding_world = (float)g_editor.atlas.padding / dpi;
                quad_min = tile_frame->edit_bounds.min - Vec2{padding_world, padding_world};
                quad_max = quad_min + texture_size;
                tile_size = GetSize(tile_frame->edit_bounds);
                // Create material on demand for tiling
                static Material* s_tile_material = nullptr;
                if (!s_tile_material) {
                    s_tile_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_TEXTURED_MESH);
                }
                SetTexture(s_tile_material, tile_frame->edit_texture, 0);
                tile_material = s_tile_material;
            }

            if (tile_material)
                DrawTiledMesh();
        }



        if (g_mesh_editor.mode == MESH_EDITOR_MODE_WEIGHT && !hide_selected) {
            DrawSkeleton();
        }



#endif

        if (!DoesToolHideSelected()) {
            DrawXRay();
        }
    }

    static void SubDivide() {
        MeshDocument* m = GetMeshDocument();
        RecordUndo(m);

        u16 selected_edges[MESH_MAX_EDGES];
        u16 selected_edge_count = GetSelectedEdges(m, selected_edges);

        for (int edge_index=0; edge_index<selected_edge_count; edge_index++) {
            int new_vertex = SplitEdge(GetMeshDocument(), selected_edges[edge_index], 0.5f, false);
            SelectVertex(static_cast<u16>(new_vertex), true);
        }

        Update(m);
        UpdateSelection();
        MarkModified(m);
    }

#if 0
    static void ToggleAnchor() {
        BeginSelectTool({.commit= [](const Vec2& position ) {
            MeshDocument* m = GetMeshData();
            RecordUndo(m);

            int anchor_index = HitTestTag(m, position - m->position);
            if (anchor_index == -1)
                AddTag(m, position - m->position);
            else
                RemoveTag(m, anchor_index);

            Update(m);
            MarkDirty(m);
            MarkModified(m);
        }});
    }
#endif

    static void BeginKnifeCut() {
        MeshDocument* m = GetMeshDocument();
        // In face mode with selected faces, restrict knife to selected faces only
        bool restrict_to_selected = (g_mesh_editor.mode == MESH_EDITOR_MODE_FACE) && (GetCurrentFrame(m)->selected_face_count > 0);
        BeginKnifeTool(m, restrict_to_selected);
    }

    static void BeginPenDraw() {
        MeshDocument* m = GetMeshDocument();
        if (!m) return;
        BeginPenTool(m, g_mesh_editor.selection_color, g_mesh_editor.selection_opacity / 10.0f);
    }

    static void BeginAutoCurve() {
        MeshDocument* m = GetMeshDocument();
        if (!m) return;
        BeginAutoCurveTool(m);
    }

    static void SendBackward() {
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
            return;

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        RecordUndo(m);

        for (u16 face_index=1; face_index<frame->geom.face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            FaceData* p = GetFace(frame, face_index-1);
            if (!IsSelected(f) || IsSelected(p))
                continue;

            SwapFace(m, face_index, face_index - 1);
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void BringForward() {
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
            return;

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        RecordUndo(m);

        for (u16 face_index=frame->geom.face_count-2; face_index>0; face_index--) {
            FaceData* f = GetFace(frame, face_index - 1);
            FaceData* n = GetFace(frame, face_index);
            if (!IsSelected(f) || IsSelected(n))
                continue;

            SwapFace(m, face_index, face_index);
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void BringToFront() {
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
            return;

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        RecordUndo(m);

        int write_index = 0;
        for (u16 read_index = 0; read_index < frame->geom.face_count; read_index++) {
            if (!IsSelected(&frame->geom.faces[read_index])) {
                if (write_index != read_index) {
                    FaceData temp = frame->geom.faces[write_index];
                    frame->geom.faces[write_index] = frame->geom.faces[read_index];
                    frame->geom.faces[read_index] = temp;
                }
                write_index++;
            }
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void SendToBack() {
        if (g_mesh_editor.mode != MESH_EDITOR_MODE_FACE)
            return;

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        RecordUndo(m);

        int write_index = frame->geom.face_count - 1;
        for (int read_index = frame->geom.face_count - 1; read_index >= 0; read_index--) {
            if (!IsSelected(&frame->geom.faces[read_index])) {
                if (write_index != read_index) {
                    FaceData temp = frame->geom.faces[write_index];
                    frame->geom.faces[write_index] = frame->geom.faces[read_index];
                    frame->geom.faces[read_index] = temp;
                }
                write_index--;
            }
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
    }

    static void ToggleXRay() {
        g_mesh_editor.xray = !g_mesh_editor.xray;
    }

    static void CommitParentTool(const Vec2& position) {
        Document* hit_asset = HitTestAssets(position);
        if (!hit_asset)
            return;

        MeshDocument* mdoc = GetMeshDocument();

        if (hit_asset->def->type == ASSET_TYPE_SKELETON) {
            RecordUndo(mdoc);
            mdoc->skeleton = static_cast<SkeletonDocument*>(hit_asset);
            mdoc->skeleton_name = hit_asset->name;
            MarkModified(mdoc);
        } else if (hit_asset->def->type == ASSET_TYPE_ATLAS) {
            AtlasDocument* adoc = static_cast<AtlasDocument*>(hit_asset);

            RecordUndo(mdoc);
            mdoc->atlas = adoc;
            MarkModified(mdoc);

            // Add mesh to atlas and render it (handles multi-frame meshes automatically)
            AtlasRect* rect = FindRectForMesh(adoc, mdoc->name);
            if (!rect) {
                rect = AllocateRect(adoc, mdoc);
            }
            if (rect) {
                RenderMeshToAtlas(adoc, mdoc, *rect);
                MarkModified(adoc);
                AddNotification(NOTIFICATION_TYPE_INFO, "'%s' added to '%s'", mdoc->name->value, adoc->name->value);
            }
        }
    }

    static void BeginParentTool() {
        BeginSelectTool({.commit=CommitParentTool});
    }

    static void FlipHorizontal() {
#if 0
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        if (frame->selected_vertex_count == 0)
            return;

        RecordUndo(m);

        // Flip selected vertices horizontally around their center
        for (u16 vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++) {
            VertexData* v = GetVertex(frame, vertex_index);
            if (!IsSelected(v)) continue;
            v->position.x = g_mesh_editor.selection_center.x - (v->position.x - g_mesh_editor.selection_center.x);
        }

        // Reverse winding order of selected faces
        for (int face_index=0; face_index<GetCurrentFrame(m)->face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            if (!IsSelected(f)) continue;

            for (int i=0; i<f.vertex_count / 2; i++) {
                int temp = f.vertices[i];
                f.vertices[i] = f.vertices[f.vertex_count - 1 - i];
                f.vertices[f.vertex_count - 1 - i] = temp;
            }
        }

        Update(m);
        MarkDirty(m);
        MarkModified(m);
#endif
    }

    static void CopySelection() {
#if 0
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        if (frame->selected_face_count == 0)
            return;

        auto& clip = g_mesh_editor.clipboard;
        clip.vertex_count = 0;
        clip.face_count = 0;

        // Build vertex map: old index -> new clipboard index
        int vertex_map[MESH_MAX_VERTICES];
        for (int i = 0; i < MESH_MAX_VERTICES; i++)
            vertex_map[i] = -1;

        // First pass: collect vertices used by selected faces
        for (int face_index = 0; face_index < frame->geom.face_count; face_index++) {
            FaceData& f = frame->geom.faces[face_index];
            if (!f.selected) continue;

            for (int vi = 0; vi < f.vertex_count; vi++) {
                int old_idx = f.vertices[vi];
                if (vertex_map[old_idx] == -1) {
                    vertex_map[old_idx] = clip.vertex_count;
                    clip.vertices[clip.vertex_count++] = frame->geom.verts[old_idx];
                }
            }
        }

        // Calculate center of copied geometry
        clip.center = VEC2_ZERO;
        for (int i = 0; i < clip.vertex_count; i++)
            clip.center = clip.center + clip.vertices[i].position;
        if (clip.vertex_count > 0)
            clip.center = clip.center * (1.0f / clip.vertex_count);

        // Second pass: copy faces with remapped vertex indices
        for (int face_index = 0; face_index < frame->geom.face_count; face_index++) {
            FaceData& f = frame->geom.faces[face_index];
            if (!f.selected) continue;

            FaceData& nf = clip.faces[clip.face_count++];
            nf = f;
            nf.selected = false;

            for (int vi = 0; vi < f.vertex_count; vi++)
                nf.vertices[vi] = vertex_map[f.vertices[vi]];
        }

        clip.has_content = true;
#endif
    }

    static void PasteSelection() {
#if 0
        auto& clip = g_mesh_editor.clipboard;
        if (!clip.has_content)
            return;

        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);

        if (frame->geom.face_count + clip.face_count > MESH_MAX_FACES)
            return;
        if (frame->geom.vert_count + clip.vertex_count > MESH_MAX_VERTICES)
            return;

        RecordUndo(m);

        // Calculate paste offset: paste at mouse position relative to clipboard center
        Vec2 paste_offset = g_workspace.mouse_world_position - m->position - clip.center;

        // Clear current selection
        for (int i = 0; i < frame->geom.vert_count; i++)
            frame->geom.verts[i].selected = false;
        for (int i = 0; i < frame->geom.face_count; i++)
            frame->geom.faces[i].selected = false;

        // Add vertices with offset
        int vertex_base = frame->geom.vert_count;
        for (int i = 0; i < clip.vertex_count; i++) {
            VertexData& v = frame->geom.verts[frame->geom.vert_count++];
            v = clip.vertices[i];
            v.position = v.position + paste_offset;
            v.selected = true;
            v.ref_count = 0;  // Will be recalculated
        }

        // Add faces with remapped vertex indices
        for (int i = 0; i < clip.face_count; i++) {
            FaceData& nf = frame->geom.faces[frame->geom.face_count++];
            nf = clip.faces[i];
            nf.selected = true;

            for (int vi = 0; vi < nf.vertex_count; vi++)
                nf.vertices[vi] = nf.vertices[vi] + vertex_base;
        }

        MarkDirty(m);
        Update(m);
        UpdateSelection(MESH_EDITOR_MODE_FACE);
        MarkModified(m);
#endif
    }

    static void DuplicateSelected() {
#if 0
        MeshDocument* m = GetMeshDocument();
        if (GetCurrentFrame(m)->face_count + GetCurrentFrame(m)->selected_face_count > MESH_MAX_FACES)
            return;
        if (GetCurrentFrame(m)->vertex_count + GetCurrentFrame(m)->selected_vertex_count > MESH_MAX_VERTICES)
            return;

        RecordUndo(m);

        int old_face_count = GetCurrentFrame(m)->face_count;
        int old_vertex_count = GetCurrentFrame(m)->vertex_count;

        int vertex_map[MESH_MAX_VERTICES];
        for (int vertex_index=0; vertex_index<old_vertex_count; vertex_index++) {
            VertexData& v = GetCurrentFrame(m)->vertices[vertex_index];
            if (!v.selected) continue;
            vertex_map[vertex_index] = GetCurrentFrame(m)->vertex_count;
            GetCurrentFrame(m)->vertices[GetCurrentFrame(m)->vertex_count++] = v;
        }

        for (int face_index=0; face_index<old_face_count; face_index++) {
            FaceData& f = GetCurrentFrame(m)->faces[face_index];
            if (!f.selected) continue;
            int new_face_index = GetCurrentFrame(m)->face_count++;
            FaceData& nf = GetCurrentFrame(m)->faces[new_face_index];
            nf = f;
            f.selected = false;
            nf.selected = true;

            for (int vertex_index=0; vertex_index<f.vertex_count; vertex_index++)
                nf.vertices[vertex_index] = vertex_map[nf.vertices[vertex_index]];
        }

        MarkDirty(m);
        Update(m);
        UpdateSelection(MESH_EDITOR_MODE_FACE);
        MarkModified(m);
        BeginMoveTool();
#endif
    }

    static Mesh* BuildEdgeMesh(MeshFrameData* frame, Mesh* existing) {
        PushScratch();
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MESH_MAX_VERTICES * 4, MESH_MAX_EDGES * 6);

        float line_width = 0.02f * g_workspace.zoom_ref_scale;

        for (int i = 0; i < frame->geom.edge_count; i++) {
            const EdgeData& e = frame->geom.edges[i];
            Vec2 v0 = frame->geom.verts[e.v0].position;
            Vec2 v1 = frame->geom.verts[e.v1].position;
            Vec2 dir = Normalize(v1 - v0);
            Vec2 n = Perpendicular(dir) * line_width;

            u16 base = GetVertexCount(builder);
            AddVertex(builder, v0 - n);
            AddVertex(builder, v0 + n);
            AddVertex(builder, v1 + n);
            AddVertex(builder, v1 - n);
            AddTriangle(builder, base, base + 1, base + 2);
            AddTriangle(builder, base, base + 2, base + 3);
        }

        Mesh* result;
        if (!existing)
            result = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        else {
            UpdateMesh(builder, existing);
            result = existing;
        }

        PopScratch();
        return result;
    }

    static void SetPrevFrame() {
        MeshDocument* mdoc = GetMeshDocument();
        if (mdoc->frame_count <= 1) return;
        int new_frame = (mdoc->current_frame - 1 + mdoc->frame_count) % mdoc->frame_count;
        SetCurrentFrame(mdoc, new_frame);
        RefreshMeshEditorSelection();
    }

    static void SetNextFrame() {
        MeshDocument* mdoc = GetMeshDocument();
        if (mdoc->frame_count <= 1) return;
        int new_frame = (mdoc->current_frame + 1) % mdoc->frame_count;
        SetCurrentFrame(mdoc, new_frame);
        RefreshMeshEditorSelection();
    }

    static void InsertFrameAfter() {
        MeshDocument* mdoc = GetMeshDocument();
        if (mdoc->frame_count >= MESH_MAX_FRAMES) return;
        RecordUndo(mdoc);
        AddFrame(mdoc, mdoc->current_frame);
        MarkModified();
    }

    static void DeleteCurrentFrame() {
        MeshDocument* mdoc = GetMeshDocument();
        if (mdoc->frame_count <= 1) return;

        RecordUndo(mdoc);
        DeleteFrame(mdoc, mdoc->current_frame);
        RefreshMeshEditorSelection();
        MarkModified();
    }

    static void TogglePlayAnimation() {
        g_mesh_editor.is_playing = !g_mesh_editor.is_playing;
        g_mesh_editor.playback_time = 0.0f;
    }

    static void IncHoldFrame() {
        MeshDocument* m = GetMeshDocument();
        GetCurrentFrame(m)->hold++;
        MarkModified();
    }

    static void DecHoldFrame() {
        MeshDocument* m = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(m);
        if (frame->hold > 0) {
            frame->hold--;
            MarkModified();
        }
    }

    static void ToggleOnionSkin() {
        g_mesh_editor.onion_skin_enabled = !g_mesh_editor.onion_skin_enabled;
    }

    static void Dopesheet() {
        MeshDocument* mdoc = GetMeshDocument();
        if (mdoc->frame_count < 2 && GetCurrentFrame(mdoc)->hold <= 0)
            return;

        BeginContainer({.padding=EdgeInsetsAll(4), .color=COLOR_WHITE_2PCT, .border{.radius=STYLE_OVERLAY_CONTENT_BORDER_RADIUS}});
        BeginRow();

        for (int frame_index = 0; frame_index < mdoc->frame_count; frame_index++) {
            MeshFrameData* f = &mdoc->frames[frame_index];
            BeginContainer({
                .width=FRAME_SIZE_X * (1 + f->hold) + FRAME_BORDER_SIZE * 2,
                .height=FRAME_SIZE_Y + FRAME_BORDER_SIZE * 2,
                .margin=EdgeInsetsLeft(-2),
                .color = frame_index == mdoc->current_frame
                    ? DOPESHEET_SELECTED_FRAME_COLOR
                    : FRAME_COLOR,
                .border = {.radius=6, .width=FRAME_BORDER_SIZE, .color=FRAME_BORDER_COLOR}
            });
            BeginContainer({.align=ALIGN_BOTTOM_LEFT, .margin=EdgeInsetsBottomLeft(DOPESHEET_FRAME_DOT_OFFSET_Y, DOPESHEET_FRAME_DOT_OFFSET_X)});
            Container({.width=DOPESHEET_FRAME_DOT_SIZE, .height=DOPESHEET_FRAME_DOT_SIZE, .color=DOPESHEET_FRAME_DOT_COLOR});
            EndContainer();
            EndContainer();
        }

        EndRow();
        EndContainer();
    }

    // Draw onion skin overlays
    static void DrawOnionSkin() {
        MeshDocument* mdoc = GetMeshDocument();

        if (!g_mesh_editor.onion_skin_enabled || mdoc->frame_count <= 1)
            return;

        // Rebuild edge meshes if frame changed
        if (g_mesh_editor.cached_frame != mdoc->current_frame) {
            g_mesh_editor.cached_frame = mdoc->current_frame;

            int prev_frame = (mdoc->current_frame - 1 + mdoc->frame_count) % mdoc->frame_count;
            if (prev_frame != mdoc->current_frame) {
                MeshFrameData* pf = &mdoc->frames[prev_frame];
                g_mesh_editor.prev_frame_mesh = BuildEdgeMesh(pf, g_mesh_editor.prev_frame_mesh);
            }

            int next_frame = (mdoc->current_frame + 1) % mdoc->frame_count;
            if (next_frame != mdoc->current_frame) {
                MeshFrameData* nf = &mdoc->frames[next_frame];
                g_mesh_editor.next_frame_mesh = BuildEdgeMesh(nf, g_mesh_editor.next_frame_mesh);
            }
        }

        // Draw prev frame edges in red
        int prev_frame = (mdoc->current_frame - 1 + mdoc->frame_count) % mdoc->frame_count;
        if (prev_frame != mdoc->current_frame && g_mesh_editor.prev_frame_mesh) {
            BindColor(COLOR_RED);
            BindMaterial(g_workspace.shaded_material);
            DrawMesh(g_mesh_editor.prev_frame_mesh, Translate(mdoc->position));
        }

        // Draw next frame edges in green
        int next_frame = (mdoc->current_frame + 1) % mdoc->frame_count;
        if (next_frame != mdoc->current_frame && g_mesh_editor.next_frame_mesh) {
            BindColor(COLOR_GREEN);
            BindMaterial(g_workspace.shaded_material);
            DrawMesh(g_mesh_editor.next_frame_mesh, Translate(mdoc->position));
        }
    }

    static void BeginMeshEditor(Document* a) {
        g_mesh_editor.mesh_data = static_cast<MeshDocument*>(a);
        g_workspace.vtable = {
            .allow_text_input = MeshViewAllowTextInput
        };

        PushInputSet(g_mesh_editor.input);

        SelectAll();

        g_mesh_editor.mode = MESH_EDITOR_MODE_VERTEX;
        g_mesh_editor.weight_bone = -1;
        g_mesh_editor.dirty = true;
        g_mesh_editor.cached_frame = -1;
        g_mesh_editor.is_playing = false;
        g_mesh_editor.playback_time = 0.0f;

        SetFocus(CANVAS_ID_OVERLAY, MESH_EDITOR_ID_EXPAND);
    }

    static void EndMeshEditor() {
        g_mesh_editor.mesh_data = nullptr;
        PopInputSet();

        // Clean up animation state
        Free(g_mesh_editor.prev_frame_mesh);
        Free(g_mesh_editor.next_frame_mesh);
        g_mesh_editor.prev_frame_mesh = nullptr;
        g_mesh_editor.next_frame_mesh = nullptr;
        g_mesh_editor.cached_frame = -1;
        g_mesh_editor.is_playing = false;
    }

    void ShutdownMeshEditor() {
        Free(g_mesh_editor.draw.mesh);
        Free(g_mesh_editor.draw.texture);
        Free(g_mesh_editor.draw.pixels);
        Free(g_mesh_editor.prev_frame_mesh);
        Free(g_mesh_editor.next_frame_mesh);

        g_mesh_editor = {};
    }

    static Shortcut g_mesh_editor_shortcuts[] = {
        { KEY_A, false, false, false, SelectAll, "Select all" },
        { KEY_G, false, false, false, BeginMoveTool, "Move" },
        { KEY_R, false, false, false, BeginRotateTool, "Rotate" },
        { KEY_S, false, false, false, BeginScaleTool, "Scale" },
        { KEY_C, false, false, false, BeginCurveToolFromSelection, "Curve" },
        { KEY_C, false, true, true, CenterMesh, "Center mesh" },
        { KEY_D, false, true, false, DuplicateSelected, "Duplicate" },
        { KEY_C, false, true, false, CopySelection, "Copy" },
        { KEY_V, false, true, false, PasteSelection, "Paste" },
        { KEY_S, false, false, true, SubDivide, "Subdivide" },
        { KEY_C, false, false, true, BeginAutoCurve, "Auto curve" },
        { KEY_X, false, false, false, DissolveSelected, "Delete" },

        { KEY_R, false, false, true, FlipHorizontal, "Flip horiztonal" },
        { KEY_F, false, false, true, NewFace, "New face" },

        { KEY_V, false, false, true, BeginKnifeCut, "Knife tool" },
        { KEY_N, false, false, false, BeginPenDraw, "Pen tool" },
        { KEY_E, false, true, false, ExtrudeSelected, "Extrude tool" },
        { KEY_P, false, false, false, BeginParentTool, "Parent tool" },

        { KEY_V, false, false, false, InsertVertex, "Add vertex" },
        { KEY_1, false, false, false, SetVertexMode, "Vertex mode" },
        { KEY_2, false, false, false, SetEdgeMode, "Edge mode" },
        { KEY_3, false, false, false, SetFaceMode, "Face mode" },
        { KEY_4, false, false, false, SetWeightMode, "Weight mode" },
        { KEY_X, true, false, false, ToggleXRay, "Toggle X-ray" },

        { KEY_LEFT_BRACKET, false, true, false, SendToBack, "Send to back" },
        { KEY_LEFT_BRACKET, false, false, false, SendBackward, "Send backward" },
        { KEY_RIGHT_BRACKET, false, false, false, BringForward, "Bring forward" },
        { KEY_RIGHT_BRACKET, false, true, false, BringToFront, "Bring to front" },

        { INPUT_CODE_NONE }
    };

    static Shortcut g_animation_shortcuts[] = {
        { KEY_Q, false, false, false, SetPrevFrame, "Prev frame" },
        { KEY_E, false, false, false, SetNextFrame, "Next frame" },
        { KEY_O, false, false, false, InsertFrameAfter, "Insert frame after" },
        { KEY_X, false, false, true, DeleteCurrentFrame, "Delete frame" },
        { KEY_H, false, false, false, IncHoldFrame, "Add hold frame" },
        { KEY_H, true, false, false, DecHoldFrame, "Remove hold frame" },
        { KEY_O, true, false, false, ToggleOnionSkin, "Toggle onion skin" },
        { KEY_SPACE, false, false, false, TogglePlayAnimation, "Play/Pause animation" },
        { INPUT_CODE_NONE }
    };

    static void OpenMeshEditorContextMenu() {
        MeshDocument* m = GetMeshDocument();
        if (g_mesh_editor.mode == MESH_EDITOR_MODE_VERTEX) {
            bool any_selected = GetCurrentFrame(m)->selected_vertex_count;
            OpenContextMenuAtMouse({
               .title="Vertex",
               .items = {
                   { "Delete", DissolveSelected, any_selected },
                   { "Subdivide", SubDivide, any_selected },
               },
               .item_count = 4
           });
        }
    }

    static void MeshEditorHelp() {
        HelpGroup("Mesh", g_mesh_editor_shortcuts);
        HelpGroup("Animation", g_animation_shortcuts);
    }

    void InitMeshEditor() {
        g_mesh_editor.color_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);

        g_mesh_editor.input = CreateInputSet(ALLOCATOR_DEFAULT);
        EnableModifiers(g_mesh_editor.input);
        EnableButton(g_mesh_editor.input, MOUSE_LEFT);
        EnableButton(g_mesh_editor.input, MOUSE_LEFT_DOUBLE_CLICK);

        g_mesh_editor.shortcuts = g_mesh_editor_shortcuts;
        g_mesh_editor.animation_shortcuts = g_animation_shortcuts;
        EnableButton(g_mesh_editor.input, KEY_Q);
        EnableButton(g_mesh_editor.input, KEY_E);
        EnableButton(g_mesh_editor.input, KEY_O);
        EnableButton(g_mesh_editor.input, KEY_SPACE);
        EnableButton(g_mesh_editor.input, KEY_H);
        EnableShortcuts(g_mesh_editor_shortcuts, g_mesh_editor.input);
        EnableShortcuts(g_animation_shortcuts, g_mesh_editor.input);
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

        g_mesh_editor.draw.pixels = CreatePixelData(ALLOCATOR_DEFAULT, Vec2Int{g_editor.atlas.size, g_editor.atlas.size});
        g_mesh_editor.draw.rasterizer = CreateRasterizer(ALLOCATOR_DEFAULT, g_mesh_editor.draw.pixels);
        g_mesh_editor.draw.texture = CreateTexture(
            ALLOCATOR_DEFAULT,
            g_editor.atlas.size,
            g_editor.atlas.size,
            TEXTURE_FORMAT_RGBA8,
            GetName("mesh_editor"),
            TEXTURE_FILTER_NEAREST);
    }

    static void RenameMesh(Document* doc, const Name* new_name) {
        MeshDocument* mdoc = static_cast<MeshDocument*>(doc);
        if (mdoc->atlas) {
            AtlasRect* rect = FindRectForMesh(mdoc->atlas, doc->name);
            if (rect) {
                rect->mesh_name = new_name;
                MarkModified(mdoc->atlas);
            }
        }
    }

    static void MeshUndoRedo(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_MESH);
        MeshDocument* mdoc = static_cast<MeshDocument*>(doc);
        MarkDirty(mdoc);

        if (mdoc->editing)
            UpdateSelection();
    }

    void UpdateGeometry() {
        MeshDocument* mdoc = GetMeshDocument();
        MeshFrameData* frame = GetCurrentFrame(mdoc);
        Geometry& g = g_mesh_editor.geom;
        g = frame->geom;

        // todo: expand curves

        // todo: update the editor mesh
    }


    void InitMeshEditor(MeshDocument* m) {
        m->vtable.undo_redo = MeshUndoRedo;
        m->vtable.editor_begin = BeginMeshEditor;
        m->vtable.editor_end = EndMeshEditor;
        m->vtable.editor_draw = DrawMeshEditor;
        m->vtable.editor_update = UpdateMeshEditor;
        m->vtable.editor_bounds = GetMeshEditorBounds;
        m->vtable.editor_overlay = MeshEditorOverlay;
        m->vtable.editor_help = MeshEditorHelp;
        m->vtable.editor_context_menu = OpenMeshEditorContextMenu;
        m->vtable.editor_rename = RenameMesh;
    }
}
