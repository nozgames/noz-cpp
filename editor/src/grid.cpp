//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

constexpr float GRID_MAX_ALPHA = 0.6f;

struct Grid {
    Mesh* mesh;
    float snap_spacing;
};

static Grid g_grid = {};

static void AddLineQuad(MeshBuilder* builder, const Vec2& center, const Vec2& half_size, const Color& color) {
    SetBaseVertex(builder);

    Vec4 color_vec = {color.r, color.g, color.b, color.a};
    MeshVertex v0 = {.position = {center.x - half_size.x, center.y - half_size.y}, .bone_weights = color_vec};
    MeshVertex v1 = {.position = {center.x + half_size.x, center.y - half_size.y}, .bone_weights = color_vec};
    MeshVertex v2 = {.position = {center.x + half_size.x, center.y + half_size.y}, .bone_weights = color_vec};
    MeshVertex v3 = {.position = {center.x - half_size.x, center.y + half_size.y}, .bone_weights = color_vec};

    AddVertex(builder, v0);
    AddVertex(builder, v1);
    AddVertex(builder, v2);
    AddVertex(builder, v3);
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 2, 3, 0);
}

static void BuildZeroGrid(MeshBuilder* builder, Camera* camera, const Color& color) {
    Vec2Int screen_size = GetScreenSize();
    Bounds2 bounds = GetWorldBounds(camera);
    float left = bounds.min.x;
    float right = bounds.max.x;
    float bottom = bounds.min.y;
    float top = bounds.max.y;
    float world_height = top - bottom;
    float pixels_per_world_unit = screen_size.y / world_height;
    float line_thickness = 1.5f / pixels_per_world_unit;

    AddLineQuad(builder, Vec2{0, (top + bottom) * 0.5f}, Vec2{line_thickness, (top - bottom) * 0.5f}, color);
    AddLineQuad(builder, Vec2{(left + right) * 0.5f, 0}, Vec2{(right - left) * 0.5f, line_thickness}, color);
}

static void BuildGridLines(MeshBuilder* builder, Camera* camera, float spacing, const Color& color) {
    if (color.a <= FLT_EPSILON) return;

    Bounds2 bounds = GetWorldBounds(camera);
    float left = bounds.min.x;
    float right = bounds.max.x;
    float bottom = bounds.min.y;
    float top = bounds.max.y;

    Vec2Int screen_size = GetScreenSize();
    float world_height = top - bottom;
    float pixels_per_world_unit = screen_size.y / world_height;
    float line_thickness = 1.0f / pixels_per_world_unit;

    float start_x = floorf(left / spacing) * spacing;
    for (float x = start_x; x <= right + spacing; x += spacing)
        AddLineQuad(builder, Vec2{x, (top + bottom) * 0.5f}, Vec2{line_thickness, (top - bottom) * 0.5f}, color);

    float start_y = floorf(bottom / spacing) * spacing;
    for (float y = start_y; y <= top + spacing; y += spacing)
        AddLineQuad(builder, Vec2{(left + right) * 0.5f, y}, Vec2{(right - left) * 0.5f, line_thickness}, color);
}

static float CalculateGridSpacing(
    Camera* camera,
    float min_pixels,
    float base_spacing,
    float* out_alpha,
    float min_alpha,
    float max_alpha) {
    // Use camera to find how big this spacing is on screen
    Vec2 world_0 = WorldToScreen(camera, Vec2{0, 0});
    Vec2 world_1 = WorldToScreen(camera, Vec2{1.0f, 0});
    f32 pixels_per_grid = Length(world_1 - world_0);
    float grid_spacing = base_spacing;

    // Scale up by 10x as long as grid is smaller than threshold
    while (pixels_per_grid < min_pixels) {
        grid_spacing *= 10.0f;
        pixels_per_grid *= 10.0f;
    }

    // Scale down by 10x as long as grid is larger than threshold * 10
    while (pixels_per_grid > min_pixels * 10.0f) {
        grid_spacing *= 0.1f;
        pixels_per_grid *= 0.1f;
    }

    *out_alpha = Mix(min_alpha, max_alpha, (pixels_per_grid - min_pixels) / (min_pixels * 10.0f));
    return grid_spacing;
}

void DrawGrid(Camera* camera) {
    BindDepth(-9.0f);
    BindMaterial(g_view.editor_material);

    float alpha1, alpha2;
    float spacing1 = CalculateGridSpacing(camera, 72.0f, 1.0f, &alpha1, GRID_MAX_ALPHA, GRID_MAX_ALPHA);
    float spacing2 = CalculateGridSpacing(camera, 72.0f, 0.1f, &alpha2, 0.0f, GRID_MAX_ALPHA);
    g_grid.snap_spacing = spacing2;

    constexpr int MAX_GRID_LINES = 1024;
    constexpr int VERTS_PER_LINE = 4;
    constexpr int INDICES_PER_LINE = 6;

    PushScratch();

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MAX_GRID_LINES * VERTS_PER_LINE, MAX_GRID_LINES * INDICES_PER_LINE);
    BuildGridLines(builder, camera, spacing1, MultiplyAlpha(STYLE_GRID_COLOR(), alpha1));
    BuildGridLines(builder, camera, spacing2, MultiplyAlpha(STYLE_GRID_COLOR(), alpha2));

    BuildZeroGrid(builder, camera, STYLE_GRID_COLOR());

    if (GetVertexCount(builder) > 0) {
        if (!g_grid.mesh)
            g_grid.mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        else
            UpdateMeshFromBuilder(g_grid.mesh, builder);

        BindColor(COLOR_WHITE);
        BindTransform(MAT3_IDENTITY);
        DrawMesh(g_grid.mesh);
    }

    PopScratch();
    BindDepth(0.0f);
}

void InitGrid() {
    g_grid.mesh = nullptr;
}

Vec2 SnapToGrid(const Vec2& position) {
    float spacing = g_grid.snap_spacing > 0.0f ? g_grid.snap_spacing : 0.1f;
    return Vec2{
        roundf(position.x / spacing) * spacing,
        roundf(position.y / spacing) * spacing
    };
}

float SnapAngle(float angle) {
    constexpr float angle_step = 15.0f;
    return roundf(angle / angle_step) * angle_step;
}

void ShutdownGrid() {
    if (g_grid.mesh)
        Free(g_grid.mesh);
    g_grid = {};
}
