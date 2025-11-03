//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr int MAX_LINES = 1024;

struct DebugLineInfo {
    Vec2 start;
    Vec2 end;
    Color color;
    float width;
};

struct Debug {
    Mesh* quad_mesh;
    Material* material;
    DebugLineInfo lines[MAX_LINES];
    int line_count;
};

static Debug g_debug = {};

void DebugLine(const Vec2& start, const Vec2& end, const Color& color, float width) {
    if (start == end)
        return;

    if (g_debug.line_count >= MAX_LINES)
        return;

    g_debug.lines[g_debug.line_count++] = { start, end, color, width };
}

void DrawDebug() {
    BindDepth(GetApplicationTraits()->renderer.max_depth - 0.01f);
    BindMaterial(g_debug.material);
    for (int i=0; i<g_debug.line_count; i++) {
        const DebugLineInfo& line = g_debug.lines[i];
        Vec2 dir = Normalize(line.end - line.start);
        Vec2 scale = { Length(line.end - line.start), line.width };

        BindColor(line.color);
        Mat3 transform = Translate((line.start + line.end) * 0.5f) * Rotate(dir) * Scale(scale);
        DrawMesh(g_debug.quad_mesh, transform);
    }

    g_debug.line_count = 0;
}

void InitDebug() {
    PushScratch();
    MeshBuilder* mb = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(mb, Vec2{-0.5f, -0.5f});
    AddVertex(mb, Vec2{0.5f, -0.5f});
    AddVertex(mb, Vec2{0.5f, 0.5f});
    AddVertex(mb, Vec2{-0.5f, 0.5f});
    AddTriangle(mb, 0, 1, 2);
    AddTriangle(mb, 2, 3, 0);
    g_debug.quad_mesh = CreateMesh(ALLOCATOR_DEFAULT, mb, GetName("debug_quad"));
    PopScratch();

    g_debug.material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
}

void ShutdownDebug() {
    Free(g_debug.quad_mesh);
    Free(g_debug.material);
}