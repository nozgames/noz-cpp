//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

Mesh** MESH = nullptr;
int MESH_COUNT = 0;

struct MeshImpl : Mesh {
    u16 vertex_count;
    u16 index_count;
    PlatformBuffer* vertex_buffer;
    PlatformBuffer* index_buffer;
    MeshVertex* vertices;
    u16* indices;
    Bounds2 bounds;
};

void UploadMesh(Mesh* mesh);

Mesh* GetMesh(const Name* name) {
    if (!name)
        return nullptr;

    for (int i = 0; i < MESH_COUNT; i++) {
        Mesh* mesh = MESH[i];
        if (mesh && mesh->name == name)
            return mesh;
    }

    return nullptr;
}

static void NormalizeVertexWeights(MeshVertex* vertices, int vertex_count) {
    for (int vertex_index=0; vertex_index<vertex_count; vertex_index++) {
        MeshVertex& v = vertices[vertex_index];
        float total_weight = v.bone_weights.x + v.bone_weights.y + v.bone_weights.z + v.bone_weights.w;
        if (total_weight < F32_EPSILON) {
            v.bone_weights.x = 1.0f;
            v.bone_weights.y = 0.0f;
            v.bone_weights.z = 0.0f;
            v.bone_weights.w = 0.0f;
        } else {
            v.bone_weights.x /= total_weight;
            v.bone_weights.y /= total_weight;
            v.bone_weights.z /= total_weight;
            v.bone_weights.w /= total_weight;
        }
    }
}

Bounds2 ToBounds(const MeshVertex* vertices, int vertex_count) {
    if (vertex_count == 0)
        return { VEC2_ZERO, VEC2_ZERO };

    Vec2 min_pos = vertices[0].position;
    Vec2 max_pos = min_pos;

    for (size_t i = 1; i < vertex_count; i++) {
        min_pos = Min(min_pos, vertices[i].position);
        max_pos = Max(max_pos, vertices[i].position);
    }

    return Bounds2{min_pos, max_pos};
}

static void MeshDestructor(void* p) {
    MeshImpl* impl = (MeshImpl*)p;

    if (impl->vertex_buffer)
        PlatformFree(impl->vertex_buffer);
    if (impl->index_buffer)
        PlatformFree(impl->index_buffer);

    Free(impl->vertices);
    Free(impl->indices);

    impl->vertex_buffer = nullptr;
    impl->index_buffer = nullptr;
    impl->vertices = nullptr;
    impl->indices = nullptr;
}

inline size_t GetMeshImplSize(size_t vertex_count, size_t index_count) {
    return
        sizeof(MeshImpl) +
        sizeof(MeshVertex) * vertex_count +
        sizeof(u16) * index_count;
}

static MeshImpl* CreateMesh(Allocator* allocator, u16 vertex_count, u16 index_count, const Name* name) {
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

void UploadMesh(Mesh* mesh)
{
    assert(mesh);
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    assert(!impl->vertex_buffer);
    impl->vertex_buffer = PlatformCreateVertexBuffer(
        impl->vertices,
        impl->vertex_count,
        impl->name->value);
    impl->index_buffer = PlatformCreateIndexBuffer(
        impl->indices,
        impl->index_count,
        impl->name->value);
}

u16 GetVertexCount(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->vertex_count;
}

u16 GetIndexCount(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->index_count;
}

const MeshVertex* GetVertices(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->vertices;
}

const u16* GetIndices(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->indices;
}

Bounds2 GetBounds(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->bounds;
}

Vec2 GetSize(Mesh* mesh) {
    return GetSize(GetBounds(mesh));
}

bool OverlapPoint(Mesh* mesh, const Vec2& overlap_point) {
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    if (!Contains(impl->bounds, overlap_point))
        return false;

    for (u16 i = 0; i < impl->index_count; i += 3) {
        const Vec2& v0 = impl->vertices[impl->indices[i + 0]].position;
        const Vec2& v1 = impl->vertices[impl->indices[i + 1]].position;
        const Vec2& v2 = impl->vertices[impl->indices[i + 2]].position;
        if (OverlapPoint(v0, v1, v2, overlap_point, nullptr))
            return true;
    }

    return false;
}

void RenderMesh(Mesh* mesh) {
    assert(mesh);
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    PlatformBindVertexBuffer(impl->vertex_buffer);
    PlatformBindIndexBuffer(impl->index_buffer);
    PlatformDrawIndexed(impl->index_count);
}

bool IsUploaded(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->vertex_buffer != nullptr;
}

Mesh* LoadMesh(Allocator* allocator, Stream* stream, const Name* name) {
    Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);

    MeshImpl* impl = CreateMesh(allocator, vertex_count, index_count, name);
    if (!impl)
        return nullptr;

    impl->bounds = bounds;

    if (impl->vertex_count > 0) {
        ReadBytes(stream, impl->vertices, sizeof(MeshVertex) * impl->vertex_count);
        ReadBytes(stream, impl->indices, sizeof(u16) * impl->index_count);
        UploadMesh(impl);
    }

    return impl;
}

Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name_table;
    return LoadMesh(allocator, stream, name);
}

Mesh* CreateMesh(
    Allocator* allocator,
    u16 vertex_count,
    const MeshVertex* vertices,
    u16 index_count,
    u16* indices,
    const Name* name,
    bool upload) {
    assert(vertices);
    assert(indices);

    if (vertex_count == 0 || index_count == 0)
        return nullptr;

    MeshImpl* mesh = CreateMesh(allocator, vertex_count, index_count, name);
    mesh->bounds = ToBounds(vertices, vertex_count);

    memcpy(mesh->vertices, vertices, sizeof(MeshVertex) * vertex_count);
    memcpy(mesh->indices, indices, sizeof(u16) * index_count);

    NormalizeVertexWeights(mesh->vertices, mesh->vertex_count);

    if (upload)
        UploadMesh(mesh);

    return mesh;
}

#ifdef NOZ_EDITOR

void ReloadMesh(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
    (void)header;
    (void)name_table;

    assert(asset);
    assert(stream);
    MeshImpl* impl = static_cast<MeshImpl*>(asset);

    Free(impl->indices);
    Free(impl->vertices);

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
