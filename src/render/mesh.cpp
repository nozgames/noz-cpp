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

static void UploadMesh(MeshImpl* impl, const Name* name);

static void MeshDestructor(void* p)
{
    MeshImpl* impl = (MeshImpl*)p;

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
    MeshImpl* mesh = (MeshImpl*)Alloc(allocator, GetMeshImplSize(vertex_count, index_count), MeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name;
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->vertices = (MeshVertex*)((u8*)mesh + sizeof(MeshImpl));
    mesh->indices = (uint16_t*)((u8*)mesh->vertices + sizeof(MeshVertex) * vertex_count);
    return mesh;
}

Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const Vec2* positions,
    const Vec2* normals,
    const Vec2* uvs,
    u8* bone_indices,
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

    if (bone_indices)
    {
        for (size_t i = 0; i < vertex_count; i++)
        {
            mesh->vertices[i].position = positions[i];
            mesh->vertices[i].normal = normals[i];
            mesh->vertices[i].uv0 = uvs[i];
            mesh->vertices[i].bone = (float)bone_indices[i];
        }
    }
    else
    {
        for (size_t i = 0; i < vertex_count; i++)
        {
            mesh->vertices[i].position = positions[i];
            mesh->vertices[i].normal = normals[i];
            mesh->vertices[i].uv0 = uvs[i];
            mesh->vertices[i].bone = 0;
        }
    }

    memcpy(mesh->indices, indices, sizeof(uint16_t) * index_count);
    UploadMesh(mesh, name);
    return mesh;
}

Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    //Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);
    u32 tex_width = ReadU32(stream);
    u32 tex_height = ReadU32(stream);

    MeshImpl* impl = CreateMesh(allocator, vertex_count, index_count, name);
    if (!impl)
        return nullptr;



//    impl->bounds = bounds;
    ReadBytes(stream, impl->vertices, sizeof(MeshVertex) * impl->vertex_count);
    ReadBytes(stream, impl->indices, sizeof(uint16_t) * impl->index_count);

    void* tex_data = Alloc(ALLOCATOR_SCRATCH, tex_width * tex_height * 4);
    ReadBytes(stream, tex_data, tex_width * tex_height * 4);
    impl->texture = CreateTexture(allocator, tex_data, tex_width, tex_height, TEXTURE_FORMAT_RGBA8, name);
    Free(tex_data);

    UploadMesh(impl, name);
    return impl;
}

static void UploadMesh(MeshImpl* impl, const Name* name)
{
    assert(impl);
    assert(!impl->vertex_buffer);
    impl->vertex_buffer = platform::CreateVertexBuffer(impl->vertices, impl->vertex_count, name ? name->value : nullptr);
    impl->index_buffer = platform::CreateIndexBuffer(impl->indices, impl->index_count, name ? name->value : nullptr);
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
    BindTextureInternal(impl->texture, 0);
    platform::BindColor(COLOR_GREEN);
    platform::DrawIndexed(impl->index_count);
}
