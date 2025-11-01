//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct MeshBuilderImpl : MeshBuilder {
    Vec3* positions;
    Vec2* uv0;
    u16* indices;
    u16 vertex_count;
    u16 vertex_max;
    u16 index_count;
    u16 index_max;
    bool is_full;
};

void MeshBuilderDestructor(void* builder) {
    assert(builder);
    MeshBuilderImpl* impl = (MeshBuilderImpl*)builder;
    Free(impl->positions);
    Free(impl->uv0);
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
    impl->positions = (Vec3*)Alloc(allocator, sizeof(Vec3) * max_vertices);
    impl->uv0 = (Vec2*)Alloc(allocator, sizeof(Vec2) * max_vertices);
    impl->indices = (u16*)Alloc(allocator, sizeof(u16) * max_indices);
    
    if (!impl->positions || !impl->uv0 || !impl->indices) {
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

const Vec3* GetPositions(MeshBuilder* builder) {
    return static_cast<MeshBuilderImpl*>(builder)->positions;
}

const Vec2* GetUvs(MeshBuilder* builder) {
    return static_cast<MeshBuilderImpl*>(builder)->uv0;
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

void AddVertex(MeshBuilder* builder, const Vec3& position, const Vec2& uv) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
    impl->positions[index] = position;
    impl->uv0[index] = uv;
}

void AddVertex(MeshBuilder* builder, const Vec3& position) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
    impl->positions[index] = position;
    impl->uv0[index] = VEC2_ZERO;
}

void AddIndex(MeshBuilder* builder, u16 index) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->index_count + 1 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count] = index;
    impl->index_count++;    
}

void AddTriangle(MeshBuilder* builder, u16 a, u16 b, u16 c) {
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->index_count + 3 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count + 0] = a;
    impl->indices[impl->index_count + 1] = b;
    impl->indices[impl->index_count + 2] = c;
    impl->index_count+=3;
}

void AddTriangle(
    MeshBuilder* builder,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c) {
    u16 vertex_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;
    AddVertex(builder, a, { 0.0f, 0.0f });
    AddVertex(builder, b, { 1.0f, 0.0f });
    AddVertex(builder, c, { 0.5f, 1.0f });
    AddTriangle(builder, vertex_index, vertex_index + 1, vertex_index + 2);
}

void AddQuad(
    MeshBuilder* builder,
    const Vec3& forward,
    const Vec3& right,
    f32 width,
    f32 height,
    const Vec2& color_uv) {
    f32 hw = width * 0.5f;
    f32 hh = height * 0.5f;
    Vec3 f = forward * hh;
    Vec3 r = right * hw;
    Vec3 a =  f - r;
    Vec3 b =  f + r;
    Vec3 c = -f + r;
    Vec3 d = -f - r;
    AddQuad(builder, a, b, c, d, color_uv);
}

void AddQuad(
    MeshBuilder* builder,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d,
    const Vec2& uv_color) {
    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;
    AddVertex(builder, a, uv_color);
    AddVertex(builder, b, uv_color);
    AddVertex(builder, c, uv_color);
    AddVertex(builder, d, uv_color);
    AddTriangle(builder, base_index, base_index + 1, base_index + 2);
    AddTriangle(builder, base_index, base_index + 2, base_index + 3);
}

void AddCircle(MeshBuilder* builder, const Vec3& center, f32 radius, int segments, const Vec2& uv_color) {
    if (segments < 3)
        segments = 3;

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;

    AddVertex(builder, center, uv_color);

    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * noz::PI * 2.0f;
        Vec3 offset = { cosf(angle) * radius, sinf(angle) * radius };
        AddVertex(builder, center + offset, uv_color);
    }

    for (int i = 0; i < segments; ++i)
        AddTriangle(builder, base_index, base_index + (u16)i + 1, base_index + (u16)i + 2);
}

void AddCircleStroke(MeshBuilder* builder, const Vec3& center, f32 radius, f32 thickness, int segments, const Vec2& uv_color) {
    segments = Max(segments, 3);

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;
    f32 inner_radius = radius - (thickness * 0.5f);
    f32 outer_radius = radius + (thickness * 0.5f);
    f32 step = 2.0f * noz::PI / (f32)segments;
    for (int i = 0; i <= segments; ++i) {
        f32 angle = i * step;
        Vec3 offset_inner = { cosf(angle) * inner_radius, sinf(angle) * inner_radius };
        Vec3 offset_outer = { cosf(angle) * outer_radius, sinf(angle) * outer_radius };
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

void AddArc(MeshBuilder* builder, const Vec3& center, f32 radius, f32 start, f32 end, int segments, const Vec2& uv_color) {
    segments = Max(segments, 3);

    u16 base_index = static_cast<MeshBuilderImpl*>(builder)->vertex_count;

    AddVertex(builder, center, uv_color);

    f32 step = noz::PI / (f32)(segments - 1);
    f32 angle_start = Radians(start);
    f32 angle_end = Radians(end);

    int actual_segments = 1;
    for (f32 angle = angle_start; angle < angle_end; angle += step, actual_segments++) {
        Vec3 offset = { cosf(angle) * radius, -sinf(angle) * radius };
        AddVertex(builder, center + offset, uv_color);
    }

    Vec3 offset_end = { cosf(angle_end) * radius, -sinf(angle_end) * radius };
    AddVertex(builder, center + offset_end, uv_color);

    for (int i = 0; i < actual_segments; ++i)
        AddTriangle(builder, base_index, base_index + (u16)i + 1, base_index + (u16)i + 2);
}

void AddRaw(
    MeshBuilder* builder,
    u16 vertex_count,
    const Vec3* positions,
    const Vec2* uv0,
    u16 index_count,
    const u16* indices)
{
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && (impl->vertex_count + vertex_count >= impl->vertex_max || impl->index_count + index_count >= impl->index_max);
    if (impl->is_full)
        return;

    size_t vertex_start = impl->vertex_count;
    memcpy(impl->positions + impl->vertex_count, positions, sizeof(Vec3) * vertex_count);
    memcpy(impl->uv0 + impl->vertex_count, uv0, sizeof(Vec2) * vertex_count);

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
        impl->positions,
        impl->uv0,
        impl->index_count,
        impl->indices,
        name,
        upload
    );
}
