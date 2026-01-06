//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr int DEBUG_MAX_LINES = 1024;

struct DebugLineInfo {
    Vec2 start;
    Vec2 end;
    Color color;
    float width;
};

struct DebugGizmos {
    Mesh* quad_mesh;
    Material* material;
    DebugLineInfo lines[DEBUG_MAX_LINES];
    int line_count;
};

static DebugGizmos g_debug_gizmos = {};

void DebugLine(const Vec2& start, const Vec2& end, const Color& color, float width) {
    if (start == end)
        return;

    if (g_debug_gizmos.line_count >= DEBUG_MAX_LINES)
        return;

    g_debug_gizmos.lines[g_debug_gizmos.line_count++] = { start, end, color, width };
}

void DebugBounds(const Bounds2& bounds, const Mat3& transform, const Color& color, float width) {
    Vec2 corners[4] = {
        { bounds.min.x, bounds.min.y },
        { bounds.max.x, bounds.min.y },
        { bounds.max.x, bounds.max.y },
        { bounds.min.x, bounds.max.y }
    };

    for (int i = 0; i < 4; i++) {
        Vec2 start = TransformPoint(transform, corners[i]);
        Vec2 end = TransformPoint(transform, corners[(i + 1) % 4]);
        DebugLine(start, end, color, width);
    }
}

void DrawDebugGizmos() {
    BindDepth(GetApplicationTraits()->renderer.max_depth - 0.01f);
    BindMaterial(g_debug_gizmos.material);
    for (int i=0; i<g_debug_gizmos.line_count; i++) {
        const DebugLineInfo& line = g_debug_gizmos.lines[i];
        Vec2 dir = Normalize(line.end - line.start);
        Vec2 scale = { Length(line.end - line.start), line.width };

        BindColor(line.color);
        Mat3 transform = Translate((line.start + line.end) * 0.5f) * Rotate(dir) * Scale(scale);
        DrawMesh(g_debug_gizmos.quad_mesh, transform);
    }

    g_debug_gizmos.line_count = 0;
}

void InitDebugGizmos() {
    PushScratch();
    MeshBuilder* mb = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(mb, Vec2{-0.5f, -0.5f});
    AddVertex(mb, Vec2{0.5f, -0.5f});
    AddVertex(mb, Vec2{0.5f, 0.5f});
    AddVertex(mb, Vec2{-0.5f, 0.5f});
    AddTriangle(mb, 0, 1, 2);
    AddTriangle(mb, 2, 3, 0);
    g_debug_gizmos.quad_mesh = CreateMesh(ALLOCATOR_DEFAULT, mb, GetName("debug_quad"));
    PopScratch();

    g_debug_gizmos.material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
}

void ShutdownDebugGizmos() {
    Free(g_debug_gizmos.quad_mesh);
    Free(g_debug_gizmos.material);
}
