//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

struct MeshImpl : Mesh
{
    size_t vertex_count;
    size_t index_count;
    platform::Buffer* vertex_buffer;
    platform::Buffer* index_buffer;
    MeshVertex* vertices;
    uint16_t* indices;
    Bounds2 bounds;
    Texture* texture;
};

//static SDL_GPUDevice* g_device = nullptr;

static void UploadMesh(MeshImpl* impl);

static void MeshDestructor(void* p)
{
    MeshImpl* impl = (MeshImpl*)p;
    Free(impl->vertices);
    Free(impl->indices);

// todo: destroy vertex bufffers

#if 0
    if (impl->index_transfer)
        SDL_ReleaseGPUTransferBuffer(g_device, impl->index_transfer);

    if (impl->index_buffer)
        SDL_ReleaseGPUBuffer(g_device, impl->index_buffer);

    if (impl->vertex_transfer)
        SDL_ReleaseGPUTransferBuffer(g_device, impl->vertex_transfer);

    if (impl->vertex_buffer)
        SDL_ReleaseGPUBuffer(g_device, impl->vertex_buffer);

    impl->index_buffer = nullptr;
    impl->index_transfer = nullptr;
    impl->vertex_buffer = nullptr;
    impl->vertex_transfer = nullptr;
#endif
}

inline size_t GetMeshImplSize(size_t vertex_count, size_t index_count)
{
    return
        sizeof(MeshImpl) +
        sizeof(MeshVertex) * vertex_count +
        sizeof(uint16_t) * index_count;
}

static MeshImpl* CreateMesh(Allocator* allocator, size_t vertex_count, size_t index_count, const Name* name)
{
    MeshImpl* mesh = (MeshImpl*)Alloc(allocator, sizeof(MeshImpl), MeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name ? name : NAME_NONE;
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->vertices = (MeshVertex*)Alloc(allocator, vertex_count * sizeof(MeshVertex));
    mesh->indices = (u16*)Alloc(allocator, index_count * sizeof(u16));
    return mesh;
}

Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const Vec2* positions,
    const Vec3* normals,
    const Vec2* uvs,
    u16 index_count,
    u16* indices,
    const Name* name)
{
    assert(positions);
    assert(normals);
    assert(uvs);
    assert(indices);

    if (vertex_count == 0 || index_count == 0)
        return nullptr;

    MeshImpl* mesh = CreateMesh(allocator, vertex_count, index_count, name);
    mesh->bounds = ToBounds(positions, vertex_count);

    for (size_t i = 0; i < vertex_count; i++)
    {
        mesh->vertices[i].position = positions[i];
        mesh->vertices[i].normal = normals[i];
        mesh->vertices[i].uv0 = uvs[i];
    }

    memcpy(mesh->indices, indices, sizeof(uint16_t) * index_count);
    UploadMesh(mesh);
    return mesh;
}

Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);

    MeshImpl* impl = CreateMesh(allocator, vertex_count, index_count, name);
    if (!impl)
        return nullptr;

    impl->bounds = bounds;

    ReadBytes(stream, impl->vertices, sizeof(MeshVertex) * impl->vertex_count);
    ReadBytes(stream, impl->indices, sizeof(uint16_t) * impl->index_count);

    UploadMesh(impl);
    return impl;
}

static void UploadMesh(MeshImpl* impl)
{
    assert(impl);
    assert(!impl->vertex_buffer);
    impl->vertex_buffer = platform::CreateVertexBuffer(impl->vertices, impl->vertex_count, impl->name->value);
    impl->index_buffer = platform::CreateIndexBuffer(impl->indices, impl->index_count, impl->name->value);
}

size_t GetVertexCount(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->vertex_count;
}

size_t GetIndexCount(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->index_count;
}

Bounds2 GetBounds(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->bounds;
}

extern void BindTextureInternal(Texture* texture, int slot);

void RenderMesh(Mesh* mesh)
{
    assert(mesh);
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    platform::BindVertexBuffer(impl->vertex_buffer);
    platform::BindIndexBuffer(impl->index_buffer);
    platform::DrawIndexed(impl->index_count);
}

#ifdef NOZ_EDITOR
void ReloadMesh(Asset* asset, Stream* stream)
{
    assert(asset);
    assert(stream);
    MeshImpl* impl = static_cast<MeshImpl*>(asset);

    Free(impl->indices);
    Free(impl->vertices);

    // todo: destroy platform buffers

    impl->bounds = ReadStruct<Bounds2>(stream);
    impl->vertex_count = ReadU16(stream);
    impl->index_count = ReadU16(stream);
    impl->vertices = (MeshVertex*)Alloc(ALLOCATOR_DEFAULT, impl->vertex_count * sizeof(MeshVertex));
    impl->indices = (u16*)Alloc(ALLOCATOR_DEFAULT, impl->index_count * sizeof(u16));

    ReadBytes(stream, impl->vertices, sizeof(MeshVertex) * impl->vertex_count);
    ReadBytes(stream, impl->indices, sizeof(uint16_t) * impl->index_count);

    impl->vertex_buffer = nullptr;
    impl->index_buffer = nullptr;
    UploadMesh(impl);

}
#endif