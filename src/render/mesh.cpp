//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

Mesh** MESH = nullptr;
int MESH_COUNT = 0;

// Per-frame mesh data
struct MeshFrame {
    u16 vertex_count;
    u16 index_count;
    PlatformBuffer* vertex_buffer;
    PlatformBuffer* index_buffer;
    MeshVertex* vertices;
    u16* indices;
    Bounds2 bounds;
    u8 hold;
};

// Unified mesh that supports 1-N frames
struct MeshImpl : Mesh {
    int frame_count;        // Always >= 1
    int frame_rate;         // Frames per second (default: ANIMATION_FRAME_RATE)
    float frame_rate_inv;   // 1.0f / frame_rate
    float duration;         // Total animation duration
    Bounds2 bounds;         // Union of all frame bounds
    MeshFrame* frames;      // Array of frames
};

// Legacy accessor macros for single-frame compat
#define SINGLE_FRAME(impl) ((impl)->frames[0])

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

    // Free all frames
    for (int i = 0; i < impl->frame_count; i++) {
        MeshFrame& frame = impl->frames[i];
        if (frame.vertex_buffer)
            PlatformFree(frame.vertex_buffer);
        if (frame.index_buffer)
            PlatformFree(frame.index_buffer);
        Free(frame.vertices);
        Free(frame.indices);
    }

    Free(impl->frames);
    impl->frames = nullptr;
    impl->frame_count = 0;
}

inline size_t GetMeshImplSize(size_t vertex_count, size_t index_count) {
    return
        sizeof(MeshImpl) +
        sizeof(MeshVertex) * vertex_count +
        sizeof(u16) * index_count;
}

// Create a mesh with the given number of frames
static MeshImpl* CreateMesh(Allocator* allocator, int frame_count, const Name* name) {
    MeshImpl* mesh = (MeshImpl*)Alloc(allocator, sizeof(MeshImpl), MeshDestructor);
    if (!mesh)
        return nullptr;

    mesh->name = name ? name : NAME_NONE;
    mesh->frame_count = frame_count;
    mesh->frame_rate = ANIMATION_FRAME_RATE;
    mesh->frame_rate_inv = 1.0f / static_cast<float>(mesh->frame_rate);
    mesh->duration = 0.0f;
    mesh->bounds = BOUNDS2_ZERO;
    mesh->frames = (MeshFrame*)Alloc(allocator, (u32)(frame_count * sizeof(MeshFrame)));
    memset(mesh->frames, 0, frame_count * sizeof(MeshFrame));
    return mesh;
}

// Allocate vertex/index data for a specific frame
static void AllocateFrameData(Allocator* allocator, MeshFrame* frame, u16 vertex_count, u16 index_count) {
    frame->vertex_count = vertex_count;
    frame->index_count = index_count;
    frame->vertices = (MeshVertex*)Alloc(allocator, (u32)(vertex_count * sizeof(MeshVertex)));
    frame->indices = (u16*)Alloc(allocator, (u32)(index_count * sizeof(u16)));
    frame->hold = 0;
}

// Upload all frames to GPU
void UploadMesh(Mesh* mesh) {
    assert(mesh);
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);

    for (int i = 0; i < impl->frame_count; i++) {
        MeshFrame& frame = impl->frames[i];
        if (frame.vertex_buffer || frame.vertex_count == 0 || frame.index_count == 0)
            continue;

        frame.vertex_buffer = PlatformCreateVertexBuffer(
            frame.vertices,
            frame.vertex_count,
            impl->name->value);
        frame.index_buffer = PlatformCreateIndexBuffer(
            frame.indices,
            frame.index_count,
            impl->name->value);
    }
}

// Single-frame accessors (use frame 0)
u16 GetVertexCount(Mesh* mesh) {
    return SINGLE_FRAME(static_cast<MeshImpl*>(mesh)).vertex_count;
}

u16 GetIndexCount(Mesh* mesh) {
    return SINGLE_FRAME(static_cast<MeshImpl*>(mesh)).index_count;
}

const MeshVertex* GetVertices(Mesh* mesh) {
    return SINGLE_FRAME(static_cast<MeshImpl*>(mesh)).vertices;
}

const u16* GetIndices(Mesh* mesh) {
    return SINGLE_FRAME(static_cast<MeshImpl*>(mesh)).indices;
}

Bounds2 GetBounds(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->bounds;
}

// Animation accessors
int GetFrameCount(Mesh* mesh) {
    return static_cast<MeshImpl*>(mesh)->frame_count;
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

    MeshFrame& frame = SINGLE_FRAME(impl);
    for (u16 i = 0; i < frame.index_count; i += 3) {
        const Vec2& v0 = frame.vertices[frame.indices[i + 0]].position;
        const Vec2& v1 = frame.vertices[frame.indices[i + 1]].position;
        const Vec2& v2 = frame.vertices[frame.indices[i + 2]].position;
        if (OverlapPoint(v0, v1, v2, overlap_point, nullptr))
            return true;
    }

    return false;
}

// Render a specific frame
void RenderMeshFrame(MeshImpl* impl, int frame_index) {
    assert(impl);
    assert(frame_index >= 0 && frame_index < impl->frame_count);
    MeshFrame& frame = impl->frames[frame_index];
    PlatformBindVertexBuffer(frame.vertex_buffer);
    PlatformBindIndexBuffer(frame.index_buffer);
    PlatformDrawIndexed(frame.index_count);
}

void RenderMesh(Mesh* mesh) {
    RenderMeshFrame(static_cast<MeshImpl*>(mesh), 0);
}

// Render mesh at specific animation time
void RenderMesh(Mesh* mesh, float time, bool loop) {
    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    int frame_index = GetFrameIndex(mesh, time, loop);
    RenderMeshFrame(impl, frame_index);
}

bool IsUploaded(Mesh* mesh) {
    return SINGLE_FRAME(static_cast<MeshImpl*>(mesh)).vertex_buffer != nullptr;
}

// Load single-frame mesh (legacy format)
Mesh* LoadMesh(Allocator* allocator, Stream* stream, const Name* name) {
    Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);

    MeshImpl* impl = CreateMesh(allocator, 1, name);
    if (!impl)
        return nullptr;

    impl->bounds = bounds;
    impl->duration = impl->frame_rate_inv;  // Single frame duration

    AllocateFrameData(allocator, &impl->frames[0], vertex_count, index_count);
    MeshFrame& frame = impl->frames[0];
    frame.bounds = bounds;

    if (vertex_count > 0) {
        ReadBytes(stream, frame.vertices, sizeof(MeshVertex) * vertex_count);
        ReadBytes(stream, frame.indices, sizeof(u16) * index_count);
        UploadMesh(impl);
    }

    return impl;
}

Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name_table;
    return LoadMesh(allocator, stream, name);
}

// Create single-frame mesh
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

    MeshImpl* mesh = CreateMesh(allocator, 1, name);
    AllocateFrameData(allocator, &mesh->frames[0], vertex_count, index_count);

    MeshFrame& frame = mesh->frames[0];
    frame.bounds = ToBounds(vertices, vertex_count);
    mesh->bounds = frame.bounds;
    mesh->duration = mesh->frame_rate_inv;

    memcpy(frame.vertices, vertices, sizeof(MeshVertex) * vertex_count);
    memcpy(frame.indices, indices, sizeof(u16) * index_count);

    NormalizeVertexWeights(frame.vertices, frame.vertex_count);

    if (upload)
        UploadMesh(mesh);

    return mesh;
}

// Update single-frame mesh (frame 0)
void UpdateMesh(Mesh* mesh, const MeshVertex* vertices, u16 vertex_count, const u16* indices, u16 index_count) {
    assert(mesh);
    assert(vertices);
    assert(indices);

    MeshImpl* impl = static_cast<MeshImpl*>(mesh);
    MeshFrame& frame = SINGLE_FRAME(impl);

    // If counts match and buffers exist, use fast SubData path
    if (vertex_count == frame.vertex_count &&
        index_count == frame.index_count &&
        frame.vertex_buffer && frame.index_buffer) {
        // Update CPU-side data
        memcpy(frame.vertices, vertices, sizeof(MeshVertex) * vertex_count);
        memcpy(frame.indices, indices, sizeof(u16) * index_count);

        // Update GPU buffers via SubData
        PlatformUpdateVertexBuffer(frame.vertex_buffer, vertices, vertex_count);
        PlatformUpdateIndexBuffer(frame.index_buffer, indices, index_count);
    } else {
        // Counts differ or no buffers - recreate
        if (frame.vertex_buffer) {
            PlatformFree(frame.vertex_buffer);
            frame.vertex_buffer = nullptr;
        }
        if (frame.index_buffer) {
            PlatformFree(frame.index_buffer);
            frame.index_buffer = nullptr;
        }

        // Reallocate CPU-side arrays if counts changed
        if (vertex_count != frame.vertex_count) {
            Free(frame.vertices);
            frame.vertices = (MeshVertex*)Alloc(ALLOCATOR_DEFAULT, vertex_count * sizeof(MeshVertex));
            frame.vertex_count = vertex_count;
        }
        if (index_count != frame.index_count) {
            Free(frame.indices);
            frame.indices = (u16*)Alloc(ALLOCATOR_DEFAULT, index_count * sizeof(u16));
            frame.index_count = index_count;
        }

        memcpy(frame.vertices, vertices, sizeof(MeshVertex) * vertex_count);
        memcpy(frame.indices, indices, sizeof(u16) * index_count);

        // Create new buffers with DYNAMIC flag for future updates
        frame.vertex_buffer = PlatformCreateVertexBuffer(vertices, vertex_count, impl->name->value, BUFFER_FLAG_DYNAMIC);
        frame.index_buffer = PlatformCreateIndexBuffer(indices, index_count, impl->name->value, BUFFER_FLAG_DYNAMIC);
    }

    frame.bounds = ToBounds(vertices, vertex_count);
    impl->bounds = frame.bounds;
    NormalizeVertexWeights(frame.vertices, frame.vertex_count);
}

#if !defined(NOZ_BUILTIN_ASSETS)

void ReloadMesh(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
    (void)header;
    (void)name_table;

    assert(asset);
    assert(stream);
    MeshImpl* impl = static_cast<MeshImpl*>(asset);
    MeshFrame& frame = SINGLE_FRAME(impl);

    Free(frame.indices);
    Free(frame.vertices);

    impl->bounds = ReadStruct<Bounds2>(stream);
    frame.vertex_count = ReadU16(stream);
    frame.index_count = ReadU16(stream);
    frame.vertices = (MeshVertex*)Alloc(ALLOCATOR_DEFAULT, frame.vertex_count * sizeof(MeshVertex));
    frame.indices = (u16*)Alloc(ALLOCATOR_DEFAULT, frame.index_count * sizeof(u16));

    ReadBytes(stream, frame.vertices, sizeof(MeshVertex) * frame.vertex_count);
    ReadBytes(stream, frame.indices, sizeof(u16) * frame.index_count);

    frame.vertex_buffer = nullptr;
    frame.index_buffer = nullptr;
    frame.bounds = impl->bounds;

    UploadMesh(impl);
}

#endif
