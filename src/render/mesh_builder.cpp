//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct MeshBuilderImpl : MeshBuilder
{
    Vec2* positions;
    Vec3* normals;
    Vec2* uv0;
    uint16_t* indices;
    size_t vertex_count;
    size_t vertex_max;
    size_t index_count;
    size_t index_max;
    bool is_full;
};

void MeshBuilderDestructor(void* builder)
{
    assert(builder);
    MeshBuilderImpl* impl = (MeshBuilderImpl*)builder;
    Free(impl->positions);
    Free(impl->normals);
    Free(impl->uv0);
    Free(impl->indices);
}

MeshBuilder* CreateMeshBuilder(Allocator* allocator, int max_vertices, int max_indices)
{
    MeshBuilderImpl* impl = (MeshBuilderImpl*)Alloc(allocator, sizeof(MeshBuilderImpl), MeshBuilderDestructor);
    if (!impl)
        return nullptr;

    impl->vertex_max = max_vertices;
    impl->index_max = max_indices;
    impl->vertex_count = 0;
    impl->index_count = 0;
    
    // TODO: do block alloc instead with the object above
    impl->positions = (Vec2*)Alloc(allocator, sizeof(Vec2) * max_vertices);
    impl->normals = (Vec3*)Alloc(allocator, sizeof(Vec3) * max_vertices);
    impl->uv0 = (Vec2*)Alloc(allocator, sizeof(Vec2) * max_vertices);
    impl->indices = (u16*)Alloc(allocator, sizeof(u16) * max_indices);
    
    if (!impl->positions || !impl->normals || !impl->uv0 || !impl->indices)
    {
        Free(impl);
        return nullptr;
    }
    
    return impl;
}

void Clear(MeshBuilder* builder)
{
    if (!builder) return;
    
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->vertex_count = 0;
    impl->index_count = 0;
}

const Vec2* GetPositions(MeshBuilder* builder)
{
    return static_cast<MeshBuilderImpl*>(builder)->positions;
}

const Vec3* GetNormals(MeshBuilder* builder)
{
    return static_cast<MeshBuilderImpl*>(builder)->normals;
}

const Vec2* GetUvs(MeshBuilder* builder)
{
    return static_cast<MeshBuilderImpl*>(builder)->uv0;
}

const u16* GetIndices(MeshBuilder* builder)
{
    return static_cast<MeshBuilderImpl*>(builder)->indices;
}

u32 GetVertexCount(MeshBuilder* builder)
{
    return static_cast<MeshBuilderImpl*>(builder)->vertex_count;
}

u32 GetIndexCount(MeshBuilder* builder)
{
    return static_cast<MeshBuilderImpl*>(builder)->index_count;
}

void AddVertex(
    MeshBuilder* builder,
    const Vec2& position,
    const Vec3& normal,
    const Vec2& uv,
    uint8_t bone_index)
{
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
    impl->positions[index] = position;
    impl->normals[index] = normal;
    impl->uv0[index] = uv;
}

void AddVertex(MeshBuilder* builder, const Vec2& position)
{
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
    impl->positions[index] = position;
    impl->normals[index] = VEC3_FORWARD;
    impl->uv0[index] = VEC2_ZERO;
}

void AddIndex(MeshBuilder* builder, uint16_t index)
{
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && impl->index_count + 1 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count] = index;
    impl->index_count++;    
}

void AddTriangle(MeshBuilder* builder, uint16_t a, uint16_t b, uint16_t c)
{
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
    const Vec2& a,
    const Vec2& b,
    const Vec2& c,
    u8 bone_index)
{
    // Calculate face normal
    Vec2 v1 = b - a;
    Vec2 v2 = c - a;
    Vec3 normal = Normalize(Cross(Vec3{v2.x, v2.y, 0}, Vec3{v1.x, v1.y, 0}));

    // Add vertices with computed normal
    uint16_t vertex_index = (uint16_t)static_cast<MeshBuilderImpl*>(builder)->vertex_count;
    AddVertex(builder, a, normal, { 0.0f, 0.0f }, bone_index);
    AddVertex(builder, a, normal, { 1.0f, 0.0f }, bone_index);
    AddVertex(builder, a, normal, { 0.5f, 1.0f }, bone_index);
    AddTriangle(builder, vertex_index, vertex_index + 1, vertex_index + 2);
}

void AddQuad(
    MeshBuilder* builder,
    const Vec2& forward,
    const Vec2& right,
    f32 width,
    f32 height,
    const Vec2& color_uv)
{
    f32 hw = width * 0.5f;
    f32 hh = height * 0.5f;
    Vec3 normal = Cross(Vec3{forward.x, forward.y, 0}, Vec3{right.x, right.y, 0});
    Vec2 f = forward * hh;
    Vec2 r = right * hw;
    Vec2 a =  f - r;
    Vec2 b =  f + r;
    Vec2 c = -f + r;
    Vec2 d = -f - r;
    AddQuad(builder, a, b, c, d, color_uv, normal, 0);
}

void AddQuad(
    MeshBuilder* builder,
    const Vec2& a,
    const Vec2& b,
    const Vec2& c,
    const Vec2& d,
    const Vec2& uv_color,
    const Vec3& normal,
    u8 bone_index)
{
    u16 base_index = (u16)static_cast<MeshBuilderImpl*>(builder)->vertex_count;

    // Add vertices
    AddVertex(builder, a, normal, uv_color, bone_index);
    AddVertex(builder, b, normal, uv_color, bone_index);
    AddVertex(builder, c, normal, uv_color, bone_index);
    AddVertex(builder, d, normal, uv_color, bone_index);

    // Add triangles
    AddTriangle(builder, base_index, base_index + 1, base_index + 2);
    AddTriangle(builder, base_index, base_index + 2, base_index + 3);
}


void AddPyramid(MeshBuilder* builder, Vec3 start, Vec3 end, float size, uint8_t bone_index)
{
#if 0
    // Calculate direction and create base
    Vec3 direction = vec3_normalize(vec3_sub(end, start));
    float length = vec3_distance(start, end);

    // Create rotation matrix to align with direction
    Vec3 up = { 0.0f, 1.0f, 0.0f };
    if (fabs(vec3_dot(direction, up)) > 0.9f)
        up = (Vec3){ 1.0f, 0.0f, 0.0f };

    Vec3 right = vec3_normalize(vec3_cross(direction, up));
    up = vec3_normalize(vec3_cross(right, direction));

    auto hsize = size * 0.5f;
    right = vec3_muls(right, hsize);
    up = vec3_muls(up, hsize);

    Vec3 right_sub_up = vec3_sub(right, up);
    Vec3 right_add_up = vec3_add(right, up);

    AddTriangle(
        builder,
        vec3_add(start, right_add_up),
        vec3_add(start, right_sub_up),
        end,
        bone_index);

    AddTriangle(
        builder,
        vec3_sub(start, right_add_up),
        vec3_add(start, right_add_up),
        end,
        bone_index);

    AddTriangle(
        builder,
        vec3_sub(start, right_sub_up),
        vec3_sub(start, right_add_up),
        end,
        bone_index);

    AddTriangle(
        builder,
        vec3_add(start, right_sub_up),
        vec3_sub(start, right_sub_up),
        end,
        bone_index);
#endif
}

void AddRaw(
    MeshBuilder* builder,
    size_t vertex_count,
    const Vec3* positions,
    const Vec3* normals,
    const Vec2* uv0,
    uint8_t bone_index,
    size_t index_count,
    const uint16_t* indices)
{
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    impl->is_full = impl->is_full && (impl->vertex_count + vertex_count >= impl->vertex_max || impl->index_count + index_count >= impl->index_max);
    if (impl->is_full)
        return;

    size_t vertex_start = impl->vertex_count;
    memcpy(impl->positions + impl->vertex_count, positions, sizeof(Vec3) * vertex_count);
    memcpy(impl->normals + impl->vertex_count, normals, sizeof(Vec3) * vertex_count);
    memcpy(impl->uv0 + impl->vertex_count, uv0, sizeof(Vec2) * vertex_count);

    for (size_t i = 0; i < index_count; ++i)
    {
        impl->indices[impl->index_count] = indices[i] + (uint16_t)vertex_start;
        impl->index_count++;
    }
}

void AddCube(MeshBuilder* builder, Vec3 center, Vec3 size, uint8_t bone_index)
{
#if 0
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);

    Vec3 half_size = vec3_muls(size, 0.5f);

    Vec3 positions[8] = {
        vec3_add(center, (Vec3) { -half_size.x, -half_size.y, -half_size.z }), // 0: bottom-left-back
        vec3_add(center, (Vec3) { half_size.x, -half_size.y, -half_size.z }), // 1: bottom-right-back
        vec3_add(center, (Vec3) { half_size.x,  half_size.y, -half_size.z }), // 2: top-right-back
        vec3_add(center, (Vec3) { -half_size.x,  half_size.y, -half_size.z }), // 3: top-left-back
        vec3_add(center, (Vec3) { -half_size.x, -half_size.y,  half_size.z }), // 4: bottom-left-front
        vec3_add(center, (Vec3) { half_size.x, -half_size.y,  half_size.z }), // 5: bottom-right-front
        vec3_add(center, (Vec3) { half_size.x,  half_size.y,  half_size.z }), // 6: top-right-front
        vec3_add(center, (Vec3) { -half_size.x,  half_size.y,  half_size.z })  // 7: top-left-front
    };

    Vec3 normals[8] = {
        (Vec3) { 0.0f,  0.0f, -1.0f }, // Back face
        (Vec3) { 0.0f,  0.0f, -1.0f },
        (Vec3) { 0.0f,  0.0f, -1.0f },
        (Vec3) { 0.0f,  0.0f, -1.0f },
        (Vec3) { 0.0f,  0.0f,  1.0f }, // Front face
        (Vec3) { 0.0f,  0.0f,  1.0f },
        (Vec3) { 0.0f,  0.0f,  1.0f },
        (Vec3) { 0.0f,  0.0f,  1.0f }
    };

    Vec3 uvs[8] = {
        (Vec3) { 0.0f, 0.0f, 0.0f },
        (Vec3) { 1.0f, 0.0f, 0.0f },
        (Vec3) { 1.0f, 1.0f, 0.0f },
        (Vec3) { 0.0f, 1.0f, 0.0f },
        (Vec3) { 0.0f, 0.0f, 0.0f },
        (Vec3) { 1.0f, 0.0f, 0.0f },
        (Vec3) { 1.0f, 1.0f, 0.0f },
        (Vec3) { 0.0f, 1.0f, 0.0f }
    };

    uint16_t indices[36] = {
        0, 1, 2, 0, 2, 3, // Back face
        5, 4, 7, 5, 7, 6, // Front face
        4, 0, 3, 4, 3, 7, // Left face
        1, 5, 6, 1, 6, 2, // Right face
        3, 2, 6, 3, 6, 7, // Top face
        4, 5, 1, 4, 1, 0  // Bottom face
    };
#endif
}

#if 0

void mesh_builder::add_sphere(const glm::Vec3& center, float radius, int segments, int rings, uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Generate sphere vertices
    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = (glm::pi<float>() * ring) / rings;
        float y = cos(phi);
        float ringRadius = sin(phi);

        for (int segment = 0; segment <= segments; ++segment)
        {
            float theta = (2.0f * glm::pi<float>() * segment) / segments;
            float x = cos(theta) * ringRadius;
            float z = sin(theta) * ringRadius;

            glm::Vec3 position = center + glm::Vec3(x, y, z) * radius;
            glm::Vec3 normal = glm::normalize(glm::Vec3(x, y, z));
            glm::Vec2 uv = glm::Vec2(static_cast<float>(segment) / segments, static_cast<float>(ring) / rings);

            add_vertex(position, normal, uv, boneIndex);
        }
    }

    // Generate sphere indices
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            int current = baseIndex + ring * (segments + 1) + segment;
            int next = current + 1;
            int below = current + (segments + 1);
            int belowNext = below + 1;

            // Two triangles per quad
            add_triangle(current, below, next);
            add_triangle(next, below, belowNext);
        }
    }
}

void mesh_builder::add_line(const glm::Vec3& start, const glm::Vec3& end, float thickness, uint32_t boneIndex)
{
    // Create a simple line as a thin cylinder
    glm::Vec3 direction = glm::normalize(end - start);
    float length = glm::distance(start, end);

    // Create rotation matrix
    glm::Vec3 up = glm::Vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::Vec3(1.0f, 0.0f, 0.0f);

    glm::Vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    // Create thin cylinder with 4 segments
    const int segments = 4;
    std::vector<uint32_t> startIndices, endIndices;

    // Add start vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::Vec3 offset = right * cos(angle) + up * sin(angle);
        glm::Vec3 position = start + offset * thickness;
        glm::Vec3 normal = glm::normalize(offset);

        startIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(position, normal, glm::Vec2(0.0f, static_cast<float>(i) / segments), boneIndex);
    }

    // Add end vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::Vec3 offset = right * cos(angle) + up * sin(angle);
        glm::Vec3 position = end + offset * thickness;
        glm::Vec3 normal = glm::normalize(offset);

        endIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(position, normal, glm::Vec2(1.0f, static_cast<float>(i) / segments), boneIndex);
    }

    // Create side faces
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;

        // Add quad for side face
        add_quad(
            glm::Vec3(_positions[startIndices[i]]),
            glm::Vec3(_positions[startIndices[next]]),
            glm::Vec3(_positions[endIndices[next]]),
            glm::Vec3(_positions[endIndices[i]]),
            glm::Vec2(0, 0),
            glm::normalize(glm::cross(
                _positions[startIndices[next]] - _positions[startIndices[i]],
                _positions[endIndices[i]] - _positions[startIndices[i]]
            )),
            boneIndex
        );
    }
}

void mesh_builder::add_quad(
    const glm::Vec3& a,
    const glm::Vec3& b,
    const glm::Vec3& c,
    const glm::Vec3& d,
    const glm::Vec2& color,
    const glm::Vec3& normal,
    uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add vertices
    add_vertex(a, normal, color, boneIndex);
    add_vertex(b, normal, color, boneIndex);
    add_vertex(c, normal, color, boneIndex);
    add_vertex(d, normal, color, boneIndex);

    // Add triangles
    add_triangle(baseIndex, baseIndex + 1, baseIndex + 2);
    add_triangle(baseIndex, baseIndex + 2, baseIndex + 3);
}

void mesh_builder::add_quad(const Vec3& forward, const Vec3& right, float width, float height, const Vec2& color_uv)
{
    auto hw = width * 0.5f;
    auto hh = height * 0.5f;
    auto normal = glm::cross(forward, right);
    auto a = right * -hw + forward * hh;
    auto b = right * hw + forward * hh;
    auto c = right * hw + forward * -hh;
    auto d = right * -hw + forward * -hh;
    add_quad(a, b, c, d, color_uv, normal, 0);
}

void mesh_builder::add_triangle(
    const glm::Vec3& a,
    const glm::Vec3& b,
    const glm::Vec3& c,
    const glm::Vec2& color,
    const glm::Vec3& normal,
    uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add vertices
    add_vertex(a, normal, color, boneIndex);
    add_vertex(b, normal, color, boneIndex);
    add_vertex(c, normal, color, boneIndex);

    // Add triangle
    add_triangle(baseIndex, baseIndex + 1, baseIndex + 2);
}

void mesh_builder::add_cylinder(const glm::Vec3& start, const glm::Vec3& end, float radius, const glm::Vec2& colorUV, int segments, uint32_t boneIndex)
{
    glm::Vec3 direction = glm::normalize(end - start);
    float length = glm::distance(start, end);

    // Create rotation matrix to align with direction
    glm::Vec3 up = glm::Vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::Vec3(1.0f, 0.0f, 0.0f);

    glm::Vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Generate vertices for start and end circles
    std::vector<uint32_t> startIndices, endIndices;

    // Add center vertices for caps
    uint32_t startCenterIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(start, -direction, colorUV, boneIndex);

    uint32_t endCenterIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(end, direction, colorUV, boneIndex);

    // Generate circle vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::Vec3 offset = right * cos(angle) + up * sin(angle);
        glm::Vec3 normal = glm::normalize(offset);

        // Start circle vertex
        glm::Vec3 startPos = start + offset * radius;
        startIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(startPos, -direction, colorUV, boneIndex);

        // End circle vertex
        glm::Vec3 endPos = end + offset * radius;
        endIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(endPos, direction, colorUV, boneIndex);
    }

    // Create start cap triangles
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;
        add_triangle(startIndices[i], startIndices[next], startCenterIndex);
    }

    // Create end cap triangles
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;
        //add_triangle(endCenterIndex, endIndices[i], endIndices[next]);
    }

    // Create side faces
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;

        // Calculate normal for this face
        glm::Vec3 v1 = glm::Vec3(_positions[startIndices[next]]) - glm::Vec3(_positions[startIndices[i]]);
        glm::Vec3 v2 = glm::Vec3(_positions[endIndices[i]]) - glm::Vec3(_positions[startIndices[i]]);
        glm::Vec3 faceNormal = -glm::normalize(glm::cross(v1, v2));

        // Add quad for side face
        add_quad(
            glm::Vec3(_positions[endIndices[i]]),
            glm::Vec3(_positions[endIndices[next]]),
            glm::Vec3(_positions[startIndices[next]]),
            glm::Vec3(_positions[startIndices[i]]),
            colorUV,
            faceNormal,
            boneIndex
        );
    }
}

void mesh_builder::add_cone(
    const glm::Vec3& base,
    const glm::Vec3& tip,
    float baseRadius,
    const glm::Vec2& colorUV,
    int segments,
    uint32_t boneIndex)
{
    glm::Vec3 direction = glm::normalize(tip - base);
    float length = glm::distance(base, tip);

    // Create rotation matrix to align with direction
    glm::Vec3 up = glm::Vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::Vec3(1.0f, 0.0f, 0.0f);

    glm::Vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add base center vertex
    uint32_t baseCenterIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(base, -direction, colorUV, boneIndex);

    // Add tip vertex
    uint32_t tipIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(tip, direction, colorUV, boneIndex);

    // Generate base circle vertices
    std::vector<uint32_t> baseIndices;
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::Vec3 offset = right * cos(angle) + up * sin(angle);
        glm::Vec3 basePos = base + offset * baseRadius;

        baseIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(basePos, -direction, colorUV, boneIndex);
    }

    // Create base cap triangles
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;
        add_triangle(baseIndices[i], baseIndices[next], baseCenterIndex);
    }

    // Create side triangles (from base to tip)
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;

        // Calculate normal for this face
        auto v1 = glm::Vec3(_positions[baseIndices[next]]) - glm::Vec3(_positions[baseIndices[i]]);
        auto v2 = glm::Vec3(_positions[tipIndex]) - glm::Vec3(_positions[baseIndices[i]]);
        auto n = -glm::normalize(glm::cross(v1, v2));

        // Add triangle
        add_triangle(
            glm::Vec3(_positions[tipIndex]),
            glm::Vec3(_positions[baseIndices[next]]),
            glm::Vec3(_positions[baseIndices[i]]),
            colorUV,
            n,
            boneIndex
        );
    }
}
#endif

Mesh* CreateMesh(Allocator* allocator, MeshBuilder* builder, const Name* name, bool upload)
{
    assert(builder);
    MeshBuilderImpl* impl = static_cast<MeshBuilderImpl*>(builder);
    return CreateMesh(
        allocator,
        impl->vertex_count,
        impl->positions,
        impl->normals,
        impl->uv0,
        impl->index_count,
        impl->indices,
        name,
        upload
    );
}
