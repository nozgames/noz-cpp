//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

static Entity* g_root_entity = nullptr;

void InitEntity();
void InitMeshRenderer();
void RenderEntities(Camera* camera);
void RenderScreenCanvases();

void InitScene()
{
    InitEntity();
    InitMeshRenderer();

    g_root_entity = CreateEntity(ALLOCATOR_DEFAULT);
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
