//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0

struct MeshRendererImpl : Component
{
    Mesh* mesh;
    Material* material;
    uint64_t render_layer;
    LinkedListNode node_render;
};

static MeshRendererImpl* Impl(MeshRenderer* c) { return (MeshRendererImpl*)Cast(c, TYPE_MESH_RENDERER); }
static LinkedList g_mesh_renderers = {};

MeshRenderer* CreateMeshRenderer(Allocator* allocator)
{
    auto renderer = (MeshRenderer*)CreateComponent(allocator, sizeof(MeshRendererImpl), TYPE_MESH_RENDERER);
    auto impl = Impl(renderer);
    impl->render_layer = RENDER_MASK_DEFAULT;
    return renderer;
}

void SetRenderLayer(MeshRenderer* renderer, uint64_t layer)
{
    Impl(renderer)->render_layer = layer;
}

void SetMaterial(MeshRenderer* renderer, Material* material)
{
    auto impl = Impl(renderer);
    impl->material = material;
}

void SetMesh(MeshRenderer* renderer, Mesh* mesh)
{
    auto impl = Impl(renderer);;
    impl->mesh = mesh;
}

static void MeshOnEnabled(Component* component)
{
    assert(!IsInList(g_mesh_renderers, component));
    PushBack(g_mesh_renderers, component);
}

static void MeshOnDisabled(Component* component)
{
    assert(IsInList(g_mesh_renderers, component));
    Remove(g_mesh_renderers, component);
}

static void DrawMeshRenderer(MeshRenderer* renderer, Camera* camera)
{
    auto impl = Impl(renderer);
    auto entity = GetEntity(renderer);
    if (!impl->mesh) return;
    if (!impl->material) return;
    BindMaterial(impl->material);
    BindTransform(entity);
    DrawMesh(impl->mesh);
}

void DrawMeshRenderers(Camera* camera)
{
    u64 mask = GetRenderLayerMask(camera);
    for (auto renderer = (MeshRenderer*)GetFront(g_mesh_renderers);
        renderer;
        renderer = (MeshRenderer*)GetNext(g_mesh_renderers, renderer))
    {
        if ((mask & Impl(renderer)->render_layer) != 0)
            DrawMeshRenderer(renderer, camera);
    }
}

void InitMeshRenderer()
{
    static ComponentTraits traits = {
        .on_enabled = MeshOnEnabled,
        .on_disabled = MeshOnDisabled,
    };
    SetComponentTraits(TYPE_MESH_RENDERER, &traits);

    Init(g_mesh_renderers, offsetof(MeshRendererImpl, node_render));
}

#endif