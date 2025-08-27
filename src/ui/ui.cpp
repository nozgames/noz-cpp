//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void InitElement();
void InitCanvas();
void InitLabel();
void ShutdownElement();

struct UI
{
    Mesh* element_quad;
    Material* element_material;
    Font* default_font;
};

static UI g_ui = {};

void SetDefaultFont(Font* font)
{
    g_ui.default_font = font;
}

Font* GetDefaultFont()
{
    return g_ui.default_font;
}

void RenderElementQuad(const color_t& color, Texture* texture)
{
    BindMaterial(g_ui.element_material);
    BindColor(color);
    DrawMesh(g_ui.element_quad);
}

static Mesh* CreateElementQuad(Allocator* allocator)
{
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 0.0f), 0);
    AddVertex(builder, vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f), 0);
    AddVertex(builder, vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 1.0f), 0);
    AddVertex(builder, vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 1.0f), 0);
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    auto mesh = CreateMesh(allocator, builder, GetName("element"));
    Destroy(builder);
    return mesh;
}

void RenderUI()
{
    // TODO: render all screen canvases based on sort
}

void InitUI()
{
    g_ui.element_quad = CreateElementQuad(ALLOCATOR_DEFAULT);
    g_ui.default_font = CoreAssets.fonts.fallback;
    g_ui.element_material = CreateMaterial(ALLOCATOR_DEFAULT, CoreAssets.shaders.ui);
    SetTexture(g_ui.element_material, CoreAssets.textures.white);

    InitCanvas();
    InitElement();
    InitLabel();
}

void ShutdownUI()
{
    ShutdownElement();
    g_ui = {};
}
