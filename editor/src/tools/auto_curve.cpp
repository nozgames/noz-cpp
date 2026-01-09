//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr float AUTO_CURVE_CENTER_SIZE = 0.2f;
constexpr float AUTO_CURVE_LINE_WIDTH = 0.02f;

struct AutoCurveEdgeState {
    int edge_index;
    Vec2 original_offset;
    float original_weight;
    Vec2 circle_offset;
    Vec2 inverted_circle_offset;
    Vec2 outward_dir;
    float edge_length;
    float circle_amount;  // Magnitude of circle offset in outward direction (for clamping)
    float circle_weight;  // Weight for perfect circular arc (cos(half_angle))
};

struct AutoCurveTool {
    MeshData* mesh;
    AutoCurveEdgeState edges[MESH_MAX_EDGES];
    int edge_count;
    Vec2 centroid;
    float initial_distance;
};

static AutoCurveTool g_auto_curve = {};

#if 0

static void CancelAutoCurve() {
    MeshFrameData* frame = GetCurrentFrame(g_auto_curve.mesh);
    for (int i = 0; i < g_auto_curve.edge_count; i++) {
        AutoCurveEdgeState& es = g_auto_curve.edges[i];
        frame->edges[es.edge_index].curve.offset = es.original_offset;
        frame->edges[es.edge_index].curve.weight = es.original_weight;
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

    MeshFrameData* frame = GetCurrentFrame(g_auto_curve.mesh);
    Vec2 centroid_world = g_auto_curve.centroid + g_auto_curve.mesh->position;
    float current_dist = Length(g_view.mouse_world_position - centroid_world);
    float delta = current_dist - g_auto_curve.initial_distance;

    if (IsCtrlDown(GetInputSet())) {
        // Snap to: inverted circle, zero, or perfect circle
        // Use raw delta (not scaled) for snapping to make snap points easier to hit
        for (int i = 0; i < g_auto_curve.edge_count; i++) {
            AutoCurveEdgeState& es = g_auto_curve.edges[i];
            EdgeData* e = frame->edges + es.edge_index;

            // Boundaries at halfway between snap points
            float half_circle = es.circle_amount * 0.5f;

            Vec2 offset;
            float weight;
            if (delta < -half_circle) {
                offset = es.inverted_circle_offset;
                weight = es.circle_weight;  // Use circle weight for inverted too
            } else if (delta > half_circle) {
                offset = es.circle_offset;
                weight = es.circle_weight;  // Perfect circle weight
            } else {
                offset = VEC2_ZERO;
                weight = 1.0f;
            }

            e->curve.weight = weight;
            e->curve.offset = offset;
        }
    } else {
        // Proportional curve based on drag, relative to original
        // Clamp to inverted circle as minimum (can't go more concave than that)
        for (int i = 0; i < g_auto_curve.edge_count; i++) {
            AutoCurveEdgeState& es = g_auto_curve.edges[i];
            EdgeData& edge = frame->edges[es.edge_index];
            float amount = delta * 0.5f;

            // Calculate current offset in outward direction
            float original_amount = Dot(es.original_offset, es.outward_dir);
            float new_amount = original_amount + amount;

            // Clamp to inverted circle minimum (-circle_amount)
            if (new_amount < -es.circle_amount) {
                amount = -es.circle_amount - original_amount;
            }

            edge.curve.offset = es.original_offset + es.outward_dir * amount;
            edge.curve.weight = 1.0f;  // Standard bezier weight for non-snapped curves
        }
    }

    MarkDirty(g_auto_curve.mesh);
}

// Wrapper to call the utility function with vertex indices
static Vec2 CalcCircleOffsetFromIndices(MeshFrameData* frame, const Vec2& centroid, int vi0, int vi1) {
    Vec2 p0 = frame->vertices[vi0].position;
    Vec2 p1 = frame->vertices[vi1].position;
    return CalculateCircleOffset(p0, p1, centroid);
}

#endif


void BeginAutoCurveTool(MeshData* mesh) {
#if 0
    MeshFrameData* frame = GetCurrentFrame(mesh);

    if (frame->selected_face_count == 0)
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
    for (u16 face_index = 0; face_index < frame->face_count; face_index++) {
        FaceData* f = GetFace(frame, face_index);
        if (!IsSelected(f)) continue;
        total_centroid += f->centroid;
    }

    g_auto_curve.centroid = total_centroid / frame->selected_face_count;

    // Collect edges from selected faces
    for (u16 face_index = 0; face_index < frame->face_count; face_index++) {
        FaceData* f = GetFace(frame, face_index);
        if (!IsSelected(f) || f.edge_count < 3)
            continue;

        for (int i = 0; i < f.vertex_count; i++) {
            int v0 = f.vertices[i];
            int v1 = f.vertices[(i + 1) % f.vertex_count];
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

            EdgeData& e = frame->edges[edge_index];
            Vec2 p0 = frame->vertices[v0].position;
            Vec2 p1 = frame->vertices[v1].position;
            Vec2 midpoint = (p0 + p1) * 0.5f;

            Vec2 to_mid = midpoint - g_auto_curve.centroid;
            float dist = Length(to_mid);

            AutoCurveEdgeState& es = g_auto_curve.edges[g_auto_curve.edge_count++];
            es.edge_index = edge_index;
            es.original_offset = e.curve.offset;
            es.original_weight = e.curve.weight;
            es.circle_offset = CalcCircleOffsetFromIndices(frame, g_auto_curve.centroid, v0, v1);
            es.inverted_circle_offset = es.circle_offset * -1.0f;
            es.outward_dir = dist > 0.0001f ? to_mid / dist : VEC2_ZERO;
            es.edge_length = Length(p1 - p0);
            es.circle_amount = Dot(es.circle_offset, es.outward_dir);  // How far circle extends outward
            es.circle_weight = CalculateCircleWeight(p0, p1, g_auto_curve.centroid);  // Weight for perfect arc
        }
    }

    Vec2 centroid_world = g_auto_curve.centroid + mesh->position;
    g_auto_curve.initial_distance = Length(g_view.mouse_world_position - centroid_world);

    SetSystemCursor(SYSTEM_CURSOR_SELECT);
#endif
}
