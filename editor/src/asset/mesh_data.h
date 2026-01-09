//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct AtlasData;
struct PixelData;
struct Rasterizer;

constexpr int MESH_MIN_DEPTH = 0;
constexpr int MESH_MAX_DEPTH = 100;
constexpr int MESH_MAX_VERTICES = 1024;
constexpr int MESH_MAX_INDICES = 1024;
constexpr int MESH_MAX_FACES = 256;
constexpr int MESH_MAX_EDGES = 2048;
constexpr int MESH_MAX_FACE_EDGES = 128;
constexpr int MESH_MAX_FACE_VERTICES = MESH_MAX_FACE_EDGES;
constexpr Bounds2 MESH_DEFAULT_BOUNDS = Bounds2(Vec2(-0.5f, -0.5f), Vec2(0.5f, 0.5f));

enum VertexFlags : u8 {
    VERTEX_FLAG_NONE = 0,
    VERTEX_FLAG_SELECTED = 1 << 0,
};

struct VertexData {
    Vec2 position;
    VertexFlags flags;
    int edge_count;
};

struct CurveData {
    Vec2 offset;
    float weight;
};

enum EdgeFlags : u8 {
    EDGE_FLAG_NONE = 0,
    EDGE_FLAG_SELECTED = 1 << 0,
};

struct EdgeData {
    u16 v0;
    u16 v1;
    u16 face_left;
    u16 face_right;

    struct {
        Vec2 offset;
        float weight;
    } curve;

    EdgeFlags flags;
};

enum FaceFlags : u8 {
    FACE_FLAG_NONE = 0,
    FACE_FLAG_SELECTED = 1 << 0,
};

struct FaceData {
    u16 edges[MESH_MAX_FACE_EDGES];
    Vec2 center;
    float opacity;
    u16 edge_count;
    u16 island;
    u8 color;
    FaceFlags flags;
};

struct MeshGeometry {
    VertexData verts[MESH_MAX_VERTICES];
    EdgeData edges[MESH_MAX_EDGES];
    FaceData faces[MESH_MAX_FACES];
    u16 vert_count;
    u16 edge_count;
    u16 face_count;
};

struct MeshFrameData {
    MeshGeometry geom;

    u16 selected_vertex_count;
    u16 selected_edge_count;
    u16 selected_face_count;

    Mesh* mesh;
    Mesh* outline;
    int outline_version;
    int hold;
    Bounds2 bounds;
    bool dirty;
};

struct MeshDataImpl {
    MeshFrameData frames[MESH_MAX_FRAMES];
    int frame_count;
    int current_frame;

    SkeletonData* skeleton;
    const Name* skeleton_name;
    u8 bone_index;
    AtlasData* atlas;
    Vec2Int size;
    int palette;
    int depth;
    bool atlas_dirty;

    Mesh* playing;
    float play_time;
};

struct MeshData : AssetData {
    MeshDataImpl* impl;
};

extern MeshFrameData* GetCurrentFrame(MeshData* m);
extern void SetCurrentFrame(MeshData* m, int frame_index);
extern int GetFrameCount(MeshData* m);
extern void AddFrame(MeshData* m, int after_index);
extern void DeleteFrame(MeshData* m, int frame_index);
extern void CopyFrame(MeshData* m, int src_frame, MeshFrameData* dst);
extern void PasteFrame(MeshData* m, int dst_frame, const MeshFrameData* src);

inline MeshFrameData* GetCurrentFrame(MeshData* m) {
    return &m->impl->frames[m->impl->current_frame];
}

inline MeshFrameData* GetFrame(MeshData* m, int frame) {
    return &m->impl->frames[frame];
}

inline int GetFrameCount(MeshData* m) {
    return m->impl->frame_count;
}

extern void InitMeshData(AssetData* a);
extern AssetData* NewMeshData(const std::filesystem::path& path);
extern MeshData* Clone(Allocator* allocator, MeshData* m);
extern MeshData* LoadEditorMesh(const std::filesystem::path& path);
extern Mesh* ToMesh(MeshData* m, bool upload=true, bool use_cache=true);
extern Mesh* ToOutlineMesh(MeshData* m);
extern int HitTestFace(MeshData* m, const Mat3& transform, const Vec2& position);
extern int HitTestFaces(MeshData* m, const Mat3& transform, const Vec2& position, int* faces, int max_faces=MESH_MAX_VERTICES);
extern int HitTestVertex(MeshData* m, const Mat3& transform, const Vec2& position, float size_mult=1.0f);
inline int HitTestVertex(MeshData* m, const Vec2& position, float size_mult=1.0f) {
    return HitTestVertex(m, Translate(m->position), position, size_mult);
}
extern Vec2 HitTestSnap(MeshData* m, const Vec2& position);
extern int HitTestVertex(const Vec2& position, const Vec2& hit_pos, float size_mult=1.0f);
extern int HitTestEdge(MeshData* m, const Mat3& transform, const Vec2& position, float* where=nullptr, float size_mult=1.0f);
inline int HitTestEdge(MeshData* m, const Vec2& position, float* where=nullptr, float size_mult=1.0f) {
    return HitTestEdge(m, Translate(m->position), position, where, size_mult);
}
extern Bounds2 GetSelectedBounds(MeshData* m);
extern void MarkDirty(MeshData* m);
extern void SetSelecteFaceColor(MeshData* m, int color);
extern void DeleteSelectedVertices(MeshData* m);
extern void DissolveSelectedEdges(MeshData* m);
extern void DeleteSelectedFaces(MeshData* m);
extern void SetHeight(MeshData* m, int index, float height);
extern int SplitEdge(MeshData* m, int edge_index, float edge_pos, bool update=true);
extern int SplitTriangle(MeshData* m, int triangle_index, const Vec2& position);
extern int AddVertex(MeshData* m, const Vec2& position);
extern int RotateEdge(MeshData* m, int edge_index);
extern void DrawMesh(MeshData* m, const Mat3& transform, Material* material=nullptr);
extern Vec2 GetFaceCenter(MeshData* m, u16 face_index);
inline Vec2 GetVertexPoint(MeshData* m, int vertex_index) {
    return GetCurrentFrame(m)->geom.verts[vertex_index].position;
}
extern void Update(MeshData* m, bool force=false);
extern void Update(MeshData* m, int frame_index, bool force=false);
extern void Center(MeshData* m);
extern bool FixWinding(MeshData* m, FaceData& ef);
extern void DrawFaceCenters(MeshData* m, const Vec2& position);
extern int CreateFace(MeshData* m);
extern u16 GetSelectedVertices(MeshData* m, u16 vertices[MESH_MAX_VERTICES]);
extern u16 GetSelectedEdges(MeshData* m, u16 edges[MESH_MAX_EDGES]);
extern void SerializeMesh(Mesh* m, Stream* stream);
extern void SwapFace(MeshData* m, int face_index_a, int face_index_b);
extern void SetOrigin(MeshData* m, const Vec2& origin);
extern void SetBone(MeshData* m, int bone_index);  // Sets entire mesh to single bone (or clears if -1)
extern void ClearBone(MeshData* m);  // Clears all bone weights from mesh

extern void SetFaceColor(MeshData* m, u8 color);
extern void SetFaceOpacity(MeshData* m, float opacity);

extern void Rasterize(
    MeshData* m,
    int frame_index,
    Rasterizer* rasterizer,
    PixelData* pixels,
    const Vec2Int& position,
    int padding=1);


// @mesh
inline bool IsSkinned(MeshData* mesh) {
    if (!mesh || !mesh->impl) return false;
    return mesh->impl->skeleton_name && mesh->impl->bone_index != U8_MAX;
}

// @frame
extern void MarkDirty(MeshFrameData* frame);
extern void DeleteFace(MeshFrameData* frame, FaceData* f, bool include_verts=true);

// @vertex
inline bool IsValid(MeshFrameData* frame, VertexData* v) {
    return frame && v && (v >= frame->geom.verts) && (v < frame->geom.verts + frame->geom.vert_count);
}
inline u16 GetIndex(MeshFrameData* frame, VertexData* v) {
    return static_cast<u16>(v - frame->geom.verts);
}
inline VertexData* GetVertex(MeshFrameData* frame, u16 vertex_index) {
    return frame->geom.verts + vertex_index;
}
inline void SetFlags(VertexData* v, VertexFlags value, VertexFlags mask) {
    v->flags = static_cast<VertexFlags>(v->flags & ~mask);
    v->flags = static_cast<VertexFlags>(v->flags | (value & mask));
}
inline bool IsSelected(const VertexData* v) {
    return (v->flags & VERTEX_FLAG_SELECTED) != VERTEX_FLAG_NONE;
}
inline bool IsVertexSelected(MeshFrameData* f, u16 vertex_index) {
    return IsSelected(f->geom.verts + vertex_index);
}
extern void DeleteVertex(MeshFrameData* frame, u16 vertex_index);

// @edge
inline u16 GetIndex(MeshFrameData* frame, EdgeData* e) {
    return static_cast<u16>(e - frame->geom.edges);
}
inline bool IsValid(MeshFrameData* frame, EdgeData* e) {
    return frame && e && (e >= frame->geom.edges) && (e < frame->geom.edges + frame->geom.edge_count);
}
extern void DeleteEdge(MeshFrameData* frame, EdgeData* e, bool include_verts);
extern EdgeData* GetEdge(MeshFrameData* frame, u16 edge_index);
extern u16 GetEdge(MeshData* m, u16 v0, u16 v1);
extern Vec2 GetEdgeMidpoint(MeshFrameData* frame, u16 edge_index);
extern Vec2 GetEdgeControlPoint(MeshFrameData* frame, u16 edge_index);
extern void DeleteEdge(MeshFrameData* frame, u16 edge_index);
extern Vec2 GetEdgePoint(MeshFrameData* frame, u16 edge_index, float t);

inline bool IsSelected(const EdgeData* e) {
    return (e->flags & EDGE_FLAG_SELECTED) != EDGE_FLAG_NONE;
}
inline void SetFlags(EdgeData* e, EdgeFlags value, EdgeFlags mask) {
    e->flags = static_cast<EdgeFlags>(e->flags & ~mask);
    e->flags = static_cast<EdgeFlags>(e->flags | (value & mask));
}
inline bool IsEdgeSelected(MeshFrameData* f, u16 edge_index) {
    return IsSelected(GetEdge(f, edge_index));
}
inline bool HasCurve(const EdgeData* e) {
    return LengthSqr(e->curve.offset) > FLT_EPSILON;
}

// @face
inline u16 GetIndex(MeshFrameData* frame, FaceData* f) {
    return static_cast<u16>(f - frame->geom.faces);
}
inline bool IsValid(MeshFrameData* frame, FaceData* f) {
    return frame && f && (f >= frame->geom.faces) && (f < frame->geom.faces + frame->geom.face_count);
}
inline FaceData* GetFace(MeshFrameData* frame, u16 face_index) {
    return frame->geom.faces + face_index;
}
inline EdgeData* GetFaceEdge(MeshFrameData* frame, const FaceData* f, u16 face_edge_index) {
    return frame->geom.edges + f->edges[face_edge_index];
}
extern u16 GetFaceVertices(MeshFrameData* frame, u16 face_index, u16 vertices[MESH_MAX_FACE_VERTICES]);
inline u16 GetFaceVertices(MeshFrameData* frame, const FaceData* f, u16 vertices[MESH_MAX_FACE_VERTICES]) {
    return GetFaceVertices(frame, (u16)(f - frame->geom.faces), vertices);
}
inline void SetFlags(FaceData* f, FaceFlags value, FaceFlags mask) {
    f->flags = static_cast<FaceFlags>(f->flags & ~mask);
    f->flags = static_cast<FaceFlags>(f->flags | (value & mask));
}
inline bool IsSelected(const FaceData* f) {
    return (f->flags & FACE_FLAG_SELECTED) != FACE_FLAG_NONE;
}
inline bool IsFaceSelected(MeshFrameData* frame, u16 face_index) {
    return IsSelected(GetFace(frame, face_index));
}
extern int CountSharedEdges(MeshFrameData* frame, u16 face_index0, u16 face_index1);
