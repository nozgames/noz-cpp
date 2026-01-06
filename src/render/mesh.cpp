//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

Mesh** MESH = nullptr;
int MESH_COUNT = 0;

struct MeshImpl : Mesh {
    // Geometry
    u16 vertex_count;
    u16 index_count;
    MeshVertex* vertices;
    u16* indices;
    Bounds2 bounds;
    PlatformBuffer* vertex_buffer;
    PlatformBuffer* index_buffer;
    // Animation (UV-based)
    int frame_count;
    int frame_rate;
    float frame_rate_inv;
    float duration;
    float frame_width_uv;
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
}

inline size_t GetMeshImplSize(size_t vertex_count, size_t index_count) {
    return
        sizeof(MeshImpl) +
        sizeof(MeshVertex) * vertex_count +
        sizeof(u16) * index_count;
}

static MeshImpl* CreateMesh(Allocator* allocator, const Name* name, u16 vertex_count, u16 index_count) {
    MeshImpl* mesh = (MeshImpl*)Alloc(allocator, sizeof(MeshImpl), MeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name ? name : NAME_NONE;
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->vertices = (MeshVertex*)Alloc(allocator, (u32)(vertex_count * sizeof(MeshVertex)));
    mesh->indices = (u16*)Alloc(allocator, (u32)(index_count * sizeof(u16)));
    mesh->bounds = BOUNDS2_ZERO;
    mesh->vertex_buffer = nullptr;
    mesh->index_buffer = nullptr;
    mesh->frame_count = 1;
    mesh->frame_rate = ANIMATION_FRAME_RATE;
    mesh->frame_rate_inv = 1.0f / static_cast<float>(mesh->frame_rate);
    mesh->duration = 0.0f;
    mesh->frame_width_uv = 0.0f;
    return mesh;
}

void UploadMesh(Mesh* mesh) {
    assert(mesh);
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);

    if (impl->vertex_buffer || impl->vertex_count == 0 || impl->index_count == 0)
        return;

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

// Animation accessors
int GetFrameCount(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->frame_count;
}

int GetFrameRate(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->frame_rate;
}

void SetAnimationInfo(Mesh* mesh, int frame_count, int frame_rate, float frame_width_uv) {
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    impl->frame_count = frame_count;
    impl->frame_rate = frame_rate;
    impl->frame_width_uv = frame_width_uv;
    if (frame_rate > 0) {
        impl->frame_rate_inv = 1.0f / static_cast<float>(frame_rate);
    }
    impl->duration = frame_count * impl->frame_rate_inv;
}

float GetFrameWidthUV(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->frame_width_uv;
}

float GetDuration(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->duration;
}

int GetFrameIndex(Mesh* mesh, float time, bool loop) {
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    if (impl->frame_count <= 1)
        return 0;
    if (loop)
        return FloorToInt(time * static_cast<float>(impl->frame_rate)) % impl->frame_count;
    return Clamp(FloorToInt(time * static_cast<float>(impl->frame_rate)), 0, impl->frame_count - 1);
}

float Update(Mesh* mesh, float current_time, float speed, bool loop) {
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    if (impl->frame_count <= 1)
        return 0.0f;

    current_time += speed * GetFrameTime();
    if (current_time >= impl->duration - F32_EPSILON) {
        if (loop)
            current_time = fmod(current_time, impl->duration);
        else
            current_time = impl->duration - F32_EPSILON;
    }

    return current_time;
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
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    PlatformBindVertexBuffer(impl->vertex_buffer);
    PlatformBindIndexBuffer(impl->index_buffer);
    PlatformDrawIndexed(impl->index_count);
}

void RenderMesh(Mesh* mesh, float time, bool loop) {
    (void)time;
    (void)loop;
    RenderMesh(mesh);
}

bool IsUploaded(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->vertex_buffer != nullptr;
}

Mesh* LoadMesh(Allocator* allocator, Stream* stream, const Name* name) {
    Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);

    MeshImpl* impl = CreateMesh(allocator, name, vertex_count, index_count);
    if (!impl)
        return nullptr;

    impl->bounds = bounds;

    if (vertex_count > 0) {
        ReadBytes(stream, impl->vertices, sizeof(MeshVertex) * vertex_count);
        ReadBytes(stream, impl->indices, sizeof(u16) * index_count);
    }

    // Read animation data
    impl->frame_count = ReadU8(stream);
    impl->frame_rate = ReadU8(stream);
    impl->frame_width_uv = ReadFloat(stream);
    if (impl->frame_rate > 0) {
        impl->frame_rate_inv = 1.0f / static_cast<float>(impl->frame_rate);
    }
    impl->duration = impl->frame_count * impl->frame_rate_inv;

    if (vertex_count > 0)
        UploadMesh(impl);

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

    MeshImpl* mesh = CreateMesh(allocator, name, vertex_count, index_count);
    mesh->bounds = ToBounds(vertices, vertex_count);
    mesh->duration = mesh->frame_rate_inv;

    memcpy(mesh->vertices, vertices, sizeof(MeshVertex) * vertex_count);
    memcpy(mesh->indices, indices, sizeof(u16) * index_count);

    NormalizeVertexWeights(mesh->vertices, mesh->vertex_count);

    if (upload)
        UploadMesh(mesh);

    return mesh;
}

void UpdateMesh(Mesh* mesh, const MeshVertex* vertices, u16 vertex_count, const u16* indices, u16 index_count) {
    assert(mesh);
    assert(vertices);
    assert(indices);

    MeshImpl* impl = static_cast<MeshImpl*>(mesh);

    // If counts match and buffers exist, use fast SubData path
    if (vertex_count == impl->vertex_count &&
        index_count == impl->index_count &&
        impl->vertex_buffer && impl->index_buffer) {
        memcpy(impl->vertices, vertices, sizeof(MeshVertex) * vertex_count);
        memcpy(impl->indices, indices, sizeof(u16) * index_count);

        PlatformUpdateVertexBuffer(impl->vertex_buffer, vertices, vertex_count);
        PlatformUpdateIndexBuffer(impl->index_buffer, indices, index_count);
    } else {
        // Counts differ or no buffers - recreate
        if (impl->vertex_buffer) {
            PlatformFree(impl->vertex_buffer);
            impl->vertex_buffer = nullptr;
        }
        if (impl->index_buffer) {
            PlatformFree(impl->index_buffer);
            impl->index_buffer = nullptr;
        }

        if (vertex_count != impl->vertex_count) {
            Free(impl->vertices);
            impl->vertices = (MeshVertex*)Alloc(ALLOCATOR_DEFAULT, vertex_count * sizeof(MeshVertex));
            impl->vertex_count = vertex_count;
        }
        if (index_count != impl->index_count) {
            Free(impl->indices);
            impl->indices = (u16*)Alloc(ALLOCATOR_DEFAULT, index_count * sizeof(u16));
            impl->index_count = index_count;
        }

        memcpy(impl->vertices, vertices, sizeof(MeshVertex) * vertex_count);
        memcpy(impl->indices, indices, sizeof(u16) * index_count);

        impl->vertex_buffer = PlatformCreateVertexBuffer(vertices, vertex_count, impl->name->value, BUFFER_FLAG_DYNAMIC);
        impl->index_buffer = PlatformCreateIndexBuffer(indices, index_count, impl->name->value, BUFFER_FLAG_DYNAMIC);
    }

    impl->bounds = ToBounds(vertices, vertex_count);
    NormalizeVertexWeights(impl->vertices, impl->vertex_count);
}

#if !defined(NOZ_BUILTIN_ASSETS)

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
