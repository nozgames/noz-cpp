//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

Sdf** SDF = nullptr;
int SDF_COUNT = 0;

struct SdfImpl : Sdf {
    Mesh* mesh;                 // Quad mesh with atlas UVs
    Texture* atlas;             // RGBA8: R=SDF, G=alpha, B=gradient
    u8* color_indices;          // Per-face color (0-63)
    u16 face_count;
    Bounds2 bounds;
};

Sdf* GetSdf(const Name* name) {
    if (!name)
        return nullptr;

    for (int i = 0; i < SDF_COUNT; i++) {
        Sdf* sdf = SDF[i];
        if (sdf && sdf->name == name)
            return sdf;
    }

    return nullptr;
}

Mesh* GetMesh(Sdf* sdf) {
    if (!sdf)
        return nullptr;
    return static_cast<SdfImpl*>(sdf)->mesh;
}

Texture* GetAtlas(Sdf* sdf) {
    if (!sdf)
        return nullptr;
    return static_cast<SdfImpl*>(sdf)->atlas;
}

int GetFaceCount(Sdf* sdf) {
    if (!sdf)
        return 0;
    return static_cast<SdfImpl*>(sdf)->face_count;
}

u8 GetColorIndex(Sdf* sdf, int face_index) {
    if (!sdf)
        return 0;
    SdfImpl* impl = static_cast<SdfImpl*>(sdf);
    if (face_index < 0 || face_index >= impl->face_count)
        return 0;
    return impl->color_indices[face_index];
}

Bounds2 GetBounds(Sdf* sdf) {
    if (!sdf)
        return BOUNDS2_ZERO;
    return static_cast<SdfImpl*>(sdf)->bounds;
}

static void SdfDestructor(void* p) {
    SdfImpl* impl = (SdfImpl*)p;

    Free(impl->mesh);
    Free(impl->atlas);
    Free(impl->color_indices);

    impl->mesh = nullptr;
    impl->atlas = nullptr;
    impl->color_indices = nullptr;
}

static Mesh* LoadMeshFromStream(Allocator* allocator, Stream* stream, const Name* name) {
    Bounds2 bounds = ReadStruct<Bounds2>(stream);
    u16 vertex_count = ReadU16(stream);
    u16 index_count = ReadU16(stream);

    if (vertex_count == 0 || index_count == 0)
        return nullptr;

    MeshVertex* vertices = (MeshVertex*)Alloc(ALLOCATOR_SCRATCH, sizeof(MeshVertex) * vertex_count);
    u16* indices = (u16*)Alloc(ALLOCATOR_SCRATCH, sizeof(u16) * index_count);

    ReadBytes(stream, vertices, sizeof(MeshVertex) * vertex_count);
    ReadBytes(stream, indices, sizeof(u16) * index_count);

    Mesh* mesh = CreateMesh(allocator, vertex_count, vertices, index_count, indices, name, true);

    return mesh;
}

static Texture* LoadTextureFromStream(Allocator* allocator, Stream* stream, const Name* name) {
    TextureFormat format = (TextureFormat)ReadU8(stream);
    u32 width = ReadU32(stream);
    u32 height = ReadU32(stream);

    int bytes_per_pixel = 4; // RGBA8
    if (format == TEXTURE_FORMAT_R8)
        bytes_per_pixel = 1;

    u32 data_size = width * height * bytes_per_pixel;
    u8* data = (u8*)Alloc(ALLOCATOR_SCRATCH, data_size);
    ReadBytes(stream, data, data_size);

    Texture* texture = CreateTexture(allocator, data, width, height, format, name);

    return texture;
}

Asset* LoadSdf(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name_table;

    PushScratch();

    SdfImpl* sdf = (SdfImpl*)Alloc(allocator, sizeof(SdfImpl), SdfDestructor);
    sdf->name = name;
    sdf->type = ASSET_TYPE_SDF;

    // Read bounds
    sdf->bounds = ReadStruct<Bounds2>(stream);

    // Read face count and color indices
    sdf->face_count = ReadU16(stream);
    sdf->color_indices = (u8*)Alloc(allocator, sdf->face_count);
    ReadBytes(stream, sdf->color_indices, sdf->face_count);

    // Read mesh
    sdf->mesh = LoadMeshFromStream(allocator, stream, name);

    // Read atlas texture
    sdf->atlas = LoadTextureFromStream(allocator, stream, name);

    PopScratch();

    return sdf;
}

void DrawSdf(Sdf* sdf, const Mat3& transform) {
    if (!sdf)
        return;

    SdfImpl* impl = static_cast<SdfImpl*>(sdf);
    if (!impl->mesh || !impl->atlas)
        return;

    // Bind the SDF atlas texture
    BindTexture(impl->atlas);
    BindTransform(transform);

    // Draw the quad mesh
    DrawMesh(impl->mesh);
}
