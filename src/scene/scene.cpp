//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

static Entity* g_root_entity = nullptr;

void InitEntity();
void InitMeshRenderer();
void RenderEntities(Camera* camera);
void RenderScreenCanvases();
Entity* CreateRootEntity(Allocator* allocator);

void InitScene()
{
    InitEntity();
    InitMeshRenderer();

    g_root_entity = CreateRootEntity(ALLOCATOR_DEFAULT);
}

void ShutdownScene()
{
}

Entity* GetSceneRoot()
{
    return g_root_entity;
}

void RenderScene(Camera* camera)
{
    BindCamera(camera);
    RenderEntities(camera);
    RenderScreenCanvases();
}
