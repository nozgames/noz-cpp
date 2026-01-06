//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

struct CurveToolEdge {
    int edge_index;
    Vec2 original_offset;
    Vec2 circle_offset;
    Vec2 outward_dir;
    float circle_amount;
};

struct CurveTool {
    MeshData* mesh;
    CurveToolEdge edges[MAX_EDGES];
    int edge_count;
    Vec2 drag_start;
};

static CurveTool g_curve = {};

static Vec2 CalculateCurveCircleOffset(MeshFrameData* frame, int edge_index, Vec2* out_outward_dir) {
    EdgeData& e = frame->edges[edge_index];

    // Find a face that uses this edge to get proper vertex ordering
    int face_index = e.face_index[0];
    if (face_index < 0) {
        *out_outward_dir = VEC2_ZERO;
        return VEC2_ZERO;
    }

    FaceData& face = frame->faces[face_index];

    // Calculate face centroid
    Vec2 centroid = VEC2_ZERO;
    for (int i = 0; i < face.vertex_count; i++)
        centroid += frame->vertices[face.vertices[i]].position;
    centroid = centroid / (float)face.vertex_count;

    // Find the edge vertices in face winding order
    int vi0 = -1, vi1 = -1;
    for (int i = 0; i < face.vertex_count; i++) {
        int v0 = face.vertices[i];
        int v1 = face.vertices[(i + 1) % face.vertex_count];
        if ((v0 == e.v0 && v1 == e.v1) || (v0 == e.v1 && v1 == e.v0)) {
            vi0 = v0;
            vi1 = v1;
            break;
        }
    }

    if (vi0 < 0) {
        *out_outward_dir = VEC2_ZERO;
        return VEC2_ZERO;
    }

    Vec2 p0 = frame->vertices[vi0].position;
    Vec2 p1 = frame->vertices[vi1].position;
    Vec2 midpoint = (p0 + p1) * 0.5f;

    // Outward direction from centroid to midpoint
    Vec2 to_mid = midpoint - centroid;
    float dist = Length(to_mid);
    *out_outward_dir = dist > 0.0001f ? to_mid / dist : VEC2_ZERO;

    // Use utility function for the tangent intersection calculation
    return CalculateCircleOffset(p0, p1, centroid);
}

static void EndCurve(bool commit) {
    if (!commit) {
        // Restore all original offsets
        MeshFrameData* frame = GetCurrentFrame(g_curve.mesh);
        for (int i = 0; i < g_curve.edge_count; i++) {
            frame->edges[g_curve.edges[i].edge_index].curve_offset = g_curve.edges[i].original_offset;
        }
        MarkDirty(g_curve.mesh);
    }

    EndDrag();
    EndTool();
}

static void UpdateCurve() {
    // Escape cancels and reverts
    if (WasButtonPressed(GetInputSet(), KEY_ESCAPE)) {
        EndCurve(false);
        return;
    }

    // Mouse released commits the curve
    if (!g_view.drag) {
        EndCurve(true);
        return;
    }

    // Calculate delta from drag start to current mouse position
    Vec2 delta = g_view.mouse_world_position - g_curve.drag_start;

    // Apply delta to all selected edges
    for (int i = 0; i < g_curve.edge_count; i++) {
        CurveToolEdge& ce = g_curve.edges[i];
        Vec2 new_offset = ce.original_offset + delta;

        // Ctrl snaps to: inverted circle, zero, or circle
        if (IsCtrlDown(GetInputSet())) {
            // Project new offset onto outward direction to determine which snap point
            float projected = Dot(new_offset, ce.outward_dir);
            float half_circle = ce.circle_amount * 0.5f;

            if (projected < -half_circle) {
                new_offset = ce.circle_offset * -1.0f;  // Inverted circle
            } else if (projected > half_circle) {
                new_offset = ce.circle_offset;          // Circle
            } else {
                new_offset = VEC2_ZERO;                 // Flat
            }
        }

        GetCurrentFrame(g_curve.mesh)->edges[ce.edge_index].curve_offset = new_offset;
    }

    MarkDirty(g_curve.mesh);
    MarkModified(g_curve.mesh);
}

void BeginCurveTool(MeshData* mesh, int* edge_indices, int edge_count) {
    static ToolVtable vtable = {
        .update = UpdateCurve
    };

    BeginTool({
        .type = TOOL_TYPE_CURVE,
        .vtable = vtable,
        .input = g_view.input_tool,
        .inherit_input = true
    });

    g_curve = {};
    g_curve.mesh = mesh;
    g_curve.edge_count = edge_count;
    g_curve.drag_start = g_view.mouse_world_position;

    MeshFrameData* frame = GetCurrentFrame(mesh);

    // Save original offsets and calculate circle offsets for all edges
    for (int i = 0; i < edge_count; i++) {
        CurveToolEdge& ce = g_curve.edges[i];
        ce.edge_index = edge_indices[i];
        ce.original_offset = frame->edges[edge_indices[i]].curve_offset;
        ce.circle_offset = CalculateCurveCircleOffset(frame, edge_indices[i], &ce.outward_dir);
        ce.circle_amount = Dot(ce.circle_offset, ce.outward_dir);
    }

    RecordUndo(mesh);
    BeginDrag();
}
