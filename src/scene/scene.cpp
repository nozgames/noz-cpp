//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

Entity* CreateRootEntity(Allocator* allocator, Scene* scene);
void InitRenderEntityList(LinkedList& list);

struct SceneImpl
{
    Entity* root;
    Camera* camera;
    LinkedList render_entities;
};

static SceneImpl* Impl(Scene* s) { return (SceneImpl*)Cast(s, TYPE_SCENE); }

void InitEntity();
void InitMeshRenderer();
void RenderScreenCanvases();

Scene* CreateScene(Allocator* allocator)
{
    auto scene = (Scene*)CreateObject(allocator, sizeof(SceneImpl), TYPE_SCENE);
    auto impl = Impl(scene);
    impl->root = CreateRootEntity(allocator, scene);
    InitRenderEntityList(impl->render_entities);
    return scene;
}

void RemoveRenderNode(Scene* scene, Entity* entity)
{
    auto impl = Impl(scene);
    if (IsInList(impl->render_entities, entity))
        Remove(impl->render_entities, entity);
}

void UpdateRenderNode(Scene* scene, Entity* entity)
{
    auto impl = Impl(scene);
    auto should_render = IsEnabled(entity);
    auto is_rendering = IsInList(impl->render_entities, entity);
    if (should_render && !is_rendering)
        PushBack(impl->render_entities, entity);
    else if (!should_render && is_rendering)
        Remove(impl->render_entities, entity);
}

void SetCamera(Scene* scene, Camera* camera)
{
    auto impl = Impl(scene);
    impl->camera = camera;
}

Entity* GetRoot(Scene* scene)
{
    return Impl(scene)->root;
}

void Render(Scene* scene)
{
    auto impl = Impl(scene);
    BindCamera(impl->camera);

    // Render all entities in the list.
    for (auto entity = (Entity*)GetFront(impl->render_entities); entity; entity = (Entity*)GetNext(impl->render_entities, entity))
        GetEntityTraits(GetType(entity))->render(entity, impl->camera);
}

void InitScene()
{
    InitEntity();
    InitMeshRenderer();
}

void ShutdownScene()
{
}
