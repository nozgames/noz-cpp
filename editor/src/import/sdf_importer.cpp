//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../msdf/msdf.h"
#include "../msdf/Shape.h"
#include "../msdf/Contour.h"
#include "../msdf/Edge.h"
#include "../utils/rect_packer.h"
#include "../view.h"

namespace fs = std::filesystem;
using namespace noz;

struct SdfFaceInfo {
    int face_index;
    Bounds2 bounds;             // Face bounding box with padding
    Vec2Int sdf_size;           // Resolution for this face's SDF
    Vec2 scale;                 // World-to-SDF scale
    Vec2 offset;                // Translation for SDF generation
    rect_packer::BinRect packed_rect;
};

// Lightweight contour that doesn't own its edges (avoids delete in destructor)
struct SdfContour {
    std::vector<msdf::Edge*> edges;
    int winding() {
        if (edges.empty()) return 0;
        double total = 0;
        auto prev = edges.back()->point(1.0);
        for (auto edge : edges) {
            auto cur = edge->point(0.0);
            total += (cur.x - prev.x) * (cur.y + prev.y);
            prev = edge->point(1.0);
        }
        total += (edges.front()->point(0.0).x - prev.x) * (edges.front()->point(0.0).y + prev.y);
        return total > 0 ? 1 : -1;
    }
};

// Lightweight shape for SDF generation
struct SdfShape {
    std::vector<SdfContour*> contours;
    bool inverseYAxis = false;
};

// Reusable SDF generation context to avoid repeated allocations
struct SdfGenContext {
    SdfShape shape;
    SdfContour contour;
    std::vector<msdf::LinearEdge> edges;
    std::vector<uint8_t> temp_buffer;

    SdfGenContext() {
        edges.reserve(128);  // Pre-allocate for typical face
        shape.contours.push_back(&contour);
        shape.inverseYAxis = false;
    }

    void setupForFace(MeshData* mesh, int face_index) {
        FaceData& face = mesh->faces[face_index];

        // Clear and rebuild edges (LinearEdge has no default constructor)
        edges.clear();
        contour.edges.clear();
        edges.reserve(face.vertex_count);

        // Set up edges
        for (int i = 0; i < face.vertex_count; i++) {
            int v0_idx = face.vertices[i];
            int v1_idx = face.vertices[(i + 1) % face.vertex_count];

            Vec2 p0 = mesh->vertices[v0_idx].position;
            Vec2 p1 = mesh->vertices[v1_idx].position;

            edges.emplace_back(Vec2Double{p0.x, p0.y}, Vec2Double{p1.x, p1.y});
        }

        // Build contour edge pointers (after all edges added to avoid reallocation)
        for (size_t i = 0; i < edges.size(); i++) {
            contour.edges.push_back(&edges[i]);
        }
    }
};

// Hardcoded SDF generation settings
static constexpr float SDF_PIXELS_PER_UNIT = 32.0f;
static constexpr float SDF_RANGE = 0.25f;
static constexpr int SDF_MIN_RESOLUTION = 16;

// Optimized SDF generation using lightweight structures
static void generateSdfOptimized(
    std::vector<uint8_t>& output,
    int outputStride,
    const Vec2Int& outputPosition,
    const Vec2Int& outputSize,
    const SdfShape& shape,
    double range,
    const Vec2Double& scale,
    const Vec2Double& translate)
{
    int contourCount = (int)shape.contours.size();
    int w = outputSize.x;
    int h = outputSize.y;

    std::vector<int> windings(contourCount);
    for (int i = 0; i < contourCount; i++)
        windings[i] = shape.contours[i]->winding();

    std::vector<double> contourSD(contourCount);

    for (int y = 0; y < h; ++y) {
        int row = shape.inverseYAxis ? h - y - 1 : y;
        for (int x = 0; x < w; ++x) {
            double dummy = 0.0;
            auto p = Vec2Double(x + .5, y + .5) / scale - translate;
            auto negDist = -msdf::SignedDistance::Infinite.distance;
            auto posDist = msdf::SignedDistance::Infinite.distance;
            int winding = 0;

            for (int i = 0; i < contourCount; i++) {
                auto minDistance = msdf::SignedDistance::Infinite;
                for (auto edge : shape.contours[i]->edges) {
                    auto distance = edge->distance(p, dummy);
                    if (distance < minDistance)
                        minDistance = distance;
                }

                contourSD[i] = minDistance.distance;
                if (windings[i] > 0 && minDistance.distance >= 0 && std::abs(minDistance.distance) < std::abs(posDist))
                    posDist = minDistance.distance;
                if (windings[i] < 0 && minDistance.distance <= 0 && std::abs(minDistance.distance) < std::abs(negDist))
                    negDist = minDistance.distance;
            }

            double sd = msdf::SignedDistance::Infinite.distance;
            if (posDist >= 0 && std::abs(posDist) <= std::abs(negDist)) {
                sd = posDist;
                winding = 1;
                for (int i = 0; i < contourCount; ++i)
                    if (windings[i] > 0 && contourSD[i] > sd && std::abs(contourSD[i]) < std::abs(negDist))
                        sd = contourSD[i];
            } else if (negDist <= 0 && std::abs(negDist) <= std::abs(posDist)) {
                sd = negDist;
                winding = -1;
                for (int i = 0; i < contourCount; ++i)
                    if (windings[i] < 0 && contourSD[i] < sd && std::abs(contourSD[i]) < std::abs(posDist))
                        sd = contourSD[i];
            }

            for (int i = 0; i < contourCount; ++i)
                if (windings[i] != winding && std::abs(contourSD[i]) < std::abs(sd))
                    sd = contourSD[i];

            sd /= (range * 2.0);
            sd = Clamp(sd, -0.5, 0.5);
            sd = sd + 0.5;

            output[x + outputPosition.x + (row + outputPosition.y) * outputStride] = (uint8_t)(sd * 255.0);
        }
    }
}

// Calculate resolution and bounds for a face
static SdfFaceInfo CalculateFaceInfo(MeshData* mesh, int face_index) {
    SdfFaceInfo info;
    info.face_index = face_index;

    FaceData& face = mesh->faces[face_index];

    // Calculate bounds
    if (face.vertex_count == 0) {
        info.bounds = BOUNDS2_ZERO;
        info.sdf_size = {SDF_MIN_RESOLUTION, SDF_MIN_RESOLUTION};
        info.scale = {1.0f, 1.0f};
        info.offset = VEC2_ZERO;
        return info;
    }

    info.bounds.min = mesh->vertices[face.vertices[0]].position;
    info.bounds.max = info.bounds.min;

    for (int i = 1; i < face.vertex_count; i++) {
        Vec2 pos = mesh->vertices[face.vertices[i]].position;
        info.bounds.min = Min(info.bounds.min, pos);
        info.bounds.max = Max(info.bounds.max, pos);
    }

    // Add SDF range padding
    info.bounds.min -= Vec2{SDF_RANGE, SDF_RANGE};
    info.bounds.max += Vec2{SDF_RANGE, SDF_RANGE};

    // Calculate resolution proportional to face size
    Vec2 size = GetSize(info.bounds);
    int w = Max(SDF_MIN_RESOLUTION, (int)(size.x * SDF_PIXELS_PER_UNIT));
    int h = Max(SDF_MIN_RESOLUTION, (int)(size.y * SDF_PIXELS_PER_UNIT));

    // Round up to multiple of 4 for better packing
    w = (w + 3) & ~3;
    h = (h + 3) & ~3;

    info.sdf_size = {w, h};
    info.scale = {(float)w / size.x, (float)h / size.y};
    info.offset = -info.bounds.min;

    return info;
}

// Generate SDF for a face directly into the atlas (single-channel R8)
static void GenerateFaceSdf(
    SdfGenContext& ctx,
    MeshData* mesh,
    const SdfFaceInfo& info,
    std::vector<uint8_t>& output,
    int atlas_width)
{
    FaceData& face = mesh->faces[info.face_index];
    if (face.vertex_count < 3)
        return;

    // Set up shape from face (reuses allocations)
    ctx.setupForFace(mesh, info.face_index);

    int w = info.sdf_size.x;
    int h = info.sdf_size.y;

    // Ensure temp buffer is large enough (reuses memory)
    size_t needed = w * h;
    if (ctx.temp_buffer.size() < needed)
        ctx.temp_buffer.resize(needed);

    // Generate SDF into temp buffer using optimized function
    // Note: generateSDF divides by range*2, so pass range*0.5 to get proper [-range, +range] mapping
    generateSdfOptimized(
        ctx.temp_buffer,
        w,
        {0, 0},
        info.sdf_size,
        ctx.shape,
        (double)SDF_RANGE * 0.5,
        {(double)info.scale.x, (double)info.scale.y},
        {(double)info.offset.x, (double)info.offset.y}
    );

    // Copy to atlas (single channel R8 format) with inversion
    int out_x = info.packed_rect.x;
    int out_y = info.packed_rect.y;

    for (int y = 0; y < h; y++) {
        uint8_t* src_row = &ctx.temp_buffer[y * w];
        uint8_t* dst_row = &output[(out_y + y) * atlas_width + out_x];
        for (int x = 0; x < w; x++) {
            // Invert SDF so that inside = high values, outside = low values
            dst_row[x] = 255 - src_row[x];
        }
    }
}

static void ImportSdf(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_SDF);
    SdfData* sdf = static_cast<SdfData*>(a);
    MeshData* mesh = &sdf->mesh;

    if (mesh->face_count == 0)
        return;

    // Calculate face info for all faces
    std::vector<SdfFaceInfo> face_infos;
    face_infos.reserve(mesh->face_count);

    for (int i = 0; i < mesh->face_count; i++) {
        face_infos.push_back(CalculateFaceInfo(mesh, i));
    }

    // Pack into atlas
    int initial_size = 256;
    rect_packer packer(initial_size, initial_size);

    bool all_packed = false;
    while (!all_packed) {
        all_packed = true;
        for (int i = 0; i < mesh->face_count; i++) {
            SdfFaceInfo& info = face_infos[i];
            if (-1 == packer.Insert(info.sdf_size, rect_packer::method::BestLongSideFit, info.packed_rect)) {
                // Resize and restart
                rect_packer::BinSize bin_size = packer.size();
                if (bin_size.w <= bin_size.h)
                    bin_size.w <<= 1;
                else
                    bin_size.h <<= 1;
                packer.Resize(bin_size.w, bin_size.h);
                all_packed = false;
                break;
            }
        }
    }

    // Allocate atlas (R8 single-channel)
    Vec2Int atlas_size = {packer.size().w, packer.size().h};
    std::vector<uint8_t> atlas_data;
    atlas_data.resize(atlas_size.x * atlas_size.y, 0);

    // Generate SDF for each face using reusable context
    SdfGenContext ctx;
    for (int i = 0; i < mesh->face_count; i++) {
        GenerateFaceSdf(ctx, mesh, face_infos[i], atlas_data, atlas_size.x);
    }

    // Generate quad mesh
    u16 vertex_count = (u16)(mesh->face_count * 4);
    u16 index_count = (u16)(mesh->face_count * 6);

    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, vertex_count, index_count);

    // Calculate overall bounds
    Bounds2 overall_bounds = face_infos[0].bounds;
    for (int i = 1; i < mesh->face_count; i++) {
        overall_bounds.min = Min(overall_bounds.min, face_infos[i].bounds.min);
        overall_bounds.max = Max(overall_bounds.max, face_infos[i].bounds.max);
    }

    for (int i = 0; i < mesh->face_count; i++) {
        const SdfFaceInfo& info = face_infos[i];

        // Quad corners in world space (from face bounds)
        Vec2 min_pos = info.bounds.min;
        Vec2 max_pos = info.bounds.max;

        // UV coordinates into atlas (normalized)
        float u0 = (float)info.packed_rect.x / atlas_size.x;
        float v0 = (float)info.packed_rect.y / atlas_size.y;
        float u1 = (float)(info.packed_rect.x + info.packed_rect.w) / atlas_size.x;
        float v1 = (float)(info.packed_rect.y + info.packed_rect.h) / atlas_size.y;

        float depth = (mesh->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);

        // Add 4 vertices for quad
        MeshVertex mv0 = {.position = {min_pos.x, min_pos.y}, .depth = depth, .uv = {u0, v0}};
        MeshVertex mv1 = {.position = {max_pos.x, min_pos.y}, .depth = depth, .uv = {u1, v0}};
        MeshVertex mv2 = {.position = {max_pos.x, max_pos.y}, .depth = depth, .uv = {u1, v1}};
        MeshVertex mv3 = {.position = {min_pos.x, max_pos.y}, .depth = depth, .uv = {u0, v1}};

        u16 base = GetVertexCount(builder);
        AddVertex(builder, mv0);
        AddVertex(builder, mv1);
        AddVertex(builder, mv2);
        AddVertex(builder, mv3);

        // Two triangles for quad
        AddTriangle(builder, base + 0, base + 1, base + 2);
        AddTriangle(builder, base + 0, base + 2, base + 3);
    }

    Mesh* out_mesh = CreateMesh(ALLOCATOR_SCRATCH, builder, GetName("sdf_mesh"), false);
    PopScratch();

    // Write the asset file
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_SDF;
    header.version = 1;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    WriteAssetHeader(stream, &header);

    // Write bounds
    WriteStruct(stream, overall_bounds);

    // Write face count and color indices
    WriteU16(stream, (u16)mesh->face_count);
    for (int i = 0; i < mesh->face_count; i++) {
        WriteU8(stream, (u8)mesh->faces[i].color);
    }

    // Write mesh data
    SerializeMesh(out_mesh, stream);

    // Write atlas (R8 single-channel)
    WriteU8(stream, (u8)TEXTURE_FORMAT_R8);
    WriteU32(stream, (u32)atlas_size.x);
    WriteU32(stream, (u32)atlas_size.y);
    WriteBytes(stream, atlas_data.data(), (u32)atlas_data.size());

    SaveStream(stream, path);
    Free(stream);
}

AssetImporter GetSdfImporter() {
    return {
        .type = ASSET_TYPE_SDF,
        .ext = ".sdf",
        .import_func = ImportSdf
    };
}

extern Shader* GetSdfShader();

// Per-face mesh data for worker thread
struct SdfFaceMeshData {
    std::vector<MeshVertex> vertices;
    std::vector<u16> indices;
    int color;
};

// Result data passed from worker thread to main thread
struct SdfPreviewResult {
    SdfData* sdf;
    std::vector<uint8_t> atlas_data;
    std::vector<SdfFaceMeshData> face_meshes;  // Per-face mesh data with colors
    Vec2Int atlas_size;
};

void GenerateSdfPreview(SdfData* sdf) {
    MeshData* mesh = &sdf->mesh;

    if (mesh->face_count == 0) {
        sdf->preview_dirty = false;
        return;
    }

    // Cancel any existing preview task
    if (sdf->preview_task) {
        Cancel(sdf->preview_task);
        sdf->preview_task = TASK_NULL;
    }

    // Create result struct with SDF pointer
    SdfPreviewResult* result = new SdfPreviewResult();
    result->sdf = sdf;

    // Create and start the task
    sdf->preview_task = CreateTask({
        // Worker thread: Generate SDF atlas and mesh data
        .run = [result](Task task) -> void* {
            SdfData* sdf = result->sdf;
            MeshData* mesh = &sdf->mesh;

            // Calculate face info for all faces
            std::vector<SdfFaceInfo> face_infos;
            face_infos.reserve(mesh->face_count);

            for (int i = 0; i < mesh->face_count; i++) {
                if (IsCancelled(task)) return nullptr;
                face_infos.push_back(CalculateFaceInfo(mesh, i));
            }

            // Pack into atlas
            int initial_size = 256;
            rect_packer packer(initial_size, initial_size);

            bool all_packed = false;
            while (!all_packed) {
                all_packed = true;
                for (int i = 0; i < mesh->face_count; i++) {
                    SdfFaceInfo& info = face_infos[i];
                    if (-1 == packer.Insert(info.sdf_size, rect_packer::method::BestLongSideFit, info.packed_rect)) {
                        rect_packer::BinSize bin_size = packer.size();
                        if (bin_size.w <= bin_size.h)
                            bin_size.w <<= 1;
                        else
                            bin_size.h <<= 1;
                        packer.Resize(bin_size.w, bin_size.h);
                        all_packed = false;
                        break;
                    }
                }
            }

            if (IsCancelled(task)) return nullptr;

            // Allocate atlas (R8 single-channel)
            result->atlas_size = {packer.size().w, packer.size().h};
            result->atlas_data.resize(result->atlas_size.x * result->atlas_size.y, 0);

            // Generate SDF for each face using reusable context
            SdfGenContext ctx;
            for (int i = 0; i < mesh->face_count; i++) {
                if (IsCancelled(task)) return nullptr;
                GenerateFaceSdf(ctx, mesh, face_infos[i], result->atlas_data, result->atlas_size.x);
            }

            // Generate per-face mesh data with colors
            result->face_meshes.resize(mesh->face_count);

            for (int i = 0; i < mesh->face_count; i++) {
                const SdfFaceInfo& info = face_infos[i];
                SdfFaceMeshData& face_mesh = result->face_meshes[i];

                // Store color from face
                face_mesh.color = mesh->faces[i].color;

                Vec2 min_pos = info.bounds.min;
                Vec2 max_pos = info.bounds.max;

                float u0 = (float)info.packed_rect.x / result->atlas_size.x;
                float v0 = (float)info.packed_rect.y / result->atlas_size.y;
                float u1 = (float)(info.packed_rect.x + info.packed_rect.w) / result->atlas_size.x;
                float v1 = (float)(info.packed_rect.y + info.packed_rect.h) / result->atlas_size.y;

                float depth = (mesh->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH);

                // Color UV stored in normal field (color_index, palette_index)
                Vec2 color_uv = {(float)face_mesh.color, (float)mesh->palette};

                // 4 vertices per face quad - normal field repurposed for color UV
                face_mesh.vertices.push_back({.position = {min_pos.x, min_pos.y}, .depth = depth, .uv = {u0, v0}, .normal = color_uv});
                face_mesh.vertices.push_back({.position = {max_pos.x, min_pos.y}, .depth = depth, .uv = {u1, v0}, .normal = color_uv});
                face_mesh.vertices.push_back({.position = {max_pos.x, max_pos.y}, .depth = depth, .uv = {u1, v1}, .normal = color_uv});
                face_mesh.vertices.push_back({.position = {min_pos.x, max_pos.y}, .depth = depth, .uv = {u0, v1}, .normal = color_uv});

                // 6 indices for 2 triangles
                face_mesh.indices.push_back(0);
                face_mesh.indices.push_back(1);
                face_mesh.indices.push_back(2);
                face_mesh.indices.push_back(0);
                face_mesh.indices.push_back(2);
                face_mesh.indices.push_back(3);
            }

            return result;
        },

        // Main thread: Create GPU resources
        .complete = [](Task task, void* result_ptr) {
            (void)task;
            if (!result_ptr) return;  // Task was cancelled

            SdfPreviewResult* result = static_cast<SdfPreviewResult*>(result_ptr);
            SdfData* sdf = result->sdf;

            // Free old resources
            if (sdf->runtime) {
                for (auto& face : sdf->runtime->preview_faces)
                    Free(face.mesh);
                sdf->runtime->preview_faces.clear();
            }
            Free(sdf->preview_atlas);
            Free(sdf->preview_material);

            // Create per-face meshes
            if (sdf->runtime) {
                sdf->runtime->preview_faces.reserve(result->face_meshes.size());
                for (const auto& face_data : result->face_meshes) {
                    PushScratch();
                    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, (u16)face_data.vertices.size(), (u16)face_data.indices.size());
                    for (const auto& v : face_data.vertices)
                        AddVertex(builder, v);
                    for (size_t i = 0; i < face_data.indices.size(); i += 3)
                        AddTriangle(builder, face_data.indices[i], face_data.indices[i+1], face_data.indices[i+2]);

                    Mesh* mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
                    PopScratch();

                    sdf->runtime->preview_faces.push_back({.mesh = mesh, .color = face_data.color});
                }
            }

            // Create preview atlas texture (R8 single-channel)
            sdf->preview_atlas = CreateTexture(
                ALLOCATOR_DEFAULT,
                result->atlas_data.data(),
                result->atlas_size.x,
                result->atlas_size.y,
                TEXTURE_FORMAT_R8,
                NAME_NONE
            );

            // Create preview material with SDF shader
            Shader* shader = GetSdfShader();
            if (shader) {
                sdf->preview_material = CreateMaterial(ALLOCATOR_DEFAULT, shader);
                SetTexture(sdf->preview_material, sdf->preview_atlas, 0);  // SDF atlas
                if (g_view.palette_texture) {
                    SetTexture(sdf->preview_material, g_view.palette_texture, 1);  // Palette
                }
            }

            sdf->preview_dirty = false;
            sdf->preview_task = TASK_NULL;
        },

        // Cleanup
        .destroy = [](void* result_ptr) {
            delete static_cast<SdfPreviewResult*>(result_ptr);
        }
    });
}
