//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CurveTool {
    MeshData* mesh;
    int edge_index;
    Vec2 original_offset;
};

static CurveTool g_curve = {};

static void EndCurve(bool commit) {
    if (!commit) {
        g_curve.mesh->impl->edges[g_curve.edge_index].curve_offset = g_curve.original_offset;
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

    // Calculate new control point offset based on mouse position
    Vec2 midpoint = GetEdgeMidpoint(g_curve.mesh, g_curve.edge_index);
    Vec2 local_pos = g_view.mouse_world_position - g_curve.mesh->position;
    Vec2 new_offset = local_pos - midpoint;

    g_curve.mesh->impl->edges[g_curve.edge_index].curve_offset = new_offset;
    MarkDirty(g_curve.mesh);
    MarkModified(g_curve.mesh);
}

void BeginCurveTool(MeshData* mesh, int edge_index) {
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
    g_curve.edge_index = edge_index;
    g_curve.original_offset = mesh->impl->edges[edge_index].curve_offset;

    RecordUndo(mesh);
    BeginDrag();
}
