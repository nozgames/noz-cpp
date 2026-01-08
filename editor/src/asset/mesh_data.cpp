//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "atlas_manager.h"

constexpr float OUTLINE_WIDTH = 0.015f;

struct SavedCurve { u64 key; Vec2 offset; float weight; };
static SavedCurve g_saved_curves[MESH_MAX_EDGES];
static int g_saved_curve_count = 0;

static void Init(MeshData* m);
extern void InitMeshEditor(MeshData* m);

// Frame management functions
MeshFrameData* GetCurrentFrame(MeshData* m) {
    return &m->impl->frames[m->impl->current_frame];
}

int GetFrameCount(MeshData* m) {
    return m->impl->frame_count;
}

void SetCurrentFrame(MeshData* m, int frame_index) {
    if (frame_index >= 0 && frame_index < m->impl->frame_count)
        m->impl->current_frame = frame_index;
}

void AddFrame(MeshData* m, int after_index) {
    if (m->impl->frame_count >= MESH_MAX_FRAMES)
        return;

    // Shift frames after insertion point
    for (int i = m->impl->frame_count; i > after_index + 1; i--)
        m->impl->frames[i] = m->impl->frames[i - 1];

    // Copy current frame to new slot
    m->impl->frames[after_index + 1] = m->impl->frames[after_index];
    m->impl->frames[after_index + 1].mesh = nullptr;
    m->impl->frames[after_index + 1].outline = nullptr;
    m->impl->frame_count++;
    m->impl->current_frame = after_index + 1;
}

void DeleteFrame(MeshData* m, int frame_index) {
    if (m->impl->frame_count <= 1)
        return;

    Free(m->impl->frames[frame_index].mesh);
    Free(m->impl->frames[frame_index].outline);

    for (int i = frame_index; i < m->impl->frame_count - 1; i++)
        m->impl->frames[i] = m->impl->frames[i + 1];

    m->impl->frame_count--;

    if (m->impl->current_frame >= m->impl->frame_count)
        m->impl->current_frame = m->impl->frame_count - 1;
}

static void DeleteFaceInternal(MeshData* m, int face_index);
static void RemoveFaceVertices(MeshData* m, int face_index, int remove_at, int remove_count);
static void InsertFaceVertices(MeshData* m, int face_index, int insert_at, int count);
static void MergeFaces(MeshData* m, const EdgeData& shared_edge);
static void DeleteFace(MeshData* m, int face_index);
void DeleteVertex(MeshData* m, int vertex_index);
static void TriangulateFace(MeshData* m, FaceData* f, MeshBuilder* builder, float depth);

static int GetFaceEdgeIndex(const FaceData& f, const EdgeData& e) {
    for (int vertex_index=0; vertex_index<f.vertex_count; vertex_index++) {
        int v0 = f.vertices[vertex_index];
        int v1 = f.vertices[(vertex_index + 1) % f.vertex_count];
        if (e.v0 == v0 && e.v1 == v1 || e.v0 == v1 && e.v1 == v0)
            return vertex_index;
    }

    return -1;
}

static void DrawMesh(AssetData* a) {
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData*>(a);

    // TODO: Animation playback preview will work after runtime merge (Phase 3)
    // For now, just draw the current frame
    DrawMesh(m, Translate(a->position));
}

void DrawMesh(MeshData* m, const Mat3& transform, Material* material) {
    // Check if mesh is in an atlas - render as pixel art textured quad
    AtlasRect* rect = nullptr;
    AtlasData* atlas = FindAtlasForMesh(m->name, &rect);
    if (atlas && rect) {
        // Ensure atlas is rendered and texture is uploaded to GPU
        if (!atlas->impl->pixels) {
            RegenerateAtlas(atlas);
        }
        SyncAtlasTexture(atlas);
        if (!atlas->impl->material) goto fallback;
        // Get geometry at exact mesh bounds (matches selection outline)
        Vec2 min, max;
        float u_min, v_min, u_max, v_max;
        GetExportQuadGeometry(atlas, *rect, &min, &max, &u_min, &v_min, &u_max, &v_max);

        // Build textured quad
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
        MeshVertex v = {};
        v.depth = 0.5f;
        v.opacity = 1.0f;

        v.position = min; v.uv = {u_min, v_min}; AddVertex(builder, v);
        v.position = {max.x, min.y}; v.uv = {u_max, v_min}; AddVertex(builder, v);
        v.position = max; v.uv = {u_max, v_max}; AddVertex(builder, v);
        v.position = {min.x, max.y}; v.uv = {u_min, v_max}; AddVertex(builder, v);
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);

        Mesh* quad = CreateMesh(ALLOCATOR_SCRATCH, builder, nullptr, false);
        Free(builder);

        BindMaterial(atlas->impl->material);
        BindColor(COLOR_WHITE);
        DrawMesh(quad, transform);
        return;
    }

fallback:
    // Fall back to vector mesh rendering
    BindMaterial(material ? material : g_view.shaded_material);
    if (g_view.draw_mode == VIEW_DRAW_MODE_WIREFRAME) {
        BindColor(COLOR_EDGE);
        DrawMesh(ToOutlineMesh(m), transform);
    } else {
        BindColor(COLOR_WHITE);
        DrawMesh(ToMesh(m), transform);
    }
}

Vec2 GetFaceCenter(MeshData* m, FaceData* f) {
    (void)m;
    return f->center;
}

Vec2 GetFaceCenter(MeshData* m, int face_index) {
    return GetCurrentFrame(m)->faces[face_index].center;
}

bool IsVertexOnOutsideEdge(MeshData* m, int v0) {
    for (int i = 0; i < GetCurrentFrame(m)->edge_count; i++) {
        EdgeData& ee = GetCurrentFrame(m)->edges[i];
        if (ee.face_count == 1 && (ee.v0 == v0 || ee.v1 == v0))
            return true;
    }

    return false;
}

int GetEdge(MeshData* m, int v0, int v1) {
    int fv0 = Min(v0, v1);
    int fv1 = Max(v0, v1);
    for (int i = 0; i < GetCurrentFrame(m)->edge_count; i++) {
        EdgeData& ee = GetCurrentFrame(m)->edges[i];
        if (ee.v0 == fv0 && ee.v1 == fv1)
            return i;
    }

    return -1;
}

int GetOrAddEdge(MeshData* m, int v0, int v1, int face_index) {
    int fv0 = Min(v0, v1);
    int fv1 = Max(v0, v1);

    for (int i = 0; i < GetCurrentFrame(m)->edge_count; i++) {
        EdgeData& ee = GetCurrentFrame(m)->edges[i];
        if (ee.v0 == fv0 && ee.v1 == fv1) {
            if (ee.face_index[0] > face_index) {
                int temp = ee.face_index[0];
                ee.face_index[0] = face_index;
                ee.face_index[1] = temp;
            } else {
                ee.face_index[ee.face_count] = face_index;
            }

            ee.face_count++;
            return i;
        }
    }

    // Not found - add it
    if (GetCurrentFrame(m)->edge_count >= MAX_EDGES)
        return -1;

    int edge_index = GetCurrentFrame(m)->edge_count++;
    EdgeData& ee = GetCurrentFrame(m)->edges[edge_index];
    ee.face_count = 1;
    ee.face_index[0] = face_index;
    ee.v0 = fv0;
    ee.v1 = fv1;
    ee.normal = Normalize(-Perpendicular(GetCurrentFrame(m)->vertices[v1].position - GetCurrentFrame(m)->vertices[v0].position));
    ee.selected = false;
    ee.curve_offset = VEC2_ZERO;
    ee.curve_weight = 1.0f;

    return edge_index;
}

Vec2 GetEdgeMidpoint(MeshData* m, int edge_index) {
    EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
    Vec2 v0 = GetCurrentFrame(m)->vertices[e.v0].position;
    Vec2 v1 = GetCurrentFrame(m)->vertices[e.v1].position;
    return (v0 + v1) * 0.5f;
}

Vec2 GetEdgeControlPoint(MeshData* m, int edge_index) {
    return GetEdgeMidpoint(m, edge_index) + GetCurrentFrame(m)->edges[edge_index].curve_offset;
}

bool IsEdgeCurved(MeshData* m, int edge_index) {
    Vec2 offset = GetCurrentFrame(m)->edges[edge_index].curve_offset;
    return LengthSqr(offset) > 0.0001f;
}

// Compute centroid using signed area formula (works for concave polygons and holes)
static Vec2 ComputeFaceCentroid(MeshData* m, FaceData& f) {
    if (f.vertex_count < 3)
        return VEC2_ZERO;

    float signed_area = 0.0f;
    Vec2 centroid = VEC2_ZERO;

    for (int i = 0; i < f.vertex_count; i++) {
        Vec2 p0 = GetCurrentFrame(m)->vertices[f.vertices[i]].position;
        Vec2 p1 = GetCurrentFrame(m)->vertices[f.vertices[(i + 1) % f.vertex_count]].position;
        float cross = p0.x * p1.y - p1.x * p0.y;
        signed_area += cross;
        centroid.x += (p0.x + p1.x) * cross;
        centroid.y += (p0.y + p1.y) * cross;
    }

    signed_area *= 0.5f;

    // Handle degenerate faces (zero area)
    if (Abs(signed_area) < F32_EPSILON) {
        // Fall back to simple average
        centroid = VEC2_ZERO;
        for (int i = 0; i < f.vertex_count; i++)
            centroid += GetCurrentFrame(m)->vertices[f.vertices[i]].position;
        return centroid / (float)f.vertex_count;
    }

    float factor = 1.0f / (6.0f * signed_area);
    return centroid * factor;
}

// Quantize position to u64 key for reliable comparison (0.0001 precision)
static u64 QuantizePosition(Vec2 p) {
    i32 x = (i32)(p.x * 10000.0f);
    i32 y = (i32)(p.y * 10000.0f);
    return ((u64)(u32)x << 32) | (u64)(u32)y;
}

// Create edge key from two positions (ordered so key is consistent regardless of direction)
static u64 MakeEdgeKey(Vec2 p0, Vec2 p1) {
    u64 k0 = QuantizePosition(p0);
    u64 k1 = QuantizePosition(p1);
    return k0 < k1 ? (k0 ^ (k1 * 31)) : (k1 ^ (k0 * 31));
}


static void SaveCurves(MeshData* m) {
    MeshFrameData* frame = GetCurrentFrame(m);
    g_saved_curve_count = 0;
    for (int i = 0; i < frame->edge_count; i++) {
        EdgeData& e = frame->edges[i];
        if (LengthSqr(e.curve_offset) > 0.0000001f) {
            Vec2 p0 = frame->vertices[e.v0].position;
            Vec2 p1 = frame->vertices[e.v1].position;
            g_saved_curves[g_saved_curve_count++] = { MakeEdgeKey(p0, p1), e.curve_offset, e.curve_weight };
        }
    }
}

void UpdateEdges(MeshData* m) {
    MeshFrameData* frame = GetCurrentFrame(m);

    // If no pre-saved curves, save them now (edges still have valid indices)
    if (g_saved_curve_count == 0) {
        for (int i = 0; i < frame->edge_count; i++) {
            EdgeData& e = frame->edges[i];
            if (LengthSqr(e.curve_offset) > 0.0000001f) {
                Vec2 p0 = frame->vertices[e.v0].position;
                Vec2 p1 = frame->vertices[e.v1].position;
                g_saved_curves[g_saved_curve_count++] = { MakeEdgeKey(p0, p1), e.curve_offset, e.curve_weight };
            }
        }
    }

    frame->edge_count = 0;

    for (int vertex_index=0; vertex_index < GetCurrentFrame(m)->vertex_count; vertex_index++) {
        GetCurrentFrame(m)->vertices[vertex_index].edge_normal = VEC2_ZERO;
        GetCurrentFrame(m)->vertices[vertex_index].ref_count = 0;
    }

    for (int face_index=0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];

        f.center = ComputeFaceCentroid(m, f);

        for (int vertex_index = 0; vertex_index<f.vertex_count - 1; vertex_index++){
            int v0 = f.vertices[vertex_index];
            int v1 = f.vertices[vertex_index + 1];
            GetOrAddEdge(m, v0, v1, face_index);
        }

        int vs = f.vertices[f.vertex_count - 1];
        int ve = f.vertices[0];
        GetOrAddEdge(m, vs, ve, face_index);
    }

    // Restore curve data to rebuilt edges (match by quantized position key)
    for (int i = 0; i < g_saved_curve_count; i++) {
        const SavedCurve& curve = g_saved_curves[i];
        for (int edge = 0; edge < frame->edge_count; edge++) {
            EdgeData& e = frame->edges[edge];
            Vec2 p0 = frame->vertices[e.v0].position;
            Vec2 p1 = frame->vertices[e.v1].position;
            if (MakeEdgeKey(p0, p1) == curve.key) {
                e.curve_offset = curve.offset;
                e.curve_weight = curve.weight;
                break;
            }
        }
    }
    g_saved_curve_count = 0;

    // Apply pending curves (from SplitEdge with update=false)
    for (int i = 0; i < GetCurrentFrame(m)->pending_curve_count; i++) {
        PendingCurve& pc = GetCurrentFrame(m)->pending_curves[i];
        int edge = GetEdge(m, pc.v0, pc.v1);
        if (edge != -1) {
            GetCurrentFrame(m)->edges[edge].curve_offset = pc.offset;
            GetCurrentFrame(m)->edges[edge].curve_weight = pc.weight;
        }
    }
    GetCurrentFrame(m)->pending_curve_count = 0;

    for (int edge_index=0; edge_index<GetCurrentFrame(m)->edge_count; edge_index++) {
        EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
        GetCurrentFrame(m)->vertices[e.v0].ref_count++;
        GetCurrentFrame(m)->vertices[e.v1].ref_count++;
        if (e.face_count == 1) {
            GetCurrentFrame(m)->vertices[e.v0].edge_normal += e.normal;
            GetCurrentFrame(m)->vertices[e.v1].edge_normal += e.normal;
        }
    }

    for (int vertex_index=0; vertex_index<GetCurrentFrame(m)->vertex_count; vertex_index++) {
        VertexData& v = GetCurrentFrame(m)->vertices[vertex_index];
        if (Length(v.edge_normal) > F32_EPSILON)
            v.edge_normal = Normalize(v.edge_normal);
    }

    // Compute face islands using union-find
    // Faces that share a vertex belong to the same island
    int island[MESH_MAX_FACES];
    for (int i = 0; i < frame->face_count; i++)
        island[i] = i;

    // Find with path compression
    auto find = [&](int x) {
        while (island[x] != x) {
            island[x] = island[island[x]];
            x = island[x];
        }
        return x;
    };

    // Union two faces into the same island
    auto unite = [&](int a, int b) {
        int ra = find(a);
        int rb = find(b);
        if (ra != rb)
            island[ra] = rb;
    };

    // For each vertex, union all faces that contain it
    for (int vi = 0; vi < frame->vertex_count; vi++) {
        int first_face = -1;
        for (int fi = 0; fi < frame->face_count; fi++) {
            FaceData& f = frame->faces[fi];
            for (int fvi = 0; fvi < f.vertex_count; fvi++) {
                if (f.vertices[fvi] == vi) {
                    if (first_face < 0)
                        first_face = fi;
                    else
                        unite(first_face, fi);
                    break;
                }
            }
        }
    }

    // Flatten and assign final island IDs to faces
    for (int fi = 0; fi < frame->face_count; fi++)
        frame->faces[fi].island = find(fi);

    // Count bones with weights (for skinned mesh detection)
    int bone_count = 0;
    bool bone_used[MAX_BONES] = {};
    for (int vi = 0; vi < frame->vertex_count; vi++) {
        const VertexData& v = frame->vertices[vi];
        for (int w = 0; w < MESH_MAX_VERTEX_WEIGHTS; w++) {
            if (v.weights[w].weight > F32_EPSILON && v.weights[w].bone_index >= 0 && v.weights[w].bone_index < MAX_BONES) {
                if (!bone_used[v.weights[w].bone_index]) {
                    bone_used[v.weights[w].bone_index] = true;
                    bone_count++;
                }
            }
        }
    }
    m->impl->bone_count = bone_count;
}

extern void MarkPreviewDirty();

void MarkDirty(MeshData* m) {
    Free(GetCurrentFrame(m)->mesh);
    Free(GetCurrentFrame(m)->outline);
    GetCurrentFrame(m)->mesh = nullptr;
    GetCurrentFrame(m)->outline = nullptr;

    if (IsFile(m))
        g_editor.meshes[GetUnsortedIndex(m)] = nullptr;

    // Mark mesh for atlas re-render on save
    MarkMeshAtlasDirty(m);

    // Mark editor preview for re-render
    MarkPreviewDirty();
}

Mesh* ToMesh(MeshData* m, bool upload, bool use_cache) {
    if (use_cache && GetCurrentFrame(m)->mesh)
        return GetCurrentFrame(m)->mesh;

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, MAX_VERTICES, MAX_INDICES);

    float depth = 0.01f + 0.99f * (m->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH-MIN_DEPTH);
    for (int i = 0; i < GetCurrentFrame(m)->face_count; i++)
        TriangulateFace(m, GetCurrentFrame(m)->faces + i, builder, depth);

    Mesh* mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, m->name, upload);
    m->bounds = mesh ? GetBounds(mesh) : BOUNDS2_ZERO;

    if (use_cache)
        GetCurrentFrame(m)->mesh = mesh;

    if (IsFile(m))
        g_editor.meshes[GetUnsortedIndex(m)] = mesh;

    Free(builder);

    return mesh;
}

static void AddVertexWeights(MeshBuilder* builder, const VertexData& v) {
    for (int weight_index=0; weight_index < MESH_MAX_VERTEX_WEIGHTS; ++weight_index)
        if (v.weights[weight_index].weight > F32_EPSILON)
            AddVertexWeight(builder, v.weights[weight_index].bone_index, v.weights[weight_index].weight);
}

Mesh* ToOutlineMesh(MeshData* m) {
    if (GetCurrentFrame(m)->outline && GetCurrentFrame(m)->outline_version == g_view.zoom_version)
        return GetCurrentFrame(m)->outline;

    // Ensure bounds are calculated (ToMesh updates m->bounds)
    if (!GetCurrentFrame(m)->mesh)
        ToMesh(m, false);

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, MAX_VERTICES, MAX_INDICES);

    float outline_size = g_view.zoom_ref_scale * OUTLINE_WIDTH * 0.5f;

    for (int i=0; i < GetCurrentFrame(m)->edge_count; i++) {
        const EdgeData& ee = GetCurrentFrame(m)->edges[i];
        const VertexData& v0 = GetCurrentFrame(m)->vertices[ee.v0];
        const VertexData& v1 = GetCurrentFrame(m)->vertices[ee.v1];
        Vec2 p0 = {v0.position.x, v0.position.y};
        Vec2 p1 = {v1.position.x, v1.position.y};
        Vec2 n = Perpendicular(Normalize(p1 - p0));
        u16 base = GetVertexCount(builder);
        AddVertex(builder, p0 - n * outline_size);
        AddVertexWeights(builder, v0);
        AddVertex(builder, p0 + n * outline_size);
        AddVertexWeights(builder, v0);
        AddVertex(builder, p1 + n * outline_size);
        AddVertexWeights(builder, v1);
        AddVertex(builder, p1 - n * outline_size);
        AddVertexWeights(builder, v1);
        AddTriangle(builder, base+0, base+1, base+3);
        AddTriangle(builder, base+1, base+2, base+3);
    }

    GetCurrentFrame(m)->outline = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
    GetCurrentFrame(m)->outline_version = g_view.zoom_version;

    Free(builder);

    return GetCurrentFrame(m)->outline;
}

void SetFaceColor(MeshData* m, int color) {
    int count = 0;
    for (i32 face_index = 0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];
        if (!f.selected) continue;
        f.color = color;
        count++;
    }

    if (!count) return;

    MarkDirty(m);
}

void SetFaceOpacity(MeshData* m, float opacity) {
    int count = 0;
    for (i32 face_index = 0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];
        if (!f.selected) continue;
        f.opacity = opacity;
        count++;
    }

    if (!count) return;

    MarkDirty(m);
}

static int CountSharedEdges(MeshData* m, int face_index0, int face_index1) {
    assert(face_index0 < face_index1);

    int shared_edge_count = 0;
    for (int edge_index=0; edge_index<GetCurrentFrame(m)->edge_count; edge_index++) {
        EdgeData& ee = GetCurrentFrame(m)->edges[edge_index];
        if (ee.face_count != 2)
            continue;

        if (ee.face_index[0] == face_index0 && ee.face_index[1] == face_index1)
            shared_edge_count++;
    }

    return shared_edge_count;
}

static void CollapseEdge(MeshData* m, int edge_index) {
    assert(m);
    assert(edge_index >= 0 && edge_index < GetCurrentFrame(m)->edge_count);

    EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
    VertexData& v0 = GetCurrentFrame(m)->vertices[e.v0];
    VertexData& v1 = GetCurrentFrame(m)->vertices[e.v1];

    DeleteVertex(m, v0.ref_count > v1.ref_count ? e.v1 : e.v0);
    UpdateEdges(m);
    MarkDirty(m);
}

void DissolveEdge(MeshData* m, int edge_index) {
    EdgeData& ee = GetCurrentFrame(m)->edges[edge_index];
    assert(ee.face_count > 0);

    if (ee.face_count == 1)
    {
        FaceData& ef = GetCurrentFrame(m)->faces[ee.face_index[0]];
        if (ef.vertex_count <= 3)
        {
            DeleteFace(m, ee.face_index[0]);
            return;
        }

        CollapseEdge(m, edge_index);
        return;
    }

    // Slit edge: same face on both sides - cannot dissolve
    if (ee.face_index[0] == ee.face_index[1])
        return;

    int shared_edge_count = CountSharedEdges (m, ee.face_index[0], ee.face_index[1]);
    if (shared_edge_count == 1)
    {
        MergeFaces(m, ee);
        return;
    }

    CollapseEdge(m, edge_index);
}

void DeleteVertex(MeshData* m, int vertex_index) {
    assert(vertex_index >= 0 && vertex_index < GetCurrentFrame(m)->vertex_count);

    // Save curve data before modifying vertices (edges still have valid indices)
    SaveCurves(m);

    for (int face_index=GetCurrentFrame(m)->face_count-1; face_index >= 0; face_index--) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];
        int vertex_pos = -1;
        for (int face_vertex_index=0; face_vertex_index<f.vertex_count; face_vertex_index++) {
            if (f.vertices[face_vertex_index] == vertex_index) {
                vertex_pos = face_vertex_index;
                break;
            }
        }

        if (vertex_pos == -1)
            continue;

        if (f.vertex_count <= 3)
            DeleteFaceInternal(m, face_index);
        else
            RemoveFaceVertices(m, face_index, vertex_pos, 1);
    }

    for (int face_index=0; face_index < GetCurrentFrame(m)->face_count; face_index++) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];
        for (int face_vertex_index=0; face_vertex_index<f.vertex_count; face_vertex_index++) {
            int v_idx = f.vertices[face_vertex_index];
            if (v_idx > vertex_index)
                f.vertices[face_vertex_index] = v_idx - 1;
        }
    }

    for (; vertex_index < GetCurrentFrame(m)->vertex_count - 1; vertex_index++)
        GetCurrentFrame(m)->vertices[vertex_index] = GetCurrentFrame(m)->vertices[vertex_index + 1];

    GetCurrentFrame(m)->vertex_count--;

    UpdateEdges(m);
}

static void DeleteFaceInternal(MeshData* m, int face_index) {
    assert(face_index >= 0 && face_index < GetCurrentFrame(m)->face_count);
    RemoveFaceVertices(m, face_index, 0, -1);
    for (int i=face_index; i < GetCurrentFrame(m)->face_count - 1; i++)
        GetCurrentFrame(m)->faces[i] = GetCurrentFrame(m)->faces[i + 1];
    GetCurrentFrame(m)->face_count--;
}

static void DeleteFace(MeshData* m, int face_index) {
    DeleteFaceInternal(m, face_index);
    UpdateEdges(m);
    MarkDirty(m);
}

void DissolveSelectedFaces(MeshData* m) {
    for (int face_index=GetCurrentFrame(m)->face_count - 1; face_index>=0; face_index--) {
        FaceData& ef = GetCurrentFrame(m)->faces[face_index];
        if (!ef.selected)
            continue;

        DeleteFace(m, face_index);
    }
}

static void MergeFaces(MeshData* m, const EdgeData& shared_edge) {
    assert(shared_edge.face_count == 2);
    assert(CountSharedEdges(m, shared_edge.face_index[0], shared_edge.face_index[1]) == 1);

    FaceData& face0 = GetCurrentFrame(m)->faces[shared_edge.face_index[0]];
    FaceData& face1 = GetCurrentFrame(m)->faces[shared_edge.face_index[1]];

    int edge_pos0 = GetFaceEdgeIndex(face0, shared_edge);
    int edge_pos1 = GetFaceEdgeIndex(face1, shared_edge);
    assert(edge_pos0 != -1);
    assert(edge_pos1 != -1);

    int insert_pos = (edge_pos0 + 1) % face0.vertex_count;
    InsertFaceVertices(m, shared_edge.face_index[0], insert_pos, face1.vertex_count - 2);

    for (int face_index=0; face_index<face1.vertex_count - 2; face_index++)
        face0.vertices[insert_pos + face_index] =
            face1.vertices[((edge_pos1 + 2 + face_index) % face1.vertex_count)];

    DeleteFaceInternal(m, shared_edge.face_index[1]);
    UpdateEdges(m);
    MarkDirty(m);
}

void DissolveSelectedVertices(MeshData* m) {
    int vertices[MAX_VERTICES];
    int vertex_count = GetSelectedVertices(m, vertices);
    for (int vertex_index=vertex_count-1; vertex_index>=0; vertex_index--)
        DeleteVertex(m, vertices[vertex_index]);

    MarkDirty(m);
}

static void InsertFaceVertices(MeshData* m, int face_index, int insert_at, int count) {
    FaceData& f = GetCurrentFrame(m)->faces[face_index];

    for (int vertex_index=f.vertex_count + count; vertex_index > insert_at; vertex_index--)
        f.vertices[vertex_index] = f.vertices[vertex_index-count];

    for (int i=0; i<count; i++)
        f.vertices[insert_at + i] = -1;

    f.vertex_count += count;
}

static void RemoveFaceVertices(MeshData* m, int face_index, int remove_at, int remove_count) {
    FaceData& f = GetCurrentFrame(m)->faces[face_index];
    if (remove_count == -1)
        remove_count = f.vertex_count - remove_at;

    assert(remove_at >= 0 && remove_at + remove_count <= f.vertex_count);

    for (int vertex_index=remove_at; vertex_index + remove_count < f.vertex_count; vertex_index++)
        f.vertices[vertex_index] = f.vertices[vertex_index + remove_count];

    f.vertex_count -= remove_count;
}

int CreateFace(MeshData* m) {
    int selected_vertices[MAX_VERTICES];
    int selected_count = GetSelectedVertices(m, selected_vertices);
    if (selected_count < 3)
        return -1;

    if (GetCurrentFrame(m)->face_count >= MAX_FACES)
        return -1;

    for (int i = 0; i < selected_count; i++) {
        int v0 = selected_vertices[i];
        int v1 = selected_vertices[(i + 1) % selected_count];

        int edge_index = GetEdge(m, v0, v1);
        if (edge_index != -1) {
            const EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
            if (e.face_count >= 2)
                return -1;
        }
    }

    // Find color
    int color_counts[COLOR_COUNT] = {};
    for (int i = 0; i < selected_count; i++) {
        int v0 = selected_vertices[i];
        int v1 = selected_vertices[(i + 1) % selected_count];
        int edge_index = GetEdge(m, v0, v1);
        if (edge_index != -1) {
            const EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
            for (int face_idx = 0; face_idx < e.face_count; face_idx++) {
                color_counts[GetCurrentFrame(m)->faces[e.face_index[face_idx]].color]++;
            }
        }
    }

    int best_color = 0;
    int best_count = 0;
    for (int i = 0; i < 64; i++) {
        if (color_counts[i] > best_count) {
            best_count = color_counts[i];
            best_color = i;
        }
    }

    Vec2 centroid = VEC2_ZERO;
    for (int i = 0; i < selected_count; i++)
        centroid += GetCurrentFrame(m)->vertices[selected_vertices[i]].position;
    centroid = centroid / (float)selected_count;

    struct VertexAngle {
        int vertex_index;
        float angle;
    };

    VertexAngle vertex_angles[MAX_VERTICES];
    for (int i = 0; i < selected_count; i++) {
        Vec2 dir = GetCurrentFrame(m)->vertices[selected_vertices[i]].position - centroid;
        vertex_angles[i].vertex_index = selected_vertices[i];
        vertex_angles[i].angle = atan2f(dir.y, dir.x);
    }

    for (int i = 0; i < selected_count - 1; i++) {
        for (int j = i + 1; j < selected_count; j++) {
            if (vertex_angles[i].angle > vertex_angles[j].angle) {
                VertexAngle temp = vertex_angles[i];
                vertex_angles[i] = vertex_angles[j];
                vertex_angles[j] = temp;
            }
        }
    }

    int face_index = GetCurrentFrame(m)->face_count++;
    FaceData& f = GetCurrentFrame(m)->faces[face_index];
    f.vertex_count = selected_count;
    f.color = best_color;
    f.opacity = 1.0f;
    f.normal = {0, 0};
    f.selected = false;

    for (int i = 0; i < selected_count; i++)
        f.vertices[i] = vertex_angles[i].vertex_index;

    UpdateEdges(m);
    MarkDirty(m);

    return face_index;
}

int SplitFaces(MeshData* m, int v0, int v1) {
    if (GetCurrentFrame(m)->face_count >= MAX_FACES)
        return -1;

    if (GetEdge(m, v0, v1) != -1)
        return -1;

    int face_index = 0;
    int v0_pos = -1;
    int v1_pos = -1;
    for (; face_index < GetCurrentFrame(m)->face_count; face_index++) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];

        v0_pos = -1;
        v1_pos = -1;
        for (int i = 0; i < f.vertex_count && (v0_pos == -1 || v1_pos == -1); i++) {
            int vertex_index = f.vertices[i];
            if (vertex_index == v0) v0_pos = i;
            if (vertex_index == v1) v1_pos = i;
        }

        if (v0_pos != -1 && v1_pos != -1)
            break;
    }

    if (face_index >= GetCurrentFrame(m)->face_count)
        return -1;

    if (v0_pos > v1_pos)
    {
        int temp = v0_pos;
        v0_pos = v1_pos;
        v1_pos = temp;
    }

    FaceData& old_face = GetCurrentFrame(m)->faces[face_index];
    FaceData& new_face = GetCurrentFrame(m)->faces[GetCurrentFrame(m)->face_count++];
    new_face.color = old_face.color;
    new_face.normal = old_face.normal;
    new_face.opacity = old_face.opacity;
    new_face.selected = old_face.selected;

    int old_vertex_count = old_face.vertex_count - (v1_pos - v0_pos - 1);
    int new_vertex_count = v1_pos - v0_pos + 1;

    new_face.vertex_count = new_vertex_count;
    for (int vertex_index=0; vertex_index<new_vertex_count; vertex_index++)
        new_face.vertices[vertex_index] = old_face.vertices[v0_pos + vertex_index];

    for (int vertex_index=0; v1_pos+vertex_index<old_face.vertex_count; vertex_index++)
        old_face.vertices[v0_pos + vertex_index + 1] =
            old_face.vertices[v1_pos + vertex_index];

    RemoveFaceVertices(m, face_index, old_vertex_count, old_face.vertex_count - old_vertex_count);

    UpdateEdges(m);
    MarkDirty(m);

    return GetEdge(m, old_face.vertices[v0_pos], old_face.vertices[(v0_pos + 1) % old_face.vertex_count]);
}

int SplitEdge(MeshData* m, int edge_index, float edge_pos, bool update) {
    assert(edge_index >= 0 && edge_index < GetCurrentFrame(m)->edge_count);

    if (GetCurrentFrame(m)->vertex_count >= MAX_VERTICES)
        return -1;

    if (GetCurrentFrame(m)->edge_count >= MAX_VERTICES)
        return -1;

    EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
    VertexData& v0 = GetCurrentFrame(m)->vertices[e.v0];
    VertexData& v1 = GetCurrentFrame(m)->vertices[e.v1];

    // Save curve data before modifying
    Vec2 original_curve_offset = e.curve_offset;
    float original_curve_weight = e.curve_weight;
    int original_v0 = e.v0;
    int original_v1 = e.v1;

    int new_vertex_index = GetCurrentFrame(m)->vertex_count++;
    VertexData& new_vertex = GetCurrentFrame(m)->vertices[new_vertex_index];
    new_vertex.gradient = (v0.gradient * (1.0f - edge_pos) + v1.gradient * edge_pos);

    // Place vertex on the Bezier curve if edge has a curve
    if (LengthSqr(original_curve_offset) > 0.0001f) {
        // Rational quadratic Bezier: B(t) = ((1-t)²P0 + 2w(1-t)t·P1 + t²P2) / ((1-t)² + 2w(1-t)t + t²)
        Vec2 p0 = v0.position;
        Vec2 p2 = v1.position;
        Vec2 p1 = (p0 + p2) * 0.5f + original_curve_offset;  // Control point
        new_vertex.position = EvalQuadraticBezier(p0, p1, p2, edge_pos, original_curve_weight);
    } else {
        new_vertex.position = (v0.position * (1.0f - edge_pos) + v1.position * edge_pos);
    }

    // Interpolate bone weights from edge endpoints
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; i++) {
        new_vertex.weights[i].bone_index = -1;
        new_vertex.weights[i].weight = 0.0f;
    }

    // Collect all unique bones from both vertices and interpolate their weights
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; i++) {
        if (v0.weights[i].weight > F32_EPSILON) {
            int bone = v0.weights[i].bone_index;
            float w0 = v0.weights[i].weight;
            float w1 = GetVertexWeight(m, e.v1, bone);
            float interpolated = w0 * (1.0f - edge_pos) + w1 * edge_pos;
            if (interpolated > F32_EPSILON)
                SetVertexWeight(m, new_vertex_index, bone, interpolated);
        }
        if (v1.weights[i].weight > F32_EPSILON) {
            int bone = v1.weights[i].bone_index;
            // Only process if we haven't already handled this bone from v0
            bool already_set = false;
            for (int j = 0; j < MESH_MAX_VERTEX_WEIGHTS && !already_set; j++)
                already_set = (new_vertex.weights[j].bone_index == bone && new_vertex.weights[j].weight > F32_EPSILON);
            if (!already_set) {
                float w0 = GetVertexWeight(m, e.v0, bone);
                float w1 = v1.weights[i].weight;
                float interpolated = w0 * (1.0f - edge_pos) + w1 * edge_pos;
                if (interpolated > F32_EPSILON)
                    SetVertexWeight(m, new_vertex_index, bone, interpolated);
            }
        }
    }

    int face_count = GetCurrentFrame(m)->face_count;
    for (int face_index = 0; face_index < face_count; face_index++) {
        FaceData& f = GetCurrentFrame(m)->faces[face_index];

        int face_edge = GetFaceEdgeIndex(f, e);
        if (face_edge == -1)
            continue;

        InsertFaceVertices(m, face_index, face_edge + 1, 1);
        f.vertices[face_edge + 1] = new_vertex_index;
    }

    // Queue or apply curve offsets to the two new edges using de Casteljau subdivision
    if (LengthSqr(original_curve_offset) > 0.0001f) {
        Vec2 offset1, offset2;
        float weight1, weight2;
        SplitBezierCurve(v0.position, v1.position, original_curve_offset, original_curve_weight, edge_pos, &offset1, &weight1, &offset2, &weight2);

        if (update) {
            // Apply immediately after UpdateEdges
            UpdateEdges(m);

            int edge1 = GetEdge(m, original_v0, new_vertex_index);
            int edge2 = GetEdge(m, new_vertex_index, original_v1);

            if (edge1 >= 0) {
                GetCurrentFrame(m)->edges[edge1].curve_offset = offset1;
                GetCurrentFrame(m)->edges[edge1].curve_weight = weight1;
            }
            if (edge2 >= 0) {
                GetCurrentFrame(m)->edges[edge2].curve_offset = offset2;
                GetCurrentFrame(m)->edges[edge2].curve_weight = weight2;
            }

            MarkDirty(m);
        } else {
            // Queue for later when UpdateEdges is called
            MeshFrameData* frame = GetCurrentFrame(m);
            if (frame->pending_curve_count < MESH_MAX_EDGES - 1) {
                frame->pending_curves[frame->pending_curve_count++] = { original_v0, new_vertex_index, offset1, weight1 };
                frame->pending_curves[frame->pending_curve_count++] = { new_vertex_index, original_v1, offset2, weight2 };
            }
        }
    } else if (update) {
        UpdateEdges(m);
        MarkDirty(m);
    }

    return new_vertex_index;
}

int HitTestVertex(const Vec2& position, const Vec2& hit_pos, float size_mult) {
    float size = g_view.select_size * size_mult;
    float dist = Length(hit_pos - position);
    return dist <= size;
}

int HitTestVertex(MeshData* m, const Mat3& transform, const Vec2& position, float size_mult) {
    float size = g_view.select_size * size_mult;
    float best_dist = F32_MAX;
    int best_vertex = -1;
    for (int i = 0; i < GetCurrentFrame(m)->vertex_count; i++) {
        const VertexData& v = GetCurrentFrame(m)->vertices[i];
        float dist = Length(position - TransformPoint(transform, v.position));
        if (dist <= size && dist < best_dist) {
            best_vertex = i;
            best_dist = dist;
        }
    }

    return best_vertex;
}

int HitTestEdge(MeshData* m, const Mat3& transform, const Vec2& hit_pos, float* where, float size_mult) {
    const float size = g_view.select_size * 0.75f * size_mult;
    float best_dist = F32_MAX;
    int best_edge = -1;
    float best_where = 0.0f;

    for (int i = 0; i < GetCurrentFrame(m)->edge_count; i++) {
        const EdgeData& e = GetCurrentFrame(m)->edges[i];
        Vec2 v0 = TransformPoint(transform, GetCurrentFrame(m)->vertices[e.v0].position);
        Vec2 v1 = TransformPoint(transform, GetCurrentFrame(m)->vertices[e.v1].position);

        if (IsEdgeCurved(m, i)) {
            // Test against subdivided curve segments
            Vec2 control = TransformPoint(transform, GetEdgeControlPoint(m, i));
            float weight = e.curve_weight > 0.0f ? e.curve_weight : 1.0f;
            constexpr int segments = 8;
            Vec2 prev = v0;
            for (int s = 1; s <= segments; s++) {
                float t = (float)s / (float)segments;
                Vec2 curr = EvalQuadraticBezier(v0, control, v1, t, weight);

                // Test point against this line segment
                Vec2 seg_dir = curr - prev;
                float seg_length = Length(seg_dir);
                if (seg_length > 0.0001f) {
                    seg_dir = seg_dir / seg_length;
                    Vec2 to_mouse = hit_pos - prev;
                    float proj = Dot(to_mouse, seg_dir);
                    if (proj >= 0 && proj <= seg_length) {
                        Vec2 closest = prev + seg_dir * proj;
                        float dist = Length(hit_pos - closest);
                        if (dist < size && dist < best_dist) {
                            best_edge = i;
                            best_dist = dist;
                            // Calculate approximate t value along full curve
                            float approx_t = ((float)(s - 1) + proj / seg_length) / (float)segments;

                            // Refine t using Newton-Raphson to find exact point on Bezier
                            for (int iter = 0; iter < 4; iter++) {
                                float mt = 1.0f - approx_t;
                                Vec2 curve_pt = v0 * (mt * mt) + control * (2.0f * approx_t * mt) + v1 * (approx_t * approx_t);
                                Vec2 derivative = (control - v0) * (2.0f * mt) + (v1 - control) * (2.0f * approx_t);
                                Vec2 diff = curve_pt - hit_pos;
                                float deriv_len_sq = LengthSqr(derivative);
                                if (deriv_len_sq < F32_EPSILON) break;
                                float dt = Dot(diff, derivative) / deriv_len_sq;
                                approx_t = Clamp(approx_t - dt, 0.0f, 1.0f);
                            }
                            best_where = approx_t;
                        }
                    }
                }
                prev = curr;
            }
        } else {
            // Straight edge test
            Vec2 edge_dir = Normalize(v1 - v0);
            Vec2 to_mouse = hit_pos - v0;
            float edge_length = Length(v1 - v0);
            float proj = Dot(to_mouse, edge_dir);
            if (proj >= 0 && proj <= edge_length) {
                Vec2 closest_point = v0 + edge_dir * proj;
                float dist = Length(hit_pos - closest_point);
                if (dist < size && dist < best_dist) {
                    best_edge = i;
                    best_dist = dist;
                    best_where = proj / edge_length;
                }
            }
        }
    }

    if (where)
        *where = best_where;

    return best_edge;
}

void Center(MeshData* m) {
    if (GetCurrentFrame(m)->vertex_count == 0)
        return;

    RecordUndo(m);

    Bounds2 bounds = {GetCurrentFrame(m)->vertices[0].position, GetCurrentFrame(m)->vertices[0].position};
    for (int vertex_index=1; vertex_index<GetCurrentFrame(m)->vertex_count; vertex_index++)
        bounds = Union(bounds, GetCurrentFrame(m)->vertices[vertex_index].position);

    Vec2 size = GetSize(bounds);
    Vec2 offset = bounds.min + size * 0.5f;
    for (int i=0; i<GetCurrentFrame(m)->vertex_count; i++)
        GetCurrentFrame(m)->vertices[i].position = GetCurrentFrame(m)->vertices[i].position - offset;

    UpdateEdges(m);
    MarkDirty(m);
    MarkModified();
}

void SwapFace(MeshData* m, int face_index_a, int face_index_b) {
    FaceData temp = GetCurrentFrame(m)->faces[face_index_a];
    GetCurrentFrame(m)->faces[face_index_a] = GetCurrentFrame(m)->faces[face_index_b];
    GetCurrentFrame(m)->faces[face_index_b] = temp;
}

bool OverlapBounds(MeshData* m, const Vec2& position, const Bounds2& hit_bounds) {
    return Intersects(m->bounds + position, hit_bounds);
}

int HitTestFaces(MeshData* m, const Mat3& transform, const Vec2& position, int* faces, int max_faces) {
    int hit_count = 0;
    for (int i = GetCurrentFrame(m)->face_count - 1; i >= 0 && hit_count < max_faces; i--) {
        FaceData& f = GetCurrentFrame(m)->faces[i];

        // Ray casting algorithm - works for both convex and concave polygons
        int intersections = 0;

        for (int vertex_index = 0; vertex_index < f.vertex_count; vertex_index++) {
            int v0_idx = f.vertices[vertex_index];
            int v1_idx = f.vertices[(vertex_index + 1) % f.vertex_count];

            Vec2 v0 = TransformPoint(transform, GetCurrentFrame(m)->vertices[v0_idx].position);
            Vec2 v1 = TransformPoint(transform, GetCurrentFrame(m)->vertices[v1_idx].position);

            // Cast horizontal ray to the right from hit_pos
            // Check if this edge intersects the ray
            float min_y = Min(v0.y, v1.y);
            float max_y = Max(v0.y, v1.y);

            // Skip horizontal edges and edges that don't cross the ray's Y level
            if (position.y < min_y || position.y >= max_y || min_y == max_y)
                continue;

            // Calculate X intersection point
            float t = (position.y - v0.y) / (v1.y - v0.y);
            float x_intersect = v0.x + t * (v1.x - v0.x);

            // Count intersection if it's to the right of the point
            if (x_intersect > position.x)
                intersections++;
        }

        // Point is inside if odd number of intersections
        if (!(intersections % 2) == 1)
            continue;

        faces[hit_count++] = i;
    }

    return hit_count;
}

int HitTestFace(MeshData* m, const Mat3& transform, const Vec2& position) {
    int faces[1];
    int hit_count = HitTestFaces(m, transform, position, faces, 1);
    return hit_count > 0 ? faces[0] : -1;
}

static void ParseVertexWeight(Tokenizer& tk, VertexWeight& vertex_weight) {
    f32 weight = 0.0f;
    i32 index = 0;
    if (!ExpectInt(tk, &index))
        ThrowError("missing weight bone index");

    if (!ExpectFloat(tk, &weight))
        ThrowError("missing vertex weight value");

    vertex_weight = { index, weight };
}

static void ParseVertex(MeshData* m, Tokenizer& tk) {
    if (GetCurrentFrame(m)->vertex_count >= MAX_VERTICES)
        ThrowError("too many vertices");

    f32 x;
    if (!ExpectFloat(tk, &x))
        ThrowError("missing vertex x coordinate");

    f32 y;
    if (!ExpectFloat(tk, &y))
        ThrowError("missing vertex y coordinate");

    VertexData& v = GetCurrentFrame(m)->vertices[GetCurrentFrame(m)->vertex_count++];
    v = {};  // Initialize to zero
    v.position = {x,y};

    int weight_count = 0;
    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "e")) {
            // deprecated
            float e;
            ExpectFloat(tk, &e);
        } else if (ExpectIdentifier(tk, "h")) {
            float temp = 0.0f;
            ExpectFloat(tk, &temp);
        } else if (ExpectIdentifier(tk, "w")) {
            ParseVertexWeight(tk, v.weights[weight_count++]);
        } else {
            break;
        }
    }
}

static void ParseEdgeColor(MeshData* m, Tokenizer& tk) {
    int cx;
    if (!ExpectInt(tk, &cx)) ThrowError("missing edge color x value");

    int cy;
    if (!ExpectInt(tk, &cy)) ThrowError("missing edge color y value");
}

static void ParseFaceColor(FaceData& f, Tokenizer& tk) {
    f.color = 0;
    if (!ExpectInt(tk, &f.color))
        ThrowError("missing face color x value");

    // Ignore old face color
    int cy;
    ExpectInt(tk, &cy);

    float opacity = 1.0f;
    ExpectFloat(tk, &opacity);
    f.opacity = Clamp01(opacity);
}

static void ParseFaceNormal(FaceData& ef, Tokenizer& tk) {
    f32 nx;
    if (!ExpectFloat(tk, &nx))
        ThrowError("missing face normal x value");

    f32 ny;
    if (!ExpectFloat(tk, &ny))
        ThrowError("missing face normal y value");

    // deprecated but keep for old
    f32 nz;
    ExpectFloat(tk, &nz);

    ef.normal = {nx, ny};
}

static void ParseFace(MeshData* m, Tokenizer& tk) {
    if (GetCurrentFrame(m)->face_count >= MAX_FACES)
        ThrowError("too many faces");

    int v0;
    if (!ExpectInt(tk, &v0))
        ThrowError("missing face v0 index");

    int v1;
    if (!ExpectInt(tk, &v1))
        ThrowError("missing face v1 index");

    int v2;
    if (!ExpectInt(tk, &v2))
        ThrowError("missing face v2 index");

    FaceData& f = GetCurrentFrame(m)->faces[GetCurrentFrame(m)->face_count++];
    f = {};  // Initialize to zero
    f.opacity = 1.0f;  // Default opacity for new/loaded faces
    f.vertices[f.vertex_count++] = v0;
    f.vertices[f.vertex_count++] = v1;
    f.vertices[f.vertex_count++] = v2;

    while (ExpectInt(tk, &v2))
        f.vertices[f.vertex_count++] = v2;

    // Handle a degenerate case where there are two points in a row.
    if (f.vertices[f.vertex_count-1] == f.vertices[0])
        f.vertex_count--;

    if (v0 < 0 || v0 >= GetCurrentFrame(m)->vertex_count || v1 < 0 || v1 >= GetCurrentFrame(m)->vertex_count || v2 < 0 || v2 >= GetCurrentFrame(m)->vertex_count)
        ThrowError("face vertex index out of range");

    f.color = 0;

    while (!IsEOF(tk))
    {
        if (ExpectIdentifier(tk, "c"))
            ParseFaceColor(f, tk);
        else if (ExpectIdentifier(tk, "n"))
            ParseFaceNormal(f, tk);
        else
            break;
    }
}

static void ParseDepth(MeshData* m, Tokenizer& tk) {
    float depth = 0.0f;
    if (!ExpectFloat(tk, &depth))
        ThrowError("missing mesh depth value");

    m->impl->depth = (int)(depth * (MAX_DEPTH - MIN_DEPTH) + MIN_DEPTH);
}

static void ParsePalette(MeshData* m, Tokenizer& tk) {
    int palette = 0;
    if (!ExpectInt(tk, &palette))
        ThrowError("missing mesh palette value");

    m->impl->palette = palette;
}

static void ParseSkeleton(MeshData* m, Tokenizer& tk) {
    if (!ExpectQuotedString(tk))
        ThrowError("missing skeleton name");

    m->impl->skeleton_name = GetName(tk);
}

// Finalize current frame after parsing (update edges, apply curves)
static void FinalizeFrame(MeshData* m, PendingCurve* pending_curves, int pending_curve_count) {
    UpdateEdges(m);

    // Apply pending curve data now that edges exist
    for (int i = 0; i < pending_curve_count; i++) {
        int edge = GetEdge(m, pending_curves[i].v0, pending_curves[i].v1);
        if (edge != -1) {
            GetCurrentFrame(m)->edges[edge].curve_offset = pending_curves[i].offset;
            GetCurrentFrame(m)->edges[edge].curve_weight = pending_curves[i].weight;
        }
    }

    MarkDirty(m);
    ToMesh(m, false);
}

void LoadMeshData(MeshData* m, Tokenizer& tk, bool multiple_mesh=false) {
    // Store curve data temporarily since edges don't exist until UpdateEdges
    PendingCurve pending_curves[MESH_MAX_EDGES];
    int pending_curve_count = 0;

    while (!IsEOF(tk)) {
        if (ExpectIdentifier(tk, "f")) {
            ParseFace(m, tk);
        } else if (ExpectIdentifier(tk, "frame")) {
            // New frame marker: "frame N hold H"

            // Finalize previous frame if this isn't the first
            if (GetCurrentFrame(m)->vertex_count > 0) {
                FinalizeFrame(m, pending_curves, pending_curve_count);
                pending_curve_count = 0;

                // Add a new frame
                if (m->impl->frame_count < MESH_MAX_FRAMES) {
                    m->impl->frame_count++;
                    m->impl->current_frame = m->impl->frame_count - 1;
                }
            }

            // Parse hold if present
            if (ExpectIdentifier(tk, "hold")) {
                int hold = 0;
                ExpectInt(tk, &hold);
                GetCurrentFrame(m)->hold = hold;
            }
        } else if (ExpectIdentifier(tk, "v")) {
            ParseVertex(m, tk);
        } else if (ExpectIdentifier(tk, "s")) {
            ParseSkeleton(m, tk);
        } else if (ExpectIdentifier(tk, "d")) {
            ParseDepth(m, tk);
        } else if (ExpectIdentifier(tk, "p")) {
            ParsePalette(m, tk);
        } else if (ExpectIdentifier(tk, "e")) {
            ParseEdgeColor(m, tk);
        } else if (ExpectIdentifier(tk, "curve")) {
            // Parse curve data: curve v0 v1 offset_x offset_y [weight]
            // Store for later since edges don't exist yet
            int v0, v1;
            float ox, oy, weight = 1.0f;
            if (ExpectInt(tk, &v0) && ExpectInt(tk, &v1) && ExpectFloat(tk, &ox) && ExpectFloat(tk, &oy)) {
                ExpectFloat(tk, &weight);  // Optional - old files won't have it
                if (pending_curve_count < MESH_MAX_EDGES) {
                    pending_curves[pending_curve_count++] = { v0, v1, {ox, oy}, weight };
                }
            }
        } else if (multiple_mesh && Peek(tk, "m")) {
            // Old animated mesh format compatibility
            break;
        } else {
            char error[1024];
            GetString(tk, error, sizeof(error) - 1);
            ThrowError("invalid token '%s' in mesh", error);
        }
    }

    // Finalize the last (or only) frame
    FinalizeFrame(m, pending_curves, pending_curve_count);

    // Reset to first frame for editing
    m->impl->current_frame = 0;
}

void SerializeMesh(Mesh* m, Stream* stream) {
    if (!m) {
        WriteStruct(stream, BOUNDS2_ZERO);
        WriteU16(stream, 0);
        WriteU16(stream, 0);
        return;
    }

    u16 vertex_count = GetVertexCount(m);
    u16 index_count = GetIndexCount(m);

    WriteStruct(stream, GetBounds(m));
    WriteU16(stream, vertex_count);
    WriteU16(stream, index_count);

    if (vertex_count > 0) {
        const MeshVertex* v = GetVertices(m);
        WriteBytes(stream, v, sizeof(MeshVertex) * GetVertexCount(m));

        const u16* i = GetIndices(m);
        WriteBytes(stream, i, sizeof(u16) * GetIndexCount(m));
    }

    WriteU8(stream, static_cast<u8>(GetFrameCount(m)));
    WriteU8(stream, static_cast<u8>(GetFrameRate(m)));
    WriteFloat(stream, GetFrameWidthUV(m));
}

static void LoadMeshData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData *>(a);

    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path.value);
    Tokenizer tk;
    Init(tk, contents.c_str());
    LoadMeshData(m, tk);
}

MeshData* LoadMeshData(const std::filesystem::path& path) {
    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
    Tokenizer tk;
    Init(tk, contents.c_str());

    MeshData* m = static_cast<MeshData*>(CreateAssetData(path));
    assert(m);
    Init(m);
    LoadMeshData(m);
    return m;
}

static void LoadMeshMetaData(AssetData* a, Props* meta) {
    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData*>(a);
    m->impl->palette = meta->GetInt("mesh", "palette", m->impl->palette);
    // atlas_name is set by atlas post-load, not stored in mesh metadata
}

static void SaveMeshMetaData(AssetData* a, Props* meta) {
    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData*>(a);
    meta->SetInt("mesh", "palette", m->impl->palette);
    // atlas_name is owned by the atlas, not saved in mesh metadata
}

static void WriteVertexWeights(Stream* stream, const VertexWeight* weights) {
    for (int weight_index=0; weight_index<MESH_MAX_VERTEX_WEIGHTS; weight_index++) {
        const VertexWeight& w = weights[weight_index];
        if (w.weight <= 0.0f)
            continue;

        WriteCSTR(stream, " w %d %f", w.bone_index, w.weight);
    }
}

// Save a single frame's data (helper for SaveMeshData)
static void SaveFrameData(MeshFrameData* frame, Stream* stream) {
    for (int i=0; i<frame->vertex_count; i++) {
        const VertexData& v = frame->vertices[i];
        WriteCSTR(stream, "v %f %f", v.position.x, v.position.y);
        WriteVertexWeights(stream, v.weights);
        WriteCSTR(stream, "\n");
    }

    WriteCSTR(stream, "\n");

    for (int i=0; i<frame->face_count; i++) {
        const FaceData& f = frame->faces[i];

        WriteCSTR(stream, "f ");
        for (int vertex_index=0; vertex_index<f.vertex_count; vertex_index++)
            WriteCSTR(stream, " %d", f.vertices[vertex_index]);

        WriteCSTR(stream, " c %d", f.color);
        if (f.opacity < 1.0f)
            WriteCSTR(stream, " %f", f.opacity);
        if (LengthSqr(f.normal) > 0.0001f)
            WriteCSTR(stream, " n %f %f", f.normal.x, f.normal.y);

        WriteCSTR(stream, "\n");
    }

    // Write curve data for curved edges
    for (int i = 0; i < frame->edge_count; i++) {
        const EdgeData& e = frame->edges[i];
        if (LengthSqr(e.curve_offset) > 0.0001f) {
            WriteCSTR(stream, "curve %d %d %f %f %f\n", e.v0, e.v1, e.curve_offset.x, e.curve_offset.y, e.curve_weight);
        }
    }
}

void SaveMeshData(MeshData* m, Stream* stream) {
    // Write shared header
    if (m->impl->skeleton_name != nullptr)
        WriteCSTR(stream, "s \"%s\"\n", m->impl->skeleton_name->value);

    WriteCSTR(stream, "d %f\n", (m->impl->depth - MIN_DEPTH) / (float)(MAX_DEPTH - MIN_DEPTH));
    WriteCSTR(stream, "p %d\n", m->impl->palette);
    WriteCSTR(stream, "\n");

    // Write each frame
    for (int frame_index = 0; frame_index < m->impl->frame_count; frame_index++) {
        MeshFrameData* frame = &m->impl->frames[frame_index];

        // Write frame header (only if multiple frames or frame has hold)
        if (m->impl->frame_count > 1 || frame->hold > 0) {
            WriteCSTR(stream, "frame");
            if (frame->hold > 0)
                WriteCSTR(stream, " h %d", frame->hold);
            WriteCSTR(stream, "\n");
        }

        SaveFrameData(frame, stream);

        if (frame_index < m->impl->frame_count - 1)
            WriteCSTR(stream, "\n");
    }
}

static void SaveMeshData(AssetData* a, const std::filesystem::path& path) {
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData*>(a);

    // Auto-assign to atlas if needed (lazy assignment)
    if (NeedsAtlasAssignment(m)) {
        AutoAssignMeshToAtlas(m);
    }

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    SaveMeshData(m, stream);
    SaveStream(stream, path);
    Free(stream);
}

AssetData* NewMeshData(const std::filesystem::path& path) {
    constexpr const char* default_mesh =
        "v -1 -1\n"
        "v 1 -1\n"
        "v 1 1\n"
        "v -1 1\n"
        "\n"
        "f 0 1 2 3 c 0 0\n";

    std::string text = std::format(default_mesh);

    if (g_view.selected_asset_count == 1) {
        AssetData* selected = GetFirstSelectedAsset();
        assert(selected);
        if (selected->type == ASSET_TYPE_MESH)
            text = ReadAllText(ALLOCATOR_DEFAULT, selected->path.value);
    }

    std::string type_folder = ToString(ASSET_TYPE_MESH);
    Lower(type_folder.data(), (u32)type_folder.size());

    std::filesystem::path full_path =  path.is_relative()
        ? std::filesystem::path(g_editor.project_path) / g_editor.save_dir / type_folder / path
        : path;
    full_path += ".mesh";

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    WriteCSTR(stream, text.c_str());
    SaveStream(stream, full_path);
    Free(stream);

    return LoadMeshData(full_path);
}

static void AllocateData(MeshData* m) {
    m->impl = static_cast<MeshDataImpl*>(Alloc(ALLOCATOR_DEFAULT, sizeof(MeshDataImpl)));
    memset(m->impl, 0, sizeof(MeshDataImpl));
    m->impl->frame_count = 1;  // Always at least one frame
    m->impl->current_frame = 0;
}

static void CloneMeshData(AssetData* a) {
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData*>(a);

    MeshDataImpl* old_data = m->impl;
    AllocateData(m);
    memcpy(m->impl, old_data, sizeof(MeshDataImpl));

    // Clear all cached mesh pointers (they belong to original)
    for (int i = 0; i < m->impl->frame_count; i++) {
        m->impl->frames[i].mesh = nullptr;
        m->impl->frames[i].outline = nullptr;
    }
    m->impl->playing = nullptr;
}

void InitMeshData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* em = static_cast<MeshData*>(a);
    Init(em);
}

static bool IsEar(MeshData* m, int* indices, int vertex_count, int ear_index) {
    int prev = (ear_index - 1 + vertex_count) % vertex_count;
    int curr = ear_index;
    int next = (ear_index + 1) % vertex_count;

    Vec2 v0 = GetCurrentFrame(m)->vertices[indices[prev]].position;
    Vec2 v1 = GetCurrentFrame(m)->vertices[indices[curr]].position;
    Vec2 v2 = GetCurrentFrame(m)->vertices[indices[next]].position;

    // Check if triangle has correct winding (counter-clockwise)
    float cross = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
    if (cross <= 0)
        return false;

    // Check if any other vertex is inside this triangle
    for (int i = 0; i < vertex_count; i++)
    {
        if (i == prev || i == curr || i == next)
            continue;

        Vec2 p = GetCurrentFrame(m)->vertices[indices[i]].position;

        // Use barycentric coordinates to check if point is inside triangle
        Vec2 v0v1 = v1 - v0;
        Vec2 v0v2 = v2 - v0;
        Vec2 v0p = p - v0;

        float dot00 = Dot(v0v2, v0v2);
        float dot01 = Dot(v0v2, v0v1);
        float dot02 = Dot(v0v2, v0p);
        float dot11 = Dot(v0v1, v0v1);
        float dot12 = Dot(v0v1, v0p);

        float inv_denom = 1.0f / (dot00 * dot11 - dot01 * dot01);
        float u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
        float v = (dot00 * dot12 - dot01 * dot02) * inv_denom;

        if (u > 0 && v > 0 && u + v < 1)
            return false;
    }

    return true;
}

// Position-based IsEar for expanded curve vertices
static bool IsEarPositions(Vec2* positions, int* position_indices, int vertex_count, int ear_index) {
    int prev = (ear_index - 1 + vertex_count) % vertex_count;
    int curr = ear_index;
    int next = (ear_index + 1) % vertex_count;

    Vec2 v0 = positions[position_indices[prev]];
    Vec2 v1 = positions[position_indices[curr]];
    Vec2 v2 = positions[position_indices[next]];

    // Check if triangle has correct winding (counter-clockwise)
    float cross = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
    if (cross <= 0)
        return false;

    // Check if any other vertex is inside this triangle
    for (int i = 0; i < vertex_count; i++) {
        if (i == prev || i == curr || i == next)
            continue;

        Vec2 p = positions[position_indices[i]];

        // Use barycentric coordinates to check if point is inside triangle
        Vec2 v0v1 = v1 - v0;
        Vec2 v0v2 = v2 - v0;
        Vec2 v0p = p - v0;

        float dot00 = Dot(v0v2, v0v2);
        float dot01 = Dot(v0v2, v0v1);
        float dot02 = Dot(v0v2, v0p);
        float dot11 = Dot(v0v1, v0v1);
        float dot12 = Dot(v0v1, v0p);

        float inv_denom = 1.0f / (dot00 * dot11 - dot01 * dot01);
        float u = (dot11 * dot02 - dot01 * dot12) * inv_denom;
        float v = (dot00 * dot12 - dot01 * dot02) * inv_denom;

        if (u > 0 && v > 0 && u + v < 1)
            return false;
    }

    return true;
}

static void TriangulateFace(MeshData* m, FaceData* f, MeshBuilder* builder, float depth) {
    if (f->vertex_count < 3)
        return;

    Vec2 uv_color = ToVec2(Vec2Int(f->color, m->impl->palette));

    bool has_curves = false;
    for (int i = 0; i < f->vertex_count; i++) {
        int v0_idx = f->vertices[i];
        int v1_idx = f->vertices[(i + 1) % f->vertex_count];
        int edge_index = GetEdge(m, v0_idx, v1_idx);
        if (edge_index != -1 && IsEdgeCurved(m, edge_index)) {
            has_curves = true;
            break;
        }
    }

    constexpr int CURVE_SEGMENTS = 8;
    int expanded_count = 0;
    Vec2 expanded_positions[MAX_VERTICES * CURVE_SEGMENTS];
    Vec4 expanded_bone_weights[MAX_VERTICES * CURVE_SEGMENTS];
    Vec4Int expanded_bone_indices[MAX_VERTICES * CURVE_SEGMENTS];
    Vec2 expanded_normals[MAX_VERTICES * CURVE_SEGMENTS];

    if (has_curves) {
        for (int i = 0; i < f->vertex_count; i++) {
            int v0_idx = f->vertices[i];
            int v1_idx = f->vertices[(i + 1) % f->vertex_count];
            VertexData& v0 = GetCurrentFrame(m)->vertices[v0_idx];
            VertexData& v1 = GetCurrentFrame(m)->vertices[v1_idx];

            int edge_index = GetEdge(m, v0_idx, v1_idx);
            if (edge_index != -1 && IsEdgeCurved(m, edge_index)) {
                EdgeData& edge = GetCurrentFrame(m)->edges[edge_index];
                Vec2 control = GetEdgeControlPoint(m, edge_index);
                float weight = edge.curve_weight > 0.0f ? edge.curve_weight : 1.0f;

                for (int s = 0; s < CURVE_SEGMENTS; s++) {
                    float t = static_cast<float>(s) / static_cast<float>(CURVE_SEGMENTS);
                    expanded_positions[expanded_count] = EvalQuadraticBezier(v0.position, control, v1.position, t, weight);

                    // Interpolate bone weights
                    expanded_bone_weights[expanded_count] = {
                        v0.weights[0].weight * (1 - t) + v1.weights[0].weight * t,
                        v0.weights[1].weight * (1 - t) + v1.weights[1].weight * t,
                        v0.weights[2].weight * (1 - t) + v1.weights[2].weight * t,
                        v0.weights[3].weight * (1 - t) + v1.weights[3].weight * t
                    };
                    expanded_bone_indices[expanded_count] = {
                        v0.weights[0].bone_index, v0.weights[1].bone_index,
                        v0.weights[2].bone_index, v0.weights[3].bone_index
                    };
                    expanded_normals[expanded_count] = Normalize(v0.edge_normal * (1 - t) + v1.edge_normal * t);
                    expanded_count++;
                }
            } else {
                // Straight edge - just add the start vertex
                expanded_positions[expanded_count] = v0.position;
                expanded_bone_weights[expanded_count] = {
                    v0.weights[0].weight, v0.weights[1].weight,
                    v0.weights[2].weight, v0.weights[3].weight
                };
                expanded_bone_indices[expanded_count] = {
                    v0.weights[0].bone_index, v0.weights[1].bone_index,
                    v0.weights[2].bone_index, v0.weights[3].bone_index
                };
                expanded_normals[expanded_count] = v0.edge_normal;
                expanded_count++;
            }
        }

        for (int i = 0; i < expanded_count; i++) {
            MeshVertex mv = {
                .position = expanded_positions[i],
                .depth = depth,
                .opacity = f->opacity,
                .uv = uv_color,
                .normal = expanded_normals[i],
                .bone_indices = expanded_bone_indices[i],
                .bone_weights = expanded_bone_weights[i]
            };
            AddVertex(builder, mv);
        }
    } else {
        // No curves - use original vertex adding logic
        for (int vertex_index = 0; vertex_index < f->vertex_count; vertex_index++) {
            VertexData& v = GetCurrentFrame(m)->vertices[f->vertices[vertex_index]];
            MeshVertex mv = {
                .position = v.position,
                .depth = depth,
                .opacity = f->opacity,
                .uv = uv_color
            };
            mv.bone_weights.x = v.weights[0].weight;
            mv.bone_weights.y = v.weights[1].weight;
            mv.bone_weights.z = v.weights[2].weight;
            mv.bone_weights.w = v.weights[3].weight;
            mv.bone_indices.x = v.weights[0].bone_index;
            mv.bone_indices.y = v.weights[1].bone_index;
            mv.bone_indices.z = v.weights[2].bone_index;
            mv.bone_indices.w = v.weights[3].bone_index;
            mv.normal = v.edge_normal;
            AddVertex(builder, mv);
        }
        expanded_count = f->vertex_count;
    }

    u16 base_vertex = GetVertexCount(builder) - static_cast<u16>(expanded_count);
    if (expanded_count == 3) {
        AddTriangle(builder, base_vertex, base_vertex + 1, base_vertex + 2);
        return;
    }

    int positions[MAX_VERTICES * CURVE_SEGMENTS];
    for (int i = 0; i < expanded_count; i++)
        positions[i] = i;

    int remaining_vertices = expanded_count;
    int current_index = 0;

    while (remaining_vertices > 3) {
        bool found_ear = false;

        for (int attempts = 0; attempts < remaining_vertices; attempts++) {
            bool is_ear = false;

            if (has_curves) {
                // Use position-based ear test for expanded curve vertices
                is_ear = IsEarPositions(expanded_positions, positions, remaining_vertices, current_index);
            } else {
                // Use vertex-based ear test for non-curved faces
                int indices[MAX_VERTICES];
                for (int i = 0; i < remaining_vertices; i++)
                    indices[i] = f->vertices[positions[i]];
                is_ear = IsEar(m, indices, remaining_vertices, current_index);
            }

            if (is_ear) {
                // Found an ear, create triangle
                int prev = (current_index - 1 + remaining_vertices) % remaining_vertices;
                int next = (current_index + 1) % remaining_vertices;

                // Use positions directly to get builder indices
                AddTriangle(builder,
                    base_vertex + (u16)positions[prev],
                    base_vertex + (u16)positions[current_index],
                    base_vertex + (u16)positions[next]);

                // Remove the ear position from the polygon
                for (int i = current_index; i < remaining_vertices - 1; i++) {
                    positions[i] = positions[i + 1];
                }
                remaining_vertices--;

                // Adjust current index after removal
                if (current_index >= remaining_vertices)
                    current_index = 0;

                found_ear = true;
                break;
            }

            current_index = (current_index + 1) % remaining_vertices;
        }

        if (!found_ear) {
            // Fallback: fan triangulation from first vertex
            for (int i = 1; i < remaining_vertices - 1; i++) {
                AddTriangle(builder,
                    base_vertex + (u16)positions[0],
                    base_vertex + (u16)positions[i],
                    base_vertex + (u16)positions[i + 1]);
            }
            break;
        }
    }

    if (remaining_vertices == 3) {
        AddTriangle(builder,
            base_vertex + (u16)positions[0],
            base_vertex + (u16)positions[1],
            base_vertex + (u16)positions[2]);
    }
}

int GetSelectedVertices(MeshData* m, int vertices[MAX_VERTICES]) {
    int selected_vertex_count=0;
    for (int select_index=0; select_index<GetCurrentFrame(m)->vertex_count; select_index++) {
        VertexData& v = GetCurrentFrame(m)->vertices[select_index];
        if (!v.selected) continue;
        vertices[selected_vertex_count++] = select_index;
    }
    return selected_vertex_count;
}

int GetSelectedEdges(MeshData* m, int edges[MAX_EDGES]) {
    int selected_edge_count=0;
    for (int edge_index=0; edge_index<GetCurrentFrame(m)->edge_count; edge_index++) {
        EdgeData& e = GetCurrentFrame(m)->edges[edge_index];
        if (!e.selected)
            continue;

        edges[selected_edge_count++] = edge_index;
    }

    return selected_edge_count;
}

Vec2 HitTestSnap(MeshData* m, const Vec2& position) {
    float best_dist_sqr = LengthSqr(position);
    Vec2 best_snap = VEC2_ZERO;
    return best_snap;
}

Vec2 GetEdgePoint(MeshData* m, int edge_index, float t) {
    EdgeData& edge = GetCurrentFrame(m)->edges[edge_index];
    Vec2 p0 = GetCurrentFrame(m)->vertices[edge.v0].position;
    Vec2 p2 = GetCurrentFrame(m)->vertices[edge.v1].position;

    if (IsEdgeCurved(m, edge_index)) {
        Vec2 p1 = GetEdgeControlPoint(m, edge_index);
        float weight = edge.curve_weight > 0.0f ? edge.curve_weight : 1.0f;
        return EvalQuadraticBezier(p0, p1, p2, t, weight);
    }

    return Mix(p0, p2, t);
}

void SetOrigin(MeshData* m, const Vec2& origin) {
    Vec2 delta = m->position - origin;
    for (int vertex_index = 0; vertex_index < GetCurrentFrame(m)->vertex_count; vertex_index++)
        GetCurrentFrame(m)->vertices[vertex_index].position += delta;

    m->position = origin;
    UpdateEdges(m);
    MarkDirty(m);
}

float GetVertexWeight(MeshData* m, int vertex_index, int bone_index) {
    if (bone_index < 0)
        return 0.0f;

    for (int weight_index = 0; weight_index < MESH_MAX_VERTEX_WEIGHTS; weight_index++) {
        const VertexWeight& w = GetCurrentFrame(m)->vertices[vertex_index].weights[weight_index];
        if (w.bone_index == bone_index)
            return w.weight;
    }

    return 0.0f;
}

int GetVertexWeightIndex(MeshData* m, int vertex_index, int bone_index) {
    for (int weight_index = 0; weight_index < MESH_MAX_VERTEX_WEIGHTS; weight_index++) {
        const VertexWeight& w = GetCurrentFrame(m)->vertices[vertex_index].weights[weight_index];
        if (w.bone_index == bone_index && w.weight > F32_EPSILON)
            return weight_index;
    }

    return -1;
}

int GetOrAddVertexWeightIndex(MeshData* m, int vertex_index, int bone_index) {
    int weight_index = GetVertexWeightIndex(m, vertex_index, bone_index);
    if (weight_index != -1)
        return weight_index;

    for (weight_index = 0; weight_index < MESH_MAX_VERTEX_WEIGHTS; weight_index++) {
        const VertexWeight& w = GetCurrentFrame(m)->vertices[vertex_index].weights[weight_index];
        if (w.weight <= F32_EPSILON)
            return weight_index;
    }

    return -1;
}

void SetVertexWeight(MeshData* m, int vertex_index, int bone_index, float weight) {
    int weight_index = GetOrAddVertexWeightIndex(m, vertex_index, bone_index);
    if (weight_index == -1)
        return;

    VertexData& v = GetCurrentFrame(m)->vertices[vertex_index];
    VertexWeight& w = v.weights[weight_index];
    w.bone_index = bone_index;
    w.weight = weight;
}

void AddVertexWeight(MeshData* m, int vertex_index, int bone_index, float weight) {
    int weight_index = GetOrAddVertexWeightIndex(m, vertex_index, bone_index);
    if (weight_index == -1)
        return;

    VertexData& v = GetCurrentFrame(m)->vertices[vertex_index];
    VertexWeight& w = v.weights[weight_index];
    w.bone_index = bone_index;
    w.weight = Clamp01(w.weight + weight);
}

void SetSingleBone(MeshData* m, int bone_index) {
    MeshFrameData* frame = GetCurrentFrame(m);

    for (int vi = 0; vi < frame->vertex_count; vi++) {
        VertexData& v = frame->vertices[vi];

        // Clear all weights
        for (int w = 0; w < MESH_MAX_VERTEX_WEIGHTS; w++) {
            v.weights[w].bone_index = -1;
            v.weights[w].weight = 0.0f;
        }

        // Set single bone weight
        if (bone_index >= 0) {
            v.weights[0].bone_index = bone_index;
            v.weights[0].weight = 1.0f;
        }
    }

    UpdateEdges(m);
    MarkDirty(m);
}

void ClearBone(MeshData* m) {
    SetSingleBone(m, -1);
}

void InterpolateVertexWeights(MeshData* m, int new_vertex_index, int v0_index, int v1_index, float t) {
    MeshFrameData* frame = GetCurrentFrame(m);
    VertexData& new_vertex = frame->vertices[new_vertex_index];
    const VertexData& v0 = frame->vertices[v0_index];
    const VertexData& v1 = frame->vertices[v1_index];

    // Initialize weights to empty
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; i++) {
        new_vertex.weights[i].bone_index = -1;
        new_vertex.weights[i].weight = 0.0f;
    }

    // Collect all unique bones from both vertices and interpolate their weights
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; i++) {
        if (v0.weights[i].weight > F32_EPSILON) {
            int bone = v0.weights[i].bone_index;
            float w0 = v0.weights[i].weight;
            float w1 = GetVertexWeight(m, v1_index, bone);
            float interpolated = w0 * (1.0f - t) + w1 * t;
            if (interpolated > F32_EPSILON)
                SetVertexWeight(m, new_vertex_index, bone, interpolated);
        }
    }
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; i++) {
        if (v1.weights[i].weight > F32_EPSILON) {
            int bone = v1.weights[i].bone_index;
            // Only process if we haven't already handled this bone from v0
            bool already_set = false;
            for (int j = 0; j < MESH_MAX_VERTEX_WEIGHTS && !already_set; j++)
                already_set = (new_vertex.weights[j].bone_index == bone && new_vertex.weights[j].weight > F32_EPSILON);
            if (!already_set) {
                float w0 = GetVertexWeight(m, v0_index, bone);
                float w1 = v1.weights[i].weight;
                float interpolated = w0 * (1.0f - t) + w1 * t;
                if (interpolated > F32_EPSILON)
                    SetVertexWeight(m, new_vertex_index, bone, interpolated);
            }
        }
    }
}

void InferVertexWeightsFromNeighbors(MeshData* m, int vertex_index) {
    MeshFrameData* frame = GetCurrentFrame(m);
    VertexData& v = frame->vertices[vertex_index];

    // Find all neighboring vertices (vertices that share an edge)
    int neighbor_count = 0;
    int neighbors[16];
    for (int ei = 0; ei < frame->edge_count; ei++) {
        const EdgeData& e = frame->edges[ei];
        int neighbor = -1;
        if (e.v0 == vertex_index)
            neighbor = e.v1;
        else if (e.v1 == vertex_index)
            neighbor = e.v0;
        if (neighbor >= 0 && neighbor_count < 16) {
            // Avoid duplicates
            bool found = false;
            for (int i = 0; i < neighbor_count && !found; i++)
                found = (neighbors[i] == neighbor);
            if (!found)
                neighbors[neighbor_count++] = neighbor;
        }
    }

    if (neighbor_count == 0)
        return;

    // Initialize weights
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS; i++) {
        v.weights[i].bone_index = -1;
        v.weights[i].weight = 0.0f;
    }

    // Average weights from all neighbors
    // First, collect total weight for each bone across neighbors
    struct BoneWeight { int bone; float total; int count; };
    BoneWeight bone_weights[64] = {};
    int bone_weight_count = 0;

    for (int ni = 0; ni < neighbor_count; ni++) {
        const VertexData& nv = frame->vertices[neighbors[ni]];
        for (int wi = 0; wi < MESH_MAX_VERTEX_WEIGHTS; wi++) {
            if (nv.weights[wi].weight <= F32_EPSILON)
                continue;
            int bone = nv.weights[wi].bone_index;
            // Find or add bone entry
            int found_idx = -1;
            for (int bi = 0; bi < bone_weight_count && found_idx < 0; bi++)
                if (bone_weights[bi].bone == bone)
                    found_idx = bi;
            if (found_idx < 0 && bone_weight_count < 64) {
                found_idx = bone_weight_count++;
                bone_weights[found_idx].bone = bone;
                bone_weights[found_idx].total = 0;
                bone_weights[found_idx].count = 0;
            }
            if (found_idx >= 0) {
                bone_weights[found_idx].total += nv.weights[wi].weight;
                bone_weights[found_idx].count++;
            }
        }
    }

    // Sort by average weight (descending) and assign top weights
    for (int i = 0; i < bone_weight_count - 1; i++) {
        for (int j = i + 1; j < bone_weight_count; j++) {
            float avg_i = bone_weights[i].total / bone_weights[i].count;
            float avg_j = bone_weights[j].total / bone_weights[j].count;
            if (avg_j > avg_i) {
                BoneWeight tmp = bone_weights[i];
                bone_weights[i] = bone_weights[j];
                bone_weights[j] = tmp;
            }
        }
    }

    // Assign top MESH_MAX_VERTEX_WEIGHTS bones
    for (int i = 0; i < MESH_MAX_VERTEX_WEIGHTS && i < bone_weight_count; i++) {
        float avg = bone_weights[i].total / bone_weights[i].count;
        if (avg > F32_EPSILON) {
            v.weights[i].bone_index = bone_weights[i].bone;
            v.weights[i].weight = avg;
        }
    }
}

static void PostLoadMeshData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_MESH);
    MeshData* m = static_cast<MeshData*>(a);
    if (m->impl->skeleton_name)
        m->impl->skeleton = static_cast<SkeletonData*>(GetAssetData(ASSET_TYPE_SKELETON, m->impl->skeleton_name));

    // Auto-snap all vertices to pixel grid
    bool modified = false;
    for (int fi = 0; fi < m->impl->frame_count; fi++) {
        MeshFrameData* frame = &m->impl->frames[fi];
        for (int vi = 0; vi < frame->vertex_count; vi++) {
            Vec2 world_pos = m->position + frame->vertices[vi].position;
            Vec2 snapped = SnapToPixelGrid(world_pos);
            if (world_pos.x != snapped.x || world_pos.y != snapped.y) {
                frame->vertices[vi].position = snapped - m->position;
                modified = true;
            }
        }
    }
    if (modified) {
        UpdateEdges(m);
        MarkDirty(m);
        MarkModified(a);
    }
}

static void DestroyMeshData(AssetData* a) {
    MeshData* m = static_cast<MeshData*>(a);

    // Free all frame meshes
    for (int i = 0; i < m->impl->frame_count; i++) {
        Free(m->impl->frames[i].mesh);
        Free(m->impl->frames[i].outline);
    }

    // Free playing mesh if any
    Free(m->impl->playing);

    Free(m->impl);
    m->impl = nullptr;
}

static void Init(MeshData* m) {
    AllocateData(m);

    m->vtable = {
        .destructor = DestroyMeshData,
        .load = LoadMeshData,
        .post_load = PostLoadMeshData,
        .save = SaveMeshData,
        .load_metadata = LoadMeshMetaData,
        .save_metadata = SaveMeshMetaData,
        .draw = DrawMesh,
        .clone = CloneMeshData
    };

    InitMeshEditor(m);
}
