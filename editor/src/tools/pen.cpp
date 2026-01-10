//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {
    
    constexpr float PEN_HIT_TOLERANCE = 0.25f;
    constexpr int PEN_MAX_POINTS = 64;

    struct PenPoint {
        Vec2 position;
        int existing_vertex;  // >= 0 if snapped to existing vertex, -1 if new point
    };

    struct PenTool {
        MeshDocument* mesh;
        PenPoint points[PEN_MAX_POINTS];
        int point_count;

        // Hover state for visual feedback
        bool hovering_first_point;
        bool hovering_existing_vertex;
        int hover_vertex_index;
        Vec2 hover_snap_position;
        bool hovering_edge;
        int hover_edge_index;
        float hover_edge_t;
        bool snapping_to_grid;
        Vec2 grid_snap_position;

        int color;
        float opacity;
    };

    static PenTool g_pen_tool = {};

    static void EndPenTool(bool commit);

    static void CommitPenFace() {
#if 0
        MeshDocument* m = g_pen_tool.mesh;
        MeshFrameData* frame = GetCurrentFrame(m);

        if (g_pen_tool.point_count < 3)
            return;

        if (frame->face_count >= MESH_MAX_FACES)
            return;

        // Create vertices for new points (reuse existing where snapped)
        int vertex_indices[PEN_MAX_POINTS];
        float default_edge_size = g_config->GetFloat("mesh", "default_edge_size", 1.0f);

        for (int i = 0; i < g_pen_tool.point_count; i++) {
            PenPoint& pt = g_pen_tool.points[i];

            if (pt.existing_vertex >= 0) {
                // Reuse existing vertex
                vertex_indices[i] = pt.existing_vertex;
            } else {
                // Create new vertex
                if (frame->vertex_count >= MESH_MAX_VERTICES)
                    return;

                int new_index = frame->vertex_count++;
                VertexData* v = frame->vertices + new_index;
                v = {};
                v->position = pt.position;
                SetFlags(v, VERTEX_FLAG_NONE, VERTEX_FLAG_SELECTED);

                vertex_indices[i] = new_index;
                pt.existing_vertex = -2;
            }
        }

        // Calculate signed area to determine winding order
        // Positive = counter-clockwise, negative = clockwise
        float signed_area = 0.0f;
        for (int i = 0; i < g_pen_tool.point_count; i++) {
            Vec2 v0 = frame->vertices[vertex_indices[i]].position;
            Vec2 v1 = frame->vertices[vertex_indices[(i + 1) % g_pen_tool.point_count]].position;
            signed_area += (v1.x - v0.x) * (v1.y + v0.y);
        }

        // Create face preserving user's click order
        int face_index = frame->face_count++;
        FaceData* f = frame->faces + face_index;
        f->vertex_count = g_pen_tool.point_count;
        f->color = g_pen_tool.color;
        f->opacity = g_pen_tool.opacity;
        SetFlags(f, FACE_FLAG_NONE, FACE_FLAG_SELECTED);

        // If clockwise (negative area), reverse the order to make it counter-clockwise
        if (signed_area > 0) {
            for (int i = 0; i < g_pen_tool.point_count; i++)
                f.vertices[i] = vertex_indices[g_pen_tool.point_count - 1 - i];
        } else {
            for (int i = 0; i < g_pen_tool.point_count; i++)
                f.vertices[i] = vertex_indices[i];
        }

        Update(m);

        // Infer bone weights for newly created vertices from their neighbors
        for (int i = 0; i < g_pen_tool.point_count; i++) {
            if (g_pen_tool.points[i].existing_vertex == -2)
                InferVertexWeightsFromNeighbors(m, vertex_indices[i]);
        }

        MarkDirty(m);
#endif
    }

    static void EndPenTool(bool commit) {
        if (commit && g_pen_tool.point_count >= 3) {
            RecordUndo(g_pen_tool.mesh);
            CommitPenFace();
            MarkModified(g_pen_tool.mesh);
        }

        g_pen_tool.mesh = nullptr;
        EndTool();
    }

    static void DrawPenTool() {
        MeshDocument* m = g_pen_tool.mesh;

        BindMaterial(g_workspace.vertex_material);

        // Draw edges between consecutive points
        BindColor(COLOR_VERTEX_SELECTED);
        for (int i = 0; i < g_pen_tool.point_count - 1; i++) {
            DrawDashedLine(
                g_pen_tool.points[i].position + m->position,
                g_pen_tool.points[i + 1].position + m->position);
        }

        // Draw preview line from last point to mouse cursor
        if (g_pen_tool.point_count > 0) {
            Vec2 last_world = g_pen_tool.points[g_pen_tool.point_count - 1].position + m->position;
            Vec2 target = g_workspace.mouse_world_position;

            // Snap preview line to existing vertex, first point, edge, or grid
            if (g_pen_tool.hovering_first_point) {
                target = g_pen_tool.points[0].position + m->position;
            } else if (g_pen_tool.hovering_existing_vertex) {
                target = g_pen_tool.hover_snap_position + m->position;
            } else if (g_pen_tool.hovering_edge) {
                target = g_pen_tool.hover_snap_position + m->position;
            } else if (g_pen_tool.snapping_to_grid) {
                target = g_pen_tool.grid_snap_position + m->position;
            }

            BindColor(SetAlpha(COLOR_VERTEX_SELECTED, 0.5f));
            DrawDashedLine(last_world, target);
        }

        // Draw closing line preview (from last point to first) when >= 3 points
        if (g_pen_tool.point_count >= 3 && !g_pen_tool.hovering_first_point) {
            BindColor(SetAlpha(COLOR_EDGE, 0.3f));
            DrawDashedLine(
                g_pen_tool.points[g_pen_tool.point_count - 1].position + m->position,
                g_pen_tool.points[0].position + m->position);
        }

        // Draw all placed vertices
        BindColor(COLOR_VERTEX_SELECTED);
        for (int i = 0; i < g_pen_tool.point_count; i++) {
            DrawVertex(g_pen_tool.points[i].position + m->position);
        }

        // Highlight first point when hovering (close indicator)
        if (g_pen_tool.hovering_first_point) {
            BindColor(COLOR_GREEN);
            DrawVertex(g_pen_tool.points[0].position + m->position);
        }

        // Highlight snap target vertex
        if (g_pen_tool.hovering_existing_vertex && !g_pen_tool.hovering_first_point) {
            BindColor(COLOR_GREEN);
            DrawVertex(g_pen_tool.hover_snap_position + m->position);
        }

        // Highlight snap point on edge
        if (g_pen_tool.hovering_edge) {
            BindColor(COLOR_GREEN);
            DrawVertex(g_pen_tool.hover_snap_position + m->position);
        }
    }

    static void UpdatePenTool() {
        MeshDocument* m = g_pen_tool.mesh;
        MeshFrameData* frame = GetCurrentFrame(m);
        Vec2 mouse_local = g_workspace.mouse_world_position - m->position;

        // Cancel
        if (WasButtonPressed(KEY_ESCAPE)) {
            EndPenTool(false);
            return;
        }

        // Commit (Enter key)
        if (WasButtonPressed(KEY_ENTER)) {
            if (g_pen_tool.point_count >= 3) {
                EndPenTool(true);
            }
            return;
        }

        // Undo last point (Right-click)
        if (WasButtonPressed(MOUSE_RIGHT)) {
            if (g_pen_tool.point_count > 0) {
                g_pen_tool.point_count--;
            }
            return;
        }

        // Hover detection for visual feedback
        // Check if hovering over first point (to close polygon)
        g_pen_tool.hovering_first_point = false;
        if (g_pen_tool.point_count >= 3) {
            if (HitTestVertex(g_pen_tool.points[0].position + m->position, g_workspace.mouse_world_position, PEN_HIT_TOLERANCE)) {
                g_pen_tool.hovering_first_point = true;
            }
        }

        // Check if hovering over existing mesh vertex (for snapping)
        g_pen_tool.hovering_existing_vertex = false;
        g_pen_tool.hover_vertex_index = -1;
        if (!g_pen_tool.hovering_first_point) {
            int vertex_index = HitTestVertex(m, g_workspace.mouse_world_position, PEN_HIT_TOLERANCE);
            if (vertex_index >= 0) {
                g_pen_tool.hovering_existing_vertex = true;
                g_pen_tool.hover_vertex_index = vertex_index;
                g_pen_tool.hover_snap_position = GetVertex(GetCurrentFrame(m), static_cast<u16>(vertex_index))->position;
            }
        }

        // Check if hovering over existing edge (for snapping)
        g_pen_tool.hovering_edge = false;
        g_pen_tool.hover_edge_index = -1;
        if (!g_pen_tool.hovering_first_point && !g_pen_tool.hovering_existing_vertex) {
            float edge_t = 0.0f;
            int edge_index = HitTestEdge(m, g_workspace.mouse_world_position, &edge_t, PEN_HIT_TOLERANCE);
            if (edge_index >= 0) {
                g_pen_tool.hovering_edge = true;
                g_pen_tool.hover_edge_index = edge_index;
                g_pen_tool.hover_edge_t = edge_t;
                g_pen_tool.hover_snap_position = GetEdgePoint(frame, static_cast<u16>(edge_index), edge_t);
            }
        }

        // Check for grid snapping (always pixel snap, Ctrl for coarser grid)
        g_pen_tool.snapping_to_grid = false;
        if (!g_pen_tool.hovering_first_point && !g_pen_tool.hovering_existing_vertex && !g_pen_tool.hovering_edge) {
            g_pen_tool.snapping_to_grid = true;
            Vec2 world_pos = g_workspace.mouse_world_position;
            g_pen_tool.grid_snap_position = (IsCtrlDown(GetInputSet()) ? SnapToGrid(world_pos) : SnapToPixelGrid(world_pos)) - m->position;
        }

        // Add point (Left-click)
        if (WasButtonPressed(MOUSE_LEFT)) {
            // Case 1: Clicking on first point to close polygon
            if (g_pen_tool.hovering_first_point) {
                EndPenTool(true);
                return;
            }

            // Prevent clicking same point twice
            if (g_pen_tool.point_count > 0) {
                Vec2 last_pos = g_pen_tool.points[g_pen_tool.point_count - 1].position;
                if (Length(mouse_local - last_pos) < 0.001f) {
                    return;
                }
            }

            // Check capacity
            if (g_pen_tool.point_count >= PEN_MAX_POINTS) {
                return;
            }

            // Case 2: Snapping to existing vertex
            if (g_pen_tool.hovering_existing_vertex) {
                g_pen_tool.points[g_pen_tool.point_count++] = {
                    .position = g_pen_tool.hover_snap_position,
                    .existing_vertex = g_pen_tool.hover_vertex_index
                };
                return;
            }

            // Case 3: Snapping to edge (creates new point on edge)
            if (g_pen_tool.hovering_edge) {
                g_pen_tool.points[g_pen_tool.point_count++] = {
                    .position = g_pen_tool.hover_snap_position,
                    .existing_vertex = -1
                };
                return;
            }

            // Case 4: New point in empty space (always pixel snap, Ctrl for coarser)
            Vec2 world_pos = m->position + mouse_local;
            Vec2 new_position = (IsCtrlDown(GetInputSet()) ? SnapToGrid(world_pos) : SnapToPixelGrid(world_pos)) - m->position;
            g_pen_tool.points[g_pen_tool.point_count++] = {
                .position = new_position,
                .existing_vertex = -1
            };
        }
    }

    void BeginPenTool(MeshDocument* mesh, int color, float opacity) {
        static ToolVtable vtable = {
            .update = UpdatePenTool,
            .draw = DrawPenTool
        };

        BeginTool({
            .type = TOOL_TYPE_SELECT,
            .vtable = vtable,
            .input = g_workspace.input_tool,
            .hide_selected = false
        });

        g_pen_tool.mesh = mesh;
        g_pen_tool.point_count = 0;
        g_pen_tool.hovering_first_point = false;
        g_pen_tool.hovering_existing_vertex = false;
        g_pen_tool.hover_vertex_index = -1;
        g_pen_tool.hovering_edge = false;
        g_pen_tool.hover_edge_index = -1;
        g_pen_tool.color = color;
        g_pen_tool.opacity = opacity;

        SetSystemCursor(SYSTEM_CURSOR_SELECT);
    }
}
