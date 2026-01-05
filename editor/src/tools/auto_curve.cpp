//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr float AUTO_CURVE_CENTER_SIZE = 0.2f;
constexpr float AUTO_CURVE_LINE_WIDTH = 0.02f;

struct AutoCurveEdgeState {
    int edge_index;
    Vec2 original_offset;
    Vec2 circle_offset;
    Vec2 inverted_circle_offset;
    Vec2 outward_dir;
    float edge_length;
    float circle_amount;  // Magnitude of circle offset in outward direction (for clamping)
};

struct AutoCurveTool {
    MeshData* mesh;
    AutoCurveEdgeState edges[MAX_EDGES];
    int edge_count;
    Vec2 centroid;
    float initial_distance;
};

static AutoCurveTool g_auto_curve = {};

static void CancelAutoCurve() {
    MeshDataImpl* impl = g_auto_curve.mesh->impl;
    for (int i = 0; i < g_auto_curve.edge_count; i++) {
        AutoCurveEdgeState& es = g_auto_curve.edges[i];
        impl->edges[es.edge_index].curve_offset = es.original_offset;
    }
    MarkDirty(g_auto_curve.mesh);
}

static void EndAutoCurveTool(bool commit) {
    if (!commit) {
        CancelAutoCurve();
        CancelUndo();
    }
    g_auto_curve.mesh = nullptr;
    EndTool();
}

static void DrawAutoCurveTool() {
    Vec2 centroid_world = g_auto_curve.centroid + g_auto_curve.mesh->position;

    BindMaterial(g_view.vertex_material);
    BindColor(SetAlpha(COLOR_CENTER, 0.75f));
    DrawVertex(centroid_world, AUTO_CURVE_CENTER_SIZE * 0.75f);
    BindColor(COLOR_CENTER);
    DrawLine(g_view.mouse_world_position, centroid_world, AUTO_CURVE_LINE_WIDTH);
    BindColor(COLOR_ORIGIN);
    DrawVertex(g_view.mouse_world_position, AUTO_CURVE_CENTER_SIZE);
}

static void UpdateAutoCurveTool() {
    if (WasButtonPressed(KEY_ESCAPE)) {
        EndAutoCurveTool(false);
        return;
    }

    if (WasButtonPressed(MOUSE_LEFT)) {
        MarkModified(g_auto_curve.mesh);
        EndAutoCurveTool(true);
        return;
    }

    MeshDataImpl* impl = g_auto_curve.mesh->impl;
    Vec2 centroid_world = g_auto_curve.centroid + g_auto_curve.mesh->position;
    float current_dist = Length(g_view.mouse_world_position - centroid_world);
    float delta = current_dist - g_auto_curve.initial_distance;

    if (IsCtrlDown(GetInputSet())) {
        // Snap to: inverted circle, zero, or perfect circle
        // Use raw delta (not scaled) for snapping to make snap points easier to hit
        for (int i = 0; i < g_auto_curve.edge_count; i++) {
            AutoCurveEdgeState& es = g_auto_curve.edges[i];

            // Boundaries at halfway between snap points
            float half_circle = es.circle_amount * 0.5f;

            Vec2 offset;
            if (delta < -half_circle) {
                offset = es.inverted_circle_offset;
            } else if (delta > half_circle) {
                offset = es.circle_offset;
            } else {
                offset = VEC2_ZERO;
            }
            impl->edges[es.edge_index].curve_offset = offset;
        }
    } else {
        // Proportional curve based on drag, relative to original
        // Clamp to inverted circle as minimum (can't go more concave than that)
        for (int i = 0; i < g_auto_curve.edge_count; i++) {
            AutoCurveEdgeState& es = g_auto_curve.edges[i];
            float amount = delta * 0.5f;

            // Calculate current offset in outward direction
            float original_amount = Dot(es.original_offset, es.outward_dir);
            float new_amount = original_amount + amount;

            // Clamp to inverted circle minimum (-circle_amount)
            if (new_amount < -es.circle_amount) {
                amount = -es.circle_amount - original_amount;
            }

            impl->edges[es.edge_index].curve_offset = es.original_offset + es.outward_dir * amount;
        }
    }

    MarkDirty(g_auto_curve.mesh);
}

// Wrapper to call the utility function with vertex indices
static Vec2 CalcCircleOffsetFromIndices(MeshDataImpl* impl, const Vec2& centroid, int vi0, int vi1) {
    Vec2 p0 = impl->vertices[vi0].position;
    Vec2 p1 = impl->vertices[vi1].position;
    return CalculateCircleOffset(p0, p1, centroid);
}

void BeginAutoCurveTool(MeshData* mesh) {
    MeshDataImpl* impl = mesh->impl;

    if (impl->selected_face_count == 0)
        return;

    RecordUndo(mesh);

    static ToolVtable vtable = {
        .cancel = CancelAutoCurve,
        .update = UpdateAutoCurveTool,
        .draw = DrawAutoCurveTool
    };

    BeginTool({
        .type = TOOL_TYPE_CURVE,
        .vtable = vtable,
        .input = g_view.input_tool,
        .hide_selected = false
    });

    g_auto_curve.mesh = mesh;
    g_auto_curve.edge_count = 0;

    // Calculate combined centroid of all selected faces
    Vec2 total_centroid = VEC2_ZERO;
    int total_verts = 0;

    for (int fi = 0; fi < impl->face_count; fi++) {
        FaceData& face = impl->faces[fi];
        if (!face.selected)
            continue;

        for (int i = 0; i < face.vertex_count; i++) {
            total_centroid += impl->vertices[face.vertices[i]].position;
            total_verts++;
        }
    }

    if (total_verts == 0) {
        EndTool();
        return;
    }

    g_auto_curve.centroid = total_centroid / (float)total_verts;

    // Collect edges from selected faces
    for (int fi = 0; fi < impl->face_count; fi++) {
        FaceData& face = impl->faces[fi];
        if (!face.selected || face.vertex_count < 3)
            continue;

        for (int i = 0; i < face.vertex_count; i++) {
            int v0 = face.vertices[i];
            int v1 = face.vertices[(i + 1) % face.vertex_count];
            int edge_index = GetEdge(mesh, v0, v1);
            if (edge_index < 0)
                continue;

            // Check if we already added this edge
            bool found = false;
            for (int j = 0; j < g_auto_curve.edge_count; j++) {
                if (g_auto_curve.edges[j].edge_index == edge_index) {
                    found = true;
                    break;
                }
            }
            if (found)
                continue;

            EdgeData& e = impl->edges[edge_index];
            Vec2 p0 = impl->vertices[v0].position;
            Vec2 p1 = impl->vertices[v1].position;
            Vec2 midpoint = (p0 + p1) * 0.5f;

            Vec2 to_mid = midpoint - g_auto_curve.centroid;
            float dist = Length(to_mid);

            AutoCurveEdgeState& es = g_auto_curve.edges[g_auto_curve.edge_count++];
            es.edge_index = edge_index;
            es.original_offset = e.curve_offset;
            es.circle_offset = CalcCircleOffsetFromIndices(impl, g_auto_curve.centroid, v0, v1);
            es.inverted_circle_offset = es.circle_offset * -1.0f;
            es.outward_dir = dist > 0.0001f ? to_mid / dist : VEC2_ZERO;
            es.edge_length = Length(p1 - p0);
            es.circle_amount = Dot(es.circle_offset, es.outward_dir);  // How far circle extends outward
        }
    }

    Vec2 centroid_world = g_auto_curve.centroid + mesh->position;
    g_auto_curve.initial_distance = Length(g_view.mouse_world_position - centroid_world);

    SetSystemCursor(SYSTEM_CURSOR_SELECT);
}
