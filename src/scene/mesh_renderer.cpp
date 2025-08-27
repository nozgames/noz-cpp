//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct MeshRendererImpl
{
    ENTITY_BASE;
    Mesh* mesh;
    Material* material;
};

static MeshRendererImpl* Impl(MeshRenderer* c) { return (MeshRendererImpl*)Cast(c, TYPE_MESH_RENDERER); }

MeshRenderer* CreateMeshRenderer(Allocator* allocator)
{
    auto renderer = (MeshRenderer*)CreateEntity(allocator, sizeof(MeshRendererImpl), TYPE_MESH_RENDERER);
    return renderer;
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

static void MeshRendererRender(Entity* entity, Camera* camera)
{
    auto impl = Impl((MeshRenderer*)entity);
    if (!impl->mesh) return;
    if (!impl->material) return;
    BindMaterial(impl->material);
    BindTransform(entity);
    DrawMesh(impl->mesh);
}

void InitMeshRenderer()
{
    static EntityTraits traits = {
        .render = MeshRendererRender
    };
    SetEntityTraits(TYPE_MESH_RENDERER, &traits);
}
