//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

struct MeshBuilderImpl : MeshBuilder {
    MeshVertex* vertices;
    u16* indices;
    u16 vertex_count;
    u16 vertex_max;
    i16 vertex_base;
    u16 index_count;
    u16 index_max;
    bool is_full;
};

void MeshBuilderDestructor(void* builder) {
    assert(builder);
    MeshBuilderImpl* impl = (MeshBuilderImpl*)builder;
    Free(impl->vertices);
    Free(impl->indices);
}

MeshBuilder* CreateMeshBuilder(Allocator* allocator, u16 max_vertices, u16 max_indices) {
    MeshBuilderImpl* impl = (MeshBuilderImpl*)Alloc(allocator, sizeof(MeshBuilderImpl), MeshBuilderDestructor);
    if (!impl)
        return nullptr;

    impl->vertex_max = max_vertices;
    impl->index_max = max_indices;
    impl->vertex_count = 0;
    impl->index_count = 0;
    impl->vertices = static_cast<MeshVertex*>(Alloc(allocator, sizeof(MeshVertex) * max_vertices));
    impl->indices = static_cast<u16*>(Alloc(allocator, sizeof(u16) * max_indices));
    
    if (!impl->vertices || !impl->indices) {
        Free(impl);
        return nullptr;
    }
    
    return impl;
}

void Clear(MeshBuilder* builder) {
    if (!builder) return;
    
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->vertex_count = 0;
    impl->index_count = 0;
}

const MeshVertex* GetVertices(MeshBuilder* builder) {
    return static_cast<MeshBuilderImpl*>(builder)->vertices;
}

const u16* GetIndices(MeshBuilder* builder) {
    return static_cast<MeshBuilderImpl*>(builder)->indices;
}

u16 GetVertexCount(MeshBuilder* builder) {
    return static_cast<MeshBuilderImpl*>(builder)->vertex_count;
}

u16 GetIndexCount(MeshBuilder* builder) {
    return static_cast<MeshBuilderImpl*>(builder)->index_count;
}

void AddVertex(MeshBuilder* builder, const MeshVertex& vertex) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
    impl->vertices[index] = vertex;
}

void AddVertex(MeshBuilder* builder, const Vec2& position, const Vec2& uv, float depth) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
    impl->vertices[index] = {
        .position = position,
        .depth = depth,
        .uv = uv,
        .bone_weights = { 0.0f }
    };
}

void AddVertexWeight(MeshBuilder* builder, int bone_idnex, float weight) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    assert(impl->vertex_count > 0);
    MeshVertex& v = impl->vertices[impl->vertex_count-1];
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; ++i) {
        if (v.bone_weights[i] < F32_EPSILON) {
            v.bone_indices[i] = bone_idnex;
            v.bone_weights[i] = weight;
            return;
        }
    }
}

void AddIndex(MeshBuilder* builder, u16 index) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->index_count + 1 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count] = index;
    impl->index_count++;    
}

void SetBaseVertex(MeshBuilder* builder, u16 base_vertex) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->vertex_base = base_vertex;
}

void SetBaseVertex(MeshBuilder* builder) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->vertex_base = impl->vertex_count;
}

void AddTriangle(MeshBuilder* builder, u16 a, u16 b, u16 c) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->index_count + 3 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count + 0] = a + impl->vertex_base;
    impl->indices[impl->index_count + 1] = b + impl->vertex_base;
    impl->indices[impl->index_count + 2] = c + impl->vertex_base;
    impl->index_count+=3;
}

void AddQuad(
    MeshBuilder* builder,
    const Vec2& forward,
    const Vec2& right,
    f32 width,
    f32 height,
    const Vec2& color_uv) {
    f32 hw = width * 0.5f;
    f32 hh = height * 0.5f;
    Vec2 f = forward * hh;
    Vec2 r = right * hw;
    Vec2 a =  f - r;
    Vec2 b =  f + r;
    Vec2 c = -f + r;
    Vec2 d = -f - r;

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;
    AddVertex(builder, a, color_uv);
    AddVertex(builder, b, color_uv);
    AddVertex(builder, c, color_uv);
    AddVertex(builder, d, color_uv);
    AddTriangle(builder, base_index, base_index + 1, base_index + 2);
    AddTriangle(builder, base_index, base_index + 2, base_index + 3);
}

void AddCircle(MeshBuilder* builder, const Vec2& center, f32 radius, int segments, const Vec2& uv_color) {
    if (segments < 3)
        segments = 3;

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;

    AddVertex(builder, center, uv_color);

    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * noz::PI * 2.0f;
        Vec2 offset = { cosf(angle) * radius, sinf(angle) * radius };
        AddVertex(builder, center + offset, uv_color);
    }

    for (int i = 0; i < segments; ++i)
        AddTriangle(builder, base_index, base_index + (u16)i + 1, base_index + (u16)i + 2);
}

void AddCircleStroke(MeshBuilder* builder, const Vec2& center, f32 radius, f32 thickness, int segments, const Vec2& uv_color) {
    segments = Max(segments, 3);

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;
    f32 inner_radius = radius - (thickness * 0.5f);
    f32 outer_radius = radius + (thickness * 0.5f);
    f32 step = 2.0f * noz::PI / (f32)segments;
    for (int i = 0; i <= segments; ++i) {
        f32 angle = i * step;
        Vec2 offset_inner = { cosf(angle) * inner_radius, sinf(angle) * inner_radius };
        Vec2 offset_outer = { cosf(angle) * outer_radius, sinf(angle) * outer_radius };
        AddVertex(builder, center + offset_inner, uv_color);
        AddVertex(builder, center + offset_outer, uv_color);
    }

    for (int i = 0; i < segments; ++i) {
        u16 i0 = base_index + (u16)(i * 2 + 0);
        u16 i1 = base_index + (u16)(i * 2 + 1);
        u16 i2 = base_index + (u16)(i * 2 + 2);
        u16 i3 = base_index + (u16)(i * 2 + 3);
        AddTriangle(builder, i0, i1, i2);
        AddTriangle(builder, i2, i1, i3);
    }
}

void AddArc(MeshBuilder* builder, const Vec2& center, f32 radius, f32 start, f32 end, int segments, const Vec2& uv_color) {
    segments = Max(segments, 3);

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;

    AddVertex(builder, center, uv_color);

    f32 step = noz::PI / (f32)(segments - 1);
    f32 angle_start = Radians(start);
    f32 angle_end = Radians(end);

    int actual_segments = 1;
    for (f32 angle = angle_start; angle < angle_end; angle += step, actual_segments++) {
        Vec2 offset = { cosf(angle) * radius, -sinf(angle) * radius };
        AddVertex(builder, center + offset, uv_color);
    }

    Vec2 offset_end = { cosf(angle_end) * radius, -sinf(angle_end) * radius };
    AddVertex(builder, center + offset_end, uv_color);

    for (int i = 0; i < actual_segments; ++i)
        AddTriangle(builder, base_index, base_index + (u16)i + 1, base_index + (u16)i + 2);
}

void AddRaw(
    MeshBuilder* builder,
    u16 vertex_count,
    const MeshVertex* vertices,
    u16 index_count,
    const u16* indices)
{
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && (impl->vertex_count + vertex_count >= impl->vertex_max || impl->index_count + index_count >= impl->index_max);
    if (impl->is_full)
        return;

    size_t vertex_start = impl->vertex_count;
    memcpy(impl->vertices + impl->vertex_count, vertices, sizeof(MeshVertex) * vertex_count);

    for (size_t i = 0; i < index_count; ++i) {
        impl->indices[impl->index_count] = indices[i] + (u16)vertex_start;
        impl->index_count++;
    }
}

Mesh* CreateMesh(Allocator* allocator, MeshBuilder* builder, const Name* name, bool upload) {
    assert(builder);
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    return CreateMesh(
        allocator,
        impl->vertex_count,
        impl->vertices,
        impl->index_count,
        impl->indices,
        name,
        upload
    );
}

void UpdateMeshFromBuilder(Mesh* mesh, MeshBuilder* builder) {
    assert(mesh);
    assert(builder);
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    if (impl->vertex_count == 0 || impl->index_count == 0)
        return;
    UpdateMesh(mesh, impl->vertices, impl->vertex_count, impl->indices, impl->index_count);
}
