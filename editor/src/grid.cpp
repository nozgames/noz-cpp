//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    constexpr float GRID_MAX_ALPHA = 0.6f;

    struct Grid {
        Mesh* coarse_mesh;
        Mesh* pixel_mesh;
        float snap_spacing;
        bool pixel_grid_visible;
    };

    static Grid g_grid = {};

    static void AddLineQuad(MeshBuilder* builder, const Vec2& center, const Vec2& half_size, const Color& color) {
        SetBaseVertex(builder);

        Vec4 color_vec = {color.r, color.g, color.b, color.a};
        MeshVertex v0 = {.position = {center.x - half_size.x, center.y - half_size.y}, .color = color_vec};
        MeshVertex v1 = {.position = {center.x + half_size.x, center.y - half_size.y}, .color = color_vec};
        MeshVertex v2 = {.position = {center.x + half_size.x, center.y + half_size.y}, .color = color_vec};
        MeshVertex v3 = {.position = {center.x - half_size.x, center.y + half_size.y}, .color = color_vec};

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

    struct GridLevels
    {
        float fineSpacing;
        float fineAlpha;
        float coarseSpacing;
        float coarseAlpha;
    };
    GridLevels calculateGridLevels(float worldWidth, float screenWidth,
                                   float baseUnit,  // 1.0f / dpi
                                   float targetGridScreenSpacing)
    {
        float worldPerPixel = worldWidth / screenWidth;
        float idealWorldSpacing = worldPerPixel * targetGridScreenSpacing;

        // Work in multiples of the base unit
        float idealInBaseUnits = idealWorldSpacing / baseUnit;

        float logSpacing = std::log10(idealInBaseUnits);
        float clampedLog = std::max(logSpacing, 0.0f);  // Never finer than 1 base unit
        float floorLog = std::floor(clampedLog);

        float t = clampedLog - floorLog;

        float multiplier = std::round(std::pow(10.0f, floorLog));

        float fineSpacing = baseUnit * multiplier;
        float coarseSpacing = fineSpacing * 10.0f;

        float fineAlpha = 1.0f - t;
        float coarseAlpha = 1.0f;

        return { fineSpacing, fineAlpha, coarseSpacing, coarseAlpha };
    }
    
    void UpdateGrid(Camera* camera) {
        float pixelSize = 1.0f / (float)g_editor.atlas.dpi;
        float dpi = (float)g_editor.atlas.dpi;
        float worldWidth = GetWorldBounds(camera).max.x - GetWorldBounds(camera).min.x;
        float screenWidth = static_cast<float>(GetScreenSize().x);

        // Main grid: 1, 10, 100... world units
        GridLevels world = calculateGridLevels(worldWidth, screenWidth, 1.0f, dpi);

        // Pixel grid: fades in when zoomed in
        float screenPixelsPerWorldPixel = screenWidth / (worldWidth / pixelSize);
        float pixelGridAlpha = 0.0f;
        if (screenPixelsPerWorldPixel > 8.0f)
            pixelGridAlpha = std::min((screenPixelsPerWorldPixel - 8.0f) / 32.0f, 1.0f);

        float spacing1 = world.coarseSpacing;
        float alpha1 = world.coarseAlpha * GRID_MAX_ALPHA;

        float spacing2 = world.fineSpacing;
        float alpha2 = world.fineAlpha * GRID_MAX_ALPHA;

        float spacing3 = pixelSize;
        float alpha3 = pixelGridAlpha * GRID_MAX_ALPHA * 0.5f;  // bit dimmer

        g_grid.pixel_grid_visible = alpha3 > F32_EPSILON;

        if (g_grid.pixel_grid_visible) {
            g_grid.snap_spacing = spacing3;
        } else {
            g_grid.snap_spacing = spacing2 * 0.5f;
        }

        constexpr int MAX_GRID_LINES = 1024;
        constexpr int VERTS_PER_LINE = 4;
        constexpr int INDICES_PER_LINE = 6;

        PushScratch();

        // course
        MeshBuilder* coarse_builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MAX_GRID_LINES * VERTS_PER_LINE, MAX_GRID_LINES * INDICES_PER_LINE);
        BuildGridLines(coarse_builder, camera, spacing1, MultiplyAlpha(STYLE_GRID_COLOR(), alpha1));
        BuildGridLines(coarse_builder, camera, spacing2, MultiplyAlpha(STYLE_GRID_COLOR(), alpha2));
        BuildZeroGrid(coarse_builder, camera, STYLE_GRID_COLOR());

        if (GetVertexCount(coarse_builder) > 0) {
            if (!g_grid.coarse_mesh)
                g_grid.coarse_mesh = CreateMesh(ALLOCATOR_DEFAULT, coarse_builder, NAME_NONE, true);
            else
                UpdateMesh(coarse_builder, g_grid.coarse_mesh);
        }
        Free(coarse_builder);

        // pixel
        if (g_grid.pixel_grid_visible) {
            MeshBuilder* pixel_builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, MAX_GRID_LINES * VERTS_PER_LINE, MAX_GRID_LINES * INDICES_PER_LINE);
            BuildGridLines(pixel_builder, camera, spacing3, MultiplyAlpha(STYLE_GRID_COLOR(), alpha3 * GRID_MAX_ALPHA * 0.5f));

            if (GetVertexCount(pixel_builder) > 0) {
                if (!g_grid.pixel_mesh)
                    g_grid.pixel_mesh = CreateMesh(ALLOCATOR_DEFAULT, pixel_builder, NAME_NONE, true);
                else
                    UpdateMesh(pixel_builder, g_grid.pixel_mesh);
            }
            Free(pixel_builder);
        }

        PopScratch();
    }

    void DrawGrid() {
        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindDepth(-9.0f);
        BindTransform(MAT3_IDENTITY);

        // Draw coarse/world grid at bottom depth
        if (g_grid.coarse_mesh) {
            DrawMesh(g_grid.coarse_mesh);
        }

        // Draw pixel grid at top depth when visible
        if (!g_grid.pixel_grid_visible && g_grid.pixel_mesh) {
            DrawMesh(g_grid.pixel_mesh);
        }
    }

    void DrawPixelGrid() {
        // Draw pixel grid at top depth when visible
        if (g_grid.pixel_grid_visible && g_grid.pixel_mesh) {
            BindMaterial(g_workspace.editor_material);
            BindColor(COLOR_WHITE);
            BindDepth(-9.0f);
            BindTransform(MAT3_IDENTITY);
            DrawMesh(g_grid.pixel_mesh);
        }
    }

    void InitGrid() {
        g_grid.coarse_mesh = nullptr;
        g_grid.pixel_mesh = nullptr;
        g_grid.pixel_grid_visible = false;
    }

    Vec2 SnapToPixelGrid(const Vec2& position) {
        float spacing = 1.0f / g_editor.atlas.dpi;
        return Vec2{
            roundf(position.x / spacing) * spacing,
            roundf(position.y / spacing) * spacing
        };
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
        Free(g_grid.coarse_mesh);
        Free(g_grid.pixel_mesh);
        g_grid = {};
    }
}
