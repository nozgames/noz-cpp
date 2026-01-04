//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

static void DrawAtlasData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_ATLAS);

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Scale factor to display atlas in world units
    float scale = 10.0f / (float)impl->width;  // Normalize to 10 units
    Vec2 size = Vec2{(float)impl->width, (float)impl->height} * scale;

    // Draw the atlas texture if available
    if (impl->material) {
        BindDepth(-0.1f);
        BindColor(COLOR_WHITE);
        BindMaterial(impl->material);
        // Flip Y because texture coords are top-down
        DrawMesh(g_view.quad_mesh, Translate(a->position) * Scale(Vec2{size.x, -size.y}));
    } else {
        // Draw a gray placeholder
        BindDepth(0.0f);
        BindMaterial(g_view.editor_material);
        BindColor(Color{0.2f, 0.2f, 0.2f, 1.0f});
        DrawMesh(g_view.quad_mesh, Translate(a->position) * Scale(size));
    }

    // Draw rect outlines for each attached mesh
    BindDepth(0.1f);
    BindMaterial(g_view.editor_material);
    for (int i = 0; i < impl->rect_count; i++) {
        if (!impl->rects[i].valid) continue;

        Vec2 rect_pos = Vec2{(float)impl->rects[i].x, (float)impl->rects[i].y} * scale;
        Vec2 rect_size = Vec2{(float)impl->rects[i].width, (float)impl->rects[i].height} * scale;

        // Position relative to atlas origin (top-left)
        Vec2 center = a->position - size * 0.5f + rect_pos + rect_size * 0.5f;
        // Flip Y
        center.y = a->position.y + size.y * 0.5f - rect_pos.y - rect_size.y * 0.5f;

        BindColor(Color{0.4f, 0.8f, 0.4f, 0.3f});
        DrawMesh(g_view.quad_mesh, Translate(center) * Scale(rect_size));
    }
}

static Bounds2 GetAtlasBounds() {
    AssetData* a = GetAssetData();
    return a->bounds;
}

void InitAtlasEditor(AtlasData* atlas) {
    atlas->vtable.draw = DrawAtlasData;
    atlas->vtable.editor_bounds = GetAtlasBounds;
}
