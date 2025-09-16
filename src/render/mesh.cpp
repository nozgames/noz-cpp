//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

struct MeshImpl : Mesh
{
    u16 vertex_count;
    u16 index_count;
    platform::Buffer* vertex_buffer;
    platform::Buffer* index_buffer;
    MeshVertex* vertices;
    u16* indices;
    Bounds2 bounds;
    Texture* texture;
};

static void UploadMesh(MeshImpl* impl);

static void MeshDestructor(void* p)
{
    MeshImpl* impl = (MeshImpl*)p;

    if (impl->vertex_buffer)
        platform::DestroyBuffer(impl->vertex_buffer);
    if (impl->index_buffer)
        platform::DestroyBuffer(impl->index_buffer);

    Free(impl->vertices);
    Free(impl->indices);
}

inline size_t GetMeshImplSize(size_t vertex_count, size_t index_count)
{
    return
        sizeof(MeshImpl) +
        sizeof(MeshVertex) * vertex_count +
        sizeof(u16) * index_count;
}

static MeshImpl* CreateMesh(Allocator* allocator, u16 vertex_count, u16 index_count, const Name* name)
{
    MeshImpl* mesh = (MeshImpl*)Alloc(allocator, sizeof(MeshImpl), MeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name ? name : NAME_NONE;
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->vertices = (MeshVertex*)Alloc(allocator, (u32)(vertex_count * sizeof(MeshVertex)));
    mesh->indices = (u16*)Alloc(allocator, (u32)(index_count * sizeof(u16)));
    return mesh;
}

static void UploadMesh(MeshImpl* impl)
{
    assert(impl);
    assert(!impl->vertex_buffer);
    impl->vertex_buffer = platform::CreateVertexBuffer(
        impl->vertices,
        impl->vertex_count,
        impl->name->value);
    impl->index_buffer = platform::CreateIndexBuffer(
        impl->indices,
        impl->index_count,
        impl->name->value);
}

u16 GetVertexCount(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->vertex_count;
}

u16 GetIndexCount(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->index_count;
}

const MeshVertex* GetVertices(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->vertices;
}

const u16* GetIndices(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->indices;
}

Bounds2 GetBounds(Mesh* mesh)
{
    return static_cast<MeshImpl*>(mesh)->bounds;
}

bool OverlapPoint(Mesh* mesh, const Vec2& overlap_point)
{
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    if (!Contains(impl->bounds, overlap_point))
        return false;

    for (u16 i = 0; i < impl->index_count; i += 3)
    {
        const Vec2& v0 = impl->vertices[impl->indices[i + 0]].position;
        const Vec2& v1 = impl->vertices[impl->indices[i + 1]].position;
        const Vec2& v2 = impl->vertices[impl->indices[i + 2]].position;

        if (OverlapPoint(v0, v1, v2, overlap_point, nullptr))
            return true;
    }

    return false;
}

void RenderMesh(Mesh* mesh)
{
    assert(mesh);
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    platform::BindVertexBuffer(impl->vertex_buffer);
    platform::BindIndexBuffer(impl->index_buffer);
    platform::DrawIndexed(impl->index_count);
}

Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table)
{
    (void)header;
    (void)name_table;

    Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);

    MeshImpl* impl = CreateMesh(allocator, vertex_count, index_count, name);
    if (!impl)
        return nullptr;

    impl->bounds = bounds;

    ReadBytes(stream, impl->vertices, sizeof(MeshVertex) * impl->vertex_count);
    ReadBytes(stream, impl->indices, sizeof(u16) * impl->index_count);

    UploadMesh(impl);
    return impl;
}

Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const Vec2* positions,
    const Vec3* normals,
    const Vec2* uvs,
    u16 index_count,
    u16* indices,
    const Name* name,
    bool upload)
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

    memcpy(mesh->indices, indices, sizeof(u16) * index_count);

    if (upload)
        UploadMesh(mesh);

    return mesh;
}

#ifdef NOZ_EDITOR

void ReloadMesh(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table)
{
    (void)header;
    (void)name_table;

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
    ReadBytes(stream, impl->indices, sizeof(u16) * impl->index_count);

    impl->vertex_buffer = nullptr;
    impl->index_buffer = nullptr;

    UploadMesh(impl);
}

#endif
