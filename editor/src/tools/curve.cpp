//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

struct CurveTool {
    MeshData* mesh;
    int edge_indices[MAX_EDGES];
    int edge_count;
    Vec2 original_offsets[MAX_EDGES];
    Vec2 drag_start;
};

static CurveTool g_curve = {};

static void EndCurve(bool commit) {
    if (!commit) {
        // Restore all original offsets
        for (int i = 0; i < g_curve.edge_count; i++) {
            g_curve.mesh->impl->edges[g_curve.edge_indices[i]].curve_offset = g_curve.original_offsets[i];
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
        int edge_index = g_curve.edge_indices[i];
        g_curve.mesh->impl->edges[edge_index].curve_offset = g_curve.original_offsets[i] + delta;
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

    // Save original offsets for all edges
    for (int i = 0; i < edge_count; i++) {
        g_curve.edge_indices[i] = edge_indices[i];
        g_curve.original_offsets[i] = mesh->impl->edges[edge_indices[i]].curve_offset;
    }

    RecordUndo(mesh);
    BeginDrag();
}
