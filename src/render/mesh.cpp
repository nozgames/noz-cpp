//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct MeshImpl : Object
{
    const name_t* name;
    size_t vertex_count;
    size_t index_count;
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    SDL_GPUTransferBuffer* vertex_transfer;
    SDL_GPUTransferBuffer* index_transfer;
    mesh_vertex* vertices;
    uint16_t* indices;
    bounds3 bounds;
};

static SDL_GPUDevice* g_device = nullptr;

static void UploadMesh(MeshImpl* impl, const name_t* name);
static void mesh_destroy_impl(MeshImpl* impl);
static MeshImpl* Impl(void* s) { return (MeshImpl*)Cast((Object*)s, TYPE_MESH); }

inline size_t GetMeshImplSize(size_t vertex_count, size_t index_count)
{
    return
        sizeof(MeshImpl) +
        sizeof(mesh_vertex) * vertex_count +
        sizeof(uint16_t) * index_count;
}

static Mesh* CreateMesh(Allocator* allocator, size_t vertex_count, size_t index_count)
{
    auto mesh = (Mesh*)CreateObject(allocator, GetMeshImplSize(vertex_count, index_count), TYPE_MESH);
    if (!mesh)
        return nullptr;

    auto impl = Impl(mesh);
    impl->vertex_count = vertex_count;
    impl->index_count = index_count;
    impl->vertices = (mesh_vertex*)((u8*)impl + sizeof(MeshImpl));
    impl->indices = (uint16_t*)((u8*)impl->vertices + sizeof(mesh_vertex) * vertex_count);

    return mesh;
}

Mesh* CreateMesh(
    Allocator* allocator,
    size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uvs,
    u8* bone_indices,
    size_t index_count,
    u16* indices,
    const name_t* name)
{
    assert(positions);
    assert(normals);
    assert(uvs);
    assert(indices);

    if (vertex_count == 0 || index_count == 0)
        return nullptr;

    auto mesh = CreateMesh(allocator, vertex_count, index_count);
    auto impl = Impl(mesh);
    impl->bounds = to_bounds(positions, vertex_count);

    if (bone_indices)
    {
        for (size_t i = 0; i < vertex_count; i++)
        {
            impl->vertices[i].position = positions[i];
            impl->vertices[i].normal = normals[i];
            impl->vertices[i].uv0 = uvs[i];
            impl->vertices[i].bone = (float)bone_indices[i];
        }
    }
    else
    {
        for (size_t i = 0; i < vertex_count; i++)
        {
            impl->vertices[i].position = positions[i];
            impl->vertices[i].normal = normals[i];
            impl->vertices[i].uv0 = uvs[i];
            impl->vertices[i].bone = 0;
        }
    }

    memcpy(impl->indices, indices, sizeof(uint16_t) * index_count);
    UploadMesh(impl, name);
    return mesh;
}

Object* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name)
{
    // Read bounds
    bounds3 bounds = {};
    ReadBytes(stream, &bounds, sizeof(bounds3));

    // counts
    auto vertex_count = ReadU32(stream);
    auto index_count = ReadU32(stream);

    auto mesh = CreateMesh(allocator, vertex_count, index_count);
    if (!mesh)
        return nullptr;

    auto impl = Impl(mesh);
    ReadBytes(stream, impl->vertices, sizeof(mesh_vertex) * impl->vertex_count);
    ReadBytes(stream, impl->indices, sizeof(uint16_t) * impl->index_count);
    UploadMesh(impl, name);

    return mesh;
}

#if 0
static void mesh_destroy_impl(MeshImpl* impl)
{
    if (impl->index_transfer)
        SDL_ReleaseGPUTransferBuffer(g_device, impl->index_transfer);

    if (impl->index_buffer)
        SDL_ReleaseGPUBuffer(g_device, impl->index_buffer);

    if (impl->vertex_transfer)
        SDL_ReleaseGPUTransferBuffer(g_device, impl->vertex_transfer);

    if (impl->vertex_buffer)
        SDL_ReleaseGPUBuffer(g_device, impl->vertex_buffer);    
}
#endif

void DrawMeshGPU(Mesh* mesh, SDL_GPURenderPass* pass)
{
    assert(pass);

    MeshImpl* impl = Impl(mesh);
    if (!impl->vertex_buffer)
        return;

    SDL_GPUBufferBinding vertex_binding = {0};
    vertex_binding.buffer = impl->vertex_buffer;
    vertex_binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);

    SDL_GPUBufferBinding index_binding = {0};
    index_binding.buffer = impl->index_buffer;
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(pass, (uint32_t)impl->index_count, 1, 0, 0, 0);
}

static void UploadMesh(MeshImpl* impl, const name_t* name)
{
    assert(impl);
    assert(!impl->vertex_buffer);
    assert(g_device);

    size_t vertex_count = impl->vertex_count;
    size_t index_count = impl->index_count;

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertex_info = {0};
    vertex_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_info.size = (uint32_t)(sizeof(mesh_vertex) * vertex_count);
    vertex_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(vertex_info.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, name ? name->value : "mesh");
    impl->vertex_buffer = SDL_CreateGPUBuffer(g_device, &vertex_info);
    SDL_DestroyProperties(vertex_info.props);

    SDL_GPUTransferBufferCreateInfo vertex_transfer_info = {};
    vertex_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vertex_transfer_info.size = vertex_info.size;
    vertex_transfer_info.props = 0;
    impl->vertex_transfer = SDL_CreateGPUTransferBuffer(g_device, &vertex_transfer_info);

    SDL_GPUTransferBufferLocation vertex_source = {impl->vertex_transfer, 0};
    SDL_GPUBufferRegion vertex_dest = {impl->vertex_buffer, 0, vertex_info.size};
    void* vertex_mapped = SDL_MapGPUTransferBuffer(g_device, impl->vertex_transfer, false);

    SDL_memcpy(vertex_mapped, impl->vertices, vertex_info.size);
    SDL_UnmapGPUTransferBuffer(g_device, impl->vertex_transfer);

    SDL_GPUCommandBuffer* vertex_upload_cmd = SDL_AcquireGPUCommandBuffer(g_device);
    SDL_GPUCopyPass* vertex_copy_pass = SDL_BeginGPUCopyPass(vertex_upload_cmd);
    SDL_UploadToGPUBuffer(vertex_copy_pass, &vertex_source, &vertex_dest, false);
    SDL_EndGPUCopyPass(vertex_copy_pass);
    SDL_SubmitGPUCommandBuffer(vertex_upload_cmd);

    // Create index buffer
    SDL_GPUBufferCreateInfo index_info = {0};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = (uint32_t)(sizeof(uint16_t) * index_count);
    index_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(vertex_info.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, name ? name->value : "mesh");
    impl->index_buffer = SDL_CreateGPUBuffer(g_device, &index_info);
    SDL_DestroyProperties(index_info.props);

    SDL_GPUTransferBufferCreateInfo index_transfer_info = {};
    index_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    index_transfer_info.size = index_info.size;
    index_transfer_info.props = 0;
    impl->index_transfer = SDL_CreateGPUTransferBuffer(g_device, &index_transfer_info);

    SDL_GPUTransferBufferLocation index_source = {impl->index_transfer, 0};
    SDL_GPUBufferRegion index_dest = {impl->index_buffer, 0, index_info.size};
    void* index_mapped = SDL_MapGPUTransferBuffer(g_device, impl->index_transfer, false);

    SDL_memcpy(index_mapped, impl->indices, index_info.size);
    SDL_UnmapGPUTransferBuffer(g_device, impl->index_transfer);

    SDL_GPUCommandBuffer* index_upload_cmd = SDL_AcquireGPUCommandBuffer(g_device);
    SDL_GPUCopyPass* index_copy_pass = SDL_BeginGPUCopyPass(index_upload_cmd);
    SDL_UploadToGPUBuffer(index_copy_pass, &index_source, &index_dest, false);
    SDL_EndGPUCopyPass(index_copy_pass);
    SDL_SubmitGPUCommandBuffer(index_upload_cmd);
}

size_t GetVertexCount(Mesh* mesh)
{
	return Impl(mesh)->vertex_count;
}

size_t GetIndexCount(Mesh* mesh)
{
    return Impl(mesh)->index_count;
}

bounds3 GetBounds(Mesh* mesh)
{
    return Impl(mesh)->bounds;
}

void InitMesh(RendererTraits* traits, SDL_GPUDevice* device)
{
    g_device = device;
}

void ShutdownMesh()
{
    g_device = nullptr;
}

