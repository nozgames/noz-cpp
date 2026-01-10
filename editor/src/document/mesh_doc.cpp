//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "utils/rasterizer.h"
#include "utils/pixel_data.h"
#include "atlas_manager.h"

namespace noz::editor {

    constexpr float OUTLINE_WIDTH = 0.015f;

    struct SavedCurve { u64 key; Vec2 offset; float weight; };
    static SavedCurve g_saved_curves[MESH_MAX_EDGES];
    static int g_saved_curve_count = 0;

    extern void InitMeshEditor(MeshDocument* m);
    static void InitMeshDocument(MeshDocument* m);
    void DeleteFaceInternal(MeshDocument* m, int face_index);
    void RemoveFaceVertices(MeshDocument* m, int face_index, int remove_at, int remove_count);
    void InsertFaceVertices(MeshDocument* m, int face_index, int insert_at, int count);
    void MergeFaces(MeshDocument* m, const EdgeData& shared_edge);
    void DeleteFace(MeshDocument* m, int face_index);

    void SetCurrentFrame(MeshDocument* m, int frame_index) {
        if (frame_index >= 0 && frame_index < m->frame_count)
            m->current_frame = frame_index;
    }

    void AddFrame(MeshDocument* m, int after_index) {
        if (m->frame_count >= MESH_MAX_FRAMES)
            return;

        for (int i = m->frame_count; i > after_index + 1; i--)
            m->frames[i] = m->frames[i - 1];

        MeshFrameData& frame = m->frames[after_index + 1];
        frame = m->frames[after_index];
        frame.mesh = nullptr;
        frame.outline = nullptr;
        m->frame_count++;
        m->current_frame = after_index + 1;
    }

    void DeleteFrame(MeshDocument* m, int frame_index) {
        if (m->frame_count <= 1)
            return;

        MeshFrameData& frame = m->frames[frame_index];
        Free(frame.mesh);
        Free(frame.outline);

        for (int i = frame_index; i < m->frame_count - 1; i++)
            m->frames[i] = m->frames[i + 1];

        m->frame_count--;

        if (m->current_frame >= m->frame_count)
            m->current_frame = m->frame_count - 1;
    }

    static void DrawMesh(Document* a) {
    #if 0
        assert(a->type == ASSET_TYPE_MESH);
        MeshDocument* m = static_cast<MeshDocument*>(a);

        // TODO: Animation playback preview will work after runtime merge (Phase 3)
        // For now, just draw the current frame
        DrawMesh(m, Translate(a->position));
    #endif
    }

    void DrawMesh(MeshDocument* m, const Mat3& transform, Material* material) {
        // Check if mesh is in an atlas - render as pixel art textured quad
        AtlasRect* rect = nullptr;
        AtlasDocument* atlas = FindAtlasForMesh(m->name, &rect);
        if (atlas && rect) {
            // Ensure atlas is rendered and texture is uploaded to GPU
            if (!atlas->pixels) {
                RegenerateAtlas(atlas);
            }
            SyncAtlasTexture(atlas);
            if (!atlas->material) goto fallback;
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

            BindMaterial(atlas->material);
            BindColor(COLOR_WHITE);
            DrawMesh(quad, transform);
            return;
        }

    fallback:
        // Fall back to vector mesh rendering
        BindMaterial(material ? material : g_workspace.shaded_material);
        if (g_workspace.draw_mode == VIEW_DRAW_MODE_WIREFRAME) {
            BindColor(COLOR_EDGE);
            DrawMesh(ToOutlineMesh(m), transform);
        } else {
            BindColor(COLOR_WHITE);
            DrawMesh(ToMesh(m), transform);
        }
    }

    Vec2 GetFaceCenter(MeshDocument* m, FaceData* f) {
        (void)m;
        return f->center;
    }

    Vec2 GetFaceCenter(MeshDocument* m, u16 face_index) {
        return GetFace(GetCurrentFrame(m), face_index)->center;
    }

    u16 GetEdge(MeshDocument* m, u16 v0, u16 v1) {
        MeshFrameData* frame = GetCurrentFrame(m);
        int fv0 = Min(v0, v1);
        int fv1 = Max(v0, v1);
        for (u16 ei = 0; ei < frame->geom.edge_count; ei++) {
            const EdgeData* e = GetEdge(frame, ei);
            if (e->v0 == fv0 && e->v1 == fv1)
                return ei;
        }

        return U16_MAX;
    }

    Vec2 GetEdgeMidpoint(MeshFrameData* frame, u16 edge_index) {
        const EdgeData* e = GetEdge(frame, edge_index);
        const Vec2& v0 = GetVertex(frame, e->v0)->position;
        const Vec2& v1 = GetVertex(frame, e->v1)->position;
        return (v0 + v1) * 0.5f;
    }

    Vec2 GetEdgeControlPoint(MeshFrameData* frame, u16 edge_index) {
        return GetEdgeMidpoint(frame, edge_index) + frame->geom.edges[edge_index].curve.offset;
    }

    u16 GetFaceVertices(MeshFrameData* frame, u16 face_index, u16 vertices[MESH_MAX_FACE_VERTICES]) {
        assert(face_index < frame->geom.face_count);

        FaceData* f = frame->geom.faces + face_index;
        for (int i = 0; i < f->edge_count; i++) {
            const EdgeData* e = frame->geom.edges + f->edges[i];
            vertices[i] = e->face_left == face_index ? e->v0 : e->v1;
        }

        vertices[f->edge_count] = vertices[0];

        return f->edge_count;
    }

    inline u64 QuantizePosition(Vec2 p) {
        i32 x = (i32)(p.x * 10000.0f);
        i32 y = (i32)(p.y * 10000.0f);
        return ((u64)(u32)x << 32) | (u64)(u32)y;
    }

    u64 MakeEdgeKey(const Vec2& p0, const Vec2& p1) {
        u64 k0 = QuantizePosition(p0);
        u64 k1 = QuantizePosition(p1);
        return k0 < k1 ? (k0 ^ (k1 * 31)) : (k1 ^ (k0 * 31));
    }

    void SaveCurves(MeshDocument* m) {
    #if 0
        MeshFrameData* frame = GetCurrentFrame(m);
        g_saved_curve_count = 0;
        for (y15 i = 0; i < frame->geom.edge_count; i++) {
            EdgeData& e = frame->geom.edges[i];
            if (LengthSqr(e.curve.offset) > FLT_EPSILON) {
                Vec2 p0 = frame->geom.verts[e.v0].position;
                Vec2 p1 = frame->geom.verts[e.v1].position;
                g_saved_curves[g_saved_curve_count++] = { MakeEdgeKey(p0, p1), e.curve.offset, e.curve.weight };
            }
        }
    #endif
    }

    static void UpdateFace(MeshFrameData* frame, FaceData* f) {
        if (f->edge_count < 3) {
            f->center = VEC2_ZERO;
            return;
        }

        float signed_area = 0.0f;
        Vec2 centroid = VEC2_ZERO;

        u16 vertices[MESH_MAX_FACE_VERTICES];
        u16 vertex_count = GetFaceVertices(frame, f, vertices);

        Bounds2 bounds = {
            GetVertex(frame, vertices[0])->position,
            GetVertex(frame, vertices[0])->position
        };
        for (u16 vertex_index=0; vertex_index < vertex_count; vertex_index++) {
            const Vec2& p0 = GetVertex(frame, vertices[vertex_index])->position;
            const Vec2& p1 = GetVertex(frame, vertices[vertex_index+1])->position;
            float cross = p0.x * p1.y - p1.x * p0.y;
            signed_area += cross;
            centroid.x += (p0.x + p1.x) * cross;
            centroid.y += (p0.y + p1.y) * cross;

            bounds = Union(bounds, p0);
        }

        for (u16 edge_index=0; edge_index < f->edge_count; edge_index++) {
            EdgeData* e = GetFaceEdge(frame, f, edge_index);
            if (LengthSqr(e->curve.offset) < 0.0001f)
                continue;

            const Vec2& p0 = GetVertex(frame, e->v0)->position;
            const Vec2& p1 = GetVertex(frame, e->v1)->position;
            Union(bounds, (p0 + p1) * 0.5f + e->curve.offset);
        }

        float pixel_size = 1.0f / g_editor.atlas.dpi;
        bounds.min.x = floorf(bounds.min.x / pixel_size) * pixel_size;
        bounds.min.y = floorf(bounds.min.y / pixel_size) * pixel_size;
        bounds.max.x = ceilf(bounds.max.x / pixel_size) * pixel_size;
        bounds.max.y = ceilf(bounds.max.y / pixel_size) * pixel_size;
        frame->bounds = bounds;

        signed_area *= 0.5f;

        if (Abs(signed_area) >= F32_EPSILON) {
            f->center = centroid * (1.0f / (6.0f * signed_area));
            return;
        }

        centroid = VEC2_ZERO;
        for (u16 vertex_index=0; vertex_index < vertex_count; vertex_index++)
            centroid += GetVertex(frame, vertices[vertex_index])->position;

        f->center = centroid * (1.0f / (float)vertex_count);
    }

    void Update(MeshDocument* m, int frame_index, bool force) {
        MeshFrameData* frame = &m->frames[frame_index];
        if (!force && !frame->dirty)
            return;

        frame->geom.edge_count = 0;

        for (u16 face_index=0; face_index < frame->geom.face_count; face_index++)
            UpdateFace(frame, GetFace(frame, face_index));
    }

    void Update(MeshDocument* m, bool force) {
        for (int i=0; i < m->frame_count; i++)
            Update(m, i, force);

        if (m->frame_count == 0) {
            m->size = VEC2INT_ZERO;
            m->bounds = MESH_DEFAULT_BOUNDS;
            return;
        }

        bool valid = false;
        for (int frame_index=0; frame_index<m->frame_count; frame_index++) {
            MeshFrameData* frame = &m->frames[frame_index];
            if (frame->geom.vert_count == 0) continue;

            if (!valid) {
                m->bounds = frame->bounds;
            } else {
                m->bounds.min.x = Min(m->bounds.min.x, frame->bounds.min.x);
                m->bounds.min.y = Min(m->bounds.min.y, frame->bounds.min.y);
                m->bounds.max.x = Max(m->bounds.max.x, frame->bounds.max.x);
                m->bounds.max.y = Max(m->bounds.max.y, frame->bounds.max.y);
            }

            valid = true;
        }

        if (!valid) {
            m->bounds = MESH_DEFAULT_BOUNDS;
            m->size = VEC2INT_ZERO;
            return;
        }

        m->size = {
            (int)roundf((m->bounds.max.x - m->bounds.min.x) * g_editor.atlas.dpi),
            (int)roundf((m->bounds.max.y - m->bounds.min.y) * g_editor.atlas.dpi)
        };

    }

    void MarkDirty(MeshFrameData* frame) {
        frame->dirty = true;
    }

    void MarkDirty(MeshDocument* mdoc) {
        MeshFrameData* frame = GetCurrentFrame(mdoc);
        Free(frame->mesh);
        Free(frame->outline);
        frame->mesh = nullptr;
        frame->outline = nullptr;
        frame->dirty = true;

        // if (IsFile(m))
        //     g_editor.meshes[GetUnsortedIndex(m)] = nullptr;


        //MarkMeshAtlasDirty(mdoc);
    }

    Mesh* ToMesh(MeshDocument* m, bool upload, bool use_cache) {
        if (use_cache && GetCurrentFrame(m)->mesh)
            return GetCurrentFrame(m)->mesh;

        MeshFrameData* frame = GetCurrentFrame(m);
        if (frame->geom.vert_count == 0) {
            m->bounds = BOUNDS2_ZERO;
            return nullptr;
        }

        Bounds2 bounds = {frame->geom.verts[0].position, frame->geom.verts[0].position};
        for (int i = 1; i < frame->geom.vert_count; i++) {
            bounds.min.x = Min(bounds.min.x, frame->geom.verts[i].position.x);
            bounds.min.y = Min(bounds.min.y, frame->geom.verts[i].position.y);
            bounds.max.x = Max(bounds.max.x, frame->geom.verts[i].position.x);
            bounds.max.y = Max(bounds.max.y, frame->geom.verts[i].position.y);
        }

        // Include curve control points in bounds
        for (int i = 0; i < frame->geom.edge_count; i++) {
            EdgeData& e = frame->geom.edges[i];
            if (LengthSqr(e.curve.offset) > 0.0001f) {
                Vec2 p0 = frame->geom.verts[e.v0].position;
                Vec2 p1 = frame->geom.verts[e.v1].position;
                Vec2 control = (p0 + p1) * 0.5f + e.curve.offset;
                bounds.min.x = Min(bounds.min.x, control.x);
                bounds.min.y = Min(bounds.min.y, control.y);
                bounds.max.x = Max(bounds.max.x, control.x);
                bounds.max.y = Max(bounds.max.y, control.y);
            }
        }

        m->bounds = bounds;

        // Build simple quad (4 vertices, 2 triangles)
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 4, 6);
        float depth = 0.01f + 0.99f * (m->depth - MESH_MIN_DEPTH) / (float)(MESH_MAX_DEPTH - MESH_MIN_DEPTH);

        MeshVertex v = {};
        v.depth = depth;
        v.opacity = 1.0f;

        // Quad corners with UV 0-1 mapping
        v.position = bounds.min; v.uv = {0, 0}; AddVertex(builder, v);
        v.position = {bounds.max.x, bounds.min.y}; v.uv = {1, 0}; AddVertex(builder, v);
        v.position = bounds.max; v.uv = {1, 1}; AddVertex(builder, v);
        v.position = {bounds.min.x, bounds.max.y}; v.uv = {0, 1}; AddVertex(builder, v);

        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);

        Mesh* mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, m->name, upload);

        if (use_cache)
            frame->mesh = mesh;

        // if (IsFile(m))
        //     g_editor.meshes[GetUnsortedIndex(m)] = mesh;

        Free(builder);

        return mesh;
    }

    Mesh* ToOutlineMesh(MeshDocument* mdoc) {
        MeshFrameData* frame = GetCurrentFrame(mdoc);
        if (frame->outline && frame->outline_version == g_workspace.zoom_version)
            return frame->outline;

        if (!frame->mesh)
            ToMesh(mdoc, false);

        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, MESH_MAX_VERTICES, MESH_MAX_INDICES);

        float outline_size = g_workspace.zoom_ref_scale * OUTLINE_WIDTH * 0.5f;

        for (u16 edge_index=0; edge_index < frame->geom.edge_count; edge_index++) {
            const EdgeData* e = GetEdge(frame, edge_index);
            const Vec2& p0 = GetVertex(frame, e->v0)->position;
            const Vec2& p1 = GetVertex(frame, e->v1)->position;
            Vec2 n = Perpendicular(Normalize(p1 - p0));
            u16 base = GetVertexCount(builder);
            AddVertex(builder, p0 - n * outline_size);
            SetBone(builder, mdoc->bone_index);
            AddVertex(builder, p0 + n * outline_size);
            SetBone(builder, mdoc->bone_index);
            AddVertex(builder, p1 + n * outline_size);
            SetBone(builder, mdoc->bone_index);
            AddVertex(builder, p1 - n * outline_size);
            SetBone(builder, mdoc->bone_index);
            AddTriangle(builder, base+0, base+1, base+3);
            AddTriangle(builder, base+1, base+2, base+3);
        }

        GetCurrentFrame(mdoc)->outline = CreateMesh(ALLOCATOR_DEFAULT, builder, NAME_NONE, true);
        GetCurrentFrame(mdoc)->outline_version = g_workspace.zoom_version;

        Free(builder);

        return GetCurrentFrame(mdoc)->outline;
    }

    void SetFaceColor(MeshDocument* m, u8 color) {
        MeshFrameData* frame = GetCurrentFrame(m);
        int count = 0;
        for (u16 face_index=0; face_index < frame->geom.face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            if (!IsSelected(f)) continue;
            f->color = color;
            count++;
        }

        if (!count) return;

        MarkDirty(frame);
    }

    void SetFaceOpacity(MeshDocument* m, float opacity) {
        MeshFrameData* frame = GetCurrentFrame(m);
        int count = 0;
        for (u16 face_index = 0; face_index < frame->geom.face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            if (!IsSelected(f)) continue;
            f->opacity = opacity;
            count++;
        }

        if (!count) return;

        MarkDirty(frame);
    }

    int CountSharedEdges(MeshFrameData* frame, u16 face_index0, u16 face_index1) {
        int shared_edge_count = 0;

        FaceData* f0 = GetFace(frame, face_index0);
        for (u16 edge_index=0; edge_index < f0->edge_count; edge_index++) {
            EdgeData* e = GetFaceEdge(frame, f0, edge_index);
            if (e->face_left == face_index1 || e->face_right == face_index1)
                shared_edge_count++;
        }

        return shared_edge_count;
    }

    static u16 GetFaceEdgeIndex(FaceData* f, u16 edge_index) {
        for (u16 i = 0; i < f->edge_count; i++) {
            if (f->edges[i] == edge_index)
                return i;
        }
        return U16_MAX;
    }

    void DeleteFaceEdge(FaceData* f, u16 edge_index) {
        u16 face_edge_index = GetFaceEdgeIndex(f, edge_index);
        assert(face_edge_index != U16_MAX);

        for (u16 i = face_edge_index; i < f->edge_count - 1; i++)
            f->edges[i] = f->edges[i + 1];

        f->edge_count--;
    }

    void DeleteVertexInternal(MeshFrameData* frame, VertexData* v) {
        assert(IsValid(frame, v));
        assert(v->edge_count == 0);
        v->edge_count--;
        if (v->edge_count > 0)
            return;

        u16 vertex_index = GetIndex(frame, v);
        for (u16 edge_index=0; edge_index < frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            if (e->v0 > vertex_index) e->v0--;
            if (e->v1 > vertex_index) e->v1--;
        }

        for (u16 i = vertex_index; i < frame->geom.vert_count - 1; i++)
            frame->geom.verts[i] = frame->geom.verts[i + 1];
    }

    void CollapseVertex(MeshFrameData* frame, u16 vertex_index) {
        assert(vertex_index < frame->geom.vert_count);

        for (u16 edge_index=frame->geom.edge_count; edge_index > 0; edge_index--) {
            EdgeData* e = GetEdge(frame, edge_index-1);
            if (e->v0 != vertex_index && e->v1 != vertex_index)
                continue;
       }
    }


    #if 0




                // Remove edge from faces
                    for (int face_side=0; face_side<2; face_side++) {
                        u16 face_index = e->face_index[face_side];
                        if (face_index == U16_MAX)


                            for (u16 face_index=frame->geom.face_count; face_index > 0; face_index--) {
                                FaceData* f = GetFace(frame, face_index-1);
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

                        for (int face_index=0; face_index < GetCurrentFrame(m)->geom.face_count; face_index++) {
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

                        Update(m);
                    }
    #endif

    void DeleteEdgeInternal(MeshFrameData* frame, EdgeData* e) {
    #if 0
                    }
        assert(IsValid(frame, e));

        u16 edge_index = GetIndex(frame, e);
        for (u16 i = edge_index; i < frame->geom.edge_count - 1; i++)
            frame->geom.edges[i] = frame->geom.edges[i + 1];

        DeleteVertexInternal(frame, GetVertex(frame, e->v0));
        DeleteVertexInternal(frame, GetVertex(frame, e->v1));
    #endif
    }

    void DeleteFace(MeshFrameData* frame, FaceData* f, bool include_verts) {
    #if 0
        assert(IsValid(frame, f));

        if (include_verts) {
            u16 vertices[MESH_MAX_FACE_VERTICES];
            u16 vertex_count = GetFaceVertices(frame, f, vertices);
            for (u16 i = 0; i < vertex_count; i++)
                DeleteVertexInternal(frame, GetVertex(frame, vertices[i]));

            for (u16 face_edge_index=0; face_edge_index < f->edge_count; face_edge_index++) {
                EdgeData* e = GetFaceEdge(frame, f, face_edge_index);
                if (e->face_left == GetIndex(frame, f))
                    e->face_left = U16_MAX;
                else if (e->face_right == GetIndex(frame, f))
                    e->face_right = U16_MAX;

                if (e->face_left == U16_MAX && e->face_right == U16_MAX)
                    DeleteEdgeInternal(frame, e);
            }
        }

        u16 face_index = GetIndex(frame, f);
        for (u16 i = face_index; i < frame->geom.face_count - 1; i++)
            frame->geom.faces[i] = frame->geom.faces[i + 1];
    #endif
    }

    void DeleteEdge(MeshFrameData* frame, u16 edge_index) {
    #if 0
        EdgeData* e = GetEdge(frame, edge_index);

        if (e->face_left != U16_MAX && e->face_right != U16_MAX) {
            // todo: dissolve by merging the two faces
            return;
        }

        if (e->face_left != U16_MAX)
            DeleteFace(frame, GetFace(frame, e->face_left), false);

        if (e->face_right != U16_MAX)
            DeleteFace(frame, GetFace(frame, e->face_right), false);
    #endif
    }

    void DeleteFaceInternal(MeshDocument* m, u16 face_index) {
    #if 0
        assert(face_index >= 0 && face_index < GetCurrentFrame(m)->geom.face_count);
        RemoveFaceVertices(m, face_index, 0, -1);
        for (int i=face_index; i < GetCurrentFrame(m)->geom.face_count - 1; i++)
            GetCurrentFrame(m)->faces[i] = GetCurrentFrame(m)->faces[i + 1];
        GetCurrentFrame(m)->geom.face_count--;
    #endif
    }


    void DeleteSelectedFaces(MeshDocument* m) {
    #if 0
        for (int face_index=GetCurrentFrame(m)->geom.face_count - 1; face_index>=0; face_index--) {
            FaceData& ef = GetCurrentFrame(m)->faces[face_index];
            if (!ef.selected)
                continue;

            DeleteFace(m, face_index);
        }
    #endif
    }

    void MergeFaces(MeshDocument* m, const EdgeData& shared_edge) {
    #if 0
        assert(CountSharedEdges(m, shared_edge.face_left, shared_edge.face_right) == 1);

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
        Update(m);
        MarkDirty(m);
    #endif
    }

    void DeleteSelectedVertices(MeshDocument* m) {
    #if 0
        int vertices[MESH_MAX_VERTICES];
        int vertex_count = GetSelectedVertices(m, vertices);
        for (int vertex_index=vertex_count-1; vertex_index>=0; vertex_index--)
            DeleteVertex(m, vertices[vertex_index]);

        MarkDirty(m);
    #endif
    }

    void InsertFaceVertices(MeshDocument* m, int face_index, int insert_at, int count) {
    #if 0
        FaceData& f = GetCurrentFrame(m)->faces[face_index];

        for (int vertex_index=f.vertex_count + count; vertex_index > insert_at; vertex_index--)
            f.vertices[vertex_index] = f.vertices[vertex_index-count];

        for (int i=0; i<count; i++)
            f.vertices[insert_at + i] = -1;

        f.vertex_count += count;
    #endif
    }

    void RemoveFaceVertices(MeshDocument* m, int face_index, int remove_at, int remove_count) {
    #if 0
        FaceData& f = GetCurrentFrame(m)->faces[face_index];
        if (remove_count == -1)
            remove_count = f.vertex_count - remove_at;

        assert(remove_at >= 0 && remove_at + remove_count <= f.vertex_count);

        for (int vertex_index=remove_at; vertex_index + remove_count < f.vertex_count; vertex_index++)
            f.vertices[vertex_index] = f.vertices[vertex_index + remove_count];

        f.vertex_count -= remove_count;
    #endif
    }

    int CreateFace(MeshDocument* m) {
    #if 0
        int selected_vertices[MESH_MAX_VERTICES];
        int selected_count = GetSelectedVertices(m, selected_vertices);
        if (selected_count < 3)
            return -1;

        if (GetCurrentFrame(m)->geom.face_count >= MESH_MAX_FACES)
            return -1;

        for (int i = 0; i < selected_count; i++) {
            int v0 = selected_vertices[i];
            int v1 = selected_vertices[(i + 1) % selected_count];

            int edge_index = GetEdge(m, v0, v1);
            if (edge_index != -1) {
                const EdgeData& e = GetCurrentFrame(m)->geom.edges[edge_index];
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
                const EdgeData& e = GetCurrentFrame(m)->geom.edges[edge_index];
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

        VertexAngle vertex_angles[MESH_MAX_VERTICES];
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

        int face_index = GetCurrentFrame(m)->geom.face_count++;
        FaceData& f = GetCurrentFrame(m)->faces[face_index];
        f.vertex_count = selected_count;
        f.color = best_color;
        f.opacity = 1.0f;
        f.selected = false;

        for (int i = 0; i < selected_count; i++)
            f.vertices[i] = vertex_angles[i].vertex_index;

        Update(m);
        MarkDirty(m);

        return face_index;
    #else
        return 0;
    #endif
    }

    int SplitFaces(MeshDocument* m, int v0, int v1) {
    #if 0
        if (GetCurrentFrame(m)->geom.face_count >= MESH_MAX_FACES)
            return -1;

        if (GetEdge(m, v0, v1) != -1)
            return -1;

        int face_index = 0;
        int v0_pos = -1;
        int v1_pos = -1;
        for (; face_index < GetCurrentFrame(m)->geom.face_count; face_index++) {
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

        if (face_index >= GetCurrentFrame(m)->geom.face_count)
            return -1;

        if (v0_pos > v1_pos)
        {
            int temp = v0_pos;
            v0_pos = v1_pos;
            v1_pos = temp;
        }

        FaceData& old_face = GetCurrentFrame(m)->faces[face_index];
        FaceData& new_face = GetCurrentFrame(m)->faces[GetCurrentFrame(m)->geom.face_count++];
        new_face.color = old_face.color;
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

        Update(m);
        MarkDirty(m);

        return GetEdge(m, old_face.vertices[v0_pos], old_face.vertices[(v0_pos + 1) % old_face.vertex_count]);
    #else
        return -1;
    #endif
    }

    int SplitEdge(MeshDocument* m, int edge_index, float edge_pos, bool update) {
    #if 0
        assert(edge_index >= 0 && edge_index < GetCurrentFrame(m)->edge_count);

        if (GetCurrentFrame(m)->vertex_count >= MESH_MAX_VERTICES)
            return -1;

        if (GetCurrentFrame(m)->edge_count >= MESH_MAX_VERTICES)
            return -1;

        EdgeData& e = GetCurrentFrame(m)->geom.edges[edge_index];
        VertexData& v0 = GetCurrentFrame(m)->vertices[e.v0];
        VertexData& v1 = GetCurrentFrame(m)->vertices[e.v1];

        // Save curve data before modifying
        Vec2 original_curve_offset = e.curve.offset;
        float original_curve_weight = e.curve.weight;
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

        int face_count = GetCurrentFrame(m)->geom.face_count;
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
                Update(m);

                int edge1 = GetEdge(m, original_v0, new_vertex_index);
                int edge2 = GetEdge(m, new_vertex_index, original_v1);

                if (edge1 >= 0) {
                    GetCurrentFrame(m)->geom.edges[edge1].curve.offset = offset1;
                    GetCurrentFrame(m)->geom.edges[edge1].curve.weight = weight1;
                }
                if (edge2 >= 0) {
                    GetCurrentFrame(m)->geom.edges[edge2].curve.offset = offset2;
                    GetCurrentFrame(m)->geom.edges[edge2].curve.weight = weight2;
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
            Update(m);
            MarkDirty(m);
        }

        return new_vertex_index;
    #else
        return -1;
    #endif
    }

    int HitTestVertex(const Vec2& position, const Vec2& hit_pos, float size_mult) {
        float size = g_workspace.select_size * size_mult;
        float dist = Length(hit_pos - position);
        return dist <= size;
    }

    int HitTestVertex(MeshDocument* m, const Mat3& transform, const Vec2& position, float size_mult) {
        MeshFrameData* frame = GetCurrentFrame(m);
        float size = g_workspace.select_size * size_mult;
        float best_dist = F32_MAX;
        int best_vertex = -1;
        for (int i = 0; i < frame->geom.vert_count; i++) {
            const VertexData& v = frame->geom.verts[i];
            float dist = Length(position - TransformPoint(transform, v.position));
            if (dist <= size && dist < best_dist) {
                best_vertex = i;
                best_dist = dist;
            }
        }

        return best_vertex;
    }

    int HitTestEdge(MeshDocument* m, const Mat3& transform, const Vec2& hit_pos, float* where, float size_mult) {
        const float size = g_workspace.select_size * 0.75f * size_mult;
        float best_dist = F32_MAX;
        int best_edge = -1;
        float best_where = 0.0f;

        MeshFrameData* frame = GetCurrentFrame(m);

        for (u16 i = 0; i < frame->geom.edge_count; i++) {
            const EdgeData& e = frame->geom.edges[i];
            Vec2 v0 = TransformPoint(transform, frame->geom.verts[e.v0].position);
            Vec2 v1 = TransformPoint(transform, frame->geom.verts[e.v1].position);

            if (HasCurve(&e)) {
                // Test against subdivided curve segments
                Vec2 control = TransformPoint(transform, GetEdgeControlPoint(frame, i));
                float weight = e.curve.weight > 0.0f ? e.curve.weight : 1.0f;
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

    void Center(MeshDocument* m) {
    #if 0
        MeshFrameData* frame =
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

        Update(m);
        MarkDirty(m);
        MarkModified();
    #endif
    }

    void SwapFace(MeshDocument* m, int face_index_a, int face_index_b) {
        FaceData temp = GetCurrentFrame(m)->geom.faces[face_index_a];
        GetCurrentFrame(m)->geom.faces[face_index_a] = GetCurrentFrame(m)->geom.faces[face_index_b];
        GetCurrentFrame(m)->geom.faces[face_index_b] = temp;
    }

    bool OverlapBounds(MeshDocument* m, const Vec2& position, const Bounds2& hit_bounds) {
        return Intersects(m->bounds + position, hit_bounds);
    }

    int HitTestFaces(MeshDocument* m, const Mat3& transform, const Vec2& position, int* faces, int max_faces) {
    #if 0
        int hit_count = 0;
        for (int i = GetCurrentFrame(m)->geom.face_count - 1; i >= 0 && hit_count < max_faces; i--) {
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
    #else
        return 0;
    #endif
    }

    int HitTestFace(MeshDocument* m, const Mat3& transform, const Vec2& position) {
        int faces[1];
        int hit_count = HitTestFaces(m, transform, position, faces, 1);
        return hit_count > 0 ? faces[0] : -1;
    }

    static void ParseVertex(MeshDocument* m, Tokenizer& tk) {
        if (GetCurrentFrame(m)->geom.vert_count >= MESH_MAX_VERTICES)
            ThrowError("too many vertices");

        f32 x;
        if (!ExpectFloat(tk, &x))
            ThrowError("missing vertex x coordinate");

        f32 y;
        if (!ExpectFloat(tk, &y))
            ThrowError("missing vertex y coordinate");

        VertexData& v = GetCurrentFrame(m)->geom.verts[GetCurrentFrame(m)->geom.vert_count++];
        v = {};
        v.position = {x,y};
    }

    static void ParseFaceColor(FaceData* f, Tokenizer& tk) {
        int color=0;
        if (!ExpectInt(tk, &color))
            ThrowError("missing face color x value");

        float opacity = 1.0f;
        ExpectFloat(tk, &opacity);

        f->color = static_cast<u8>(color);
        f->opacity = Clamp01(opacity);
    }

    static void ParseEdge(MeshDocument* m, Tokenizer& tk) {

        MeshFrameData* frame = GetCurrentFrame(m);
        if (frame->geom.edge_count >= MESH_MAX_EDGES)
            ThrowError("too many edges");

        int v0 = 0;
        int v1 = 0;
        float ox = 0.0f;
        float oy = 0.0f;
        float weight = 1.0f;

        ExpectInt(tk, &v0);
        ExpectInt(tk, &v1);
        ExpectFloat(tk, &ox);
        ExpectFloat(tk, &oy);
        ExpectFloat(tk, &weight);

        EdgeData* e = GetEdge(frame, frame->geom.edge_count++);
        e->v0 = static_cast<u16>(v0);;
        e->v1 = static_cast<u16>(v1);
        e->curve.offset = {ox, oy};
        e->curve.weight = weight;
    }

    static void ParseFace(MeshDocument* m, Tokenizer& tk) {
        if (GetCurrentFrame(m)->geom.face_count >= MESH_MAX_FACES)
            ThrowError("too many faces");

        MeshFrameData* frame = GetCurrentFrame(m);
        FaceData* f = frame->geom.faces + frame->geom.face_count++;
        *f = {};
        f->opacity = 1.0f;

        int e;
        while (ExpectInt(tk, &e))
            f->edges[f->edge_count++] = static_cast<u16>(e);

        if (f->edge_count < 3) {
            LogError("face has fewer than 3 vertices, skipping");
            return;
        }

        f->color = 0;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "c"))
                ParseFaceColor(f, tk);
            else
                break;
        }
    }

    static void ParseDepth(MeshDocument* m, Tokenizer& tk) {
        float depth = 0.0f;
        if (!ExpectFloat(tk, &depth))
            ThrowError("missing mesh depth value");

        m->depth = (int)(depth * (MESH_MAX_DEPTH - MESH_MIN_DEPTH) + MESH_MIN_DEPTH);
    }

    static void ParsePalette(MeshDocument* m, Tokenizer& tk) {
        int palette = 0;
        if (!ExpectInt(tk, &palette))
            ThrowError("missing mesh palette value");

        m->palette = palette;
    }

    static void ParseSkeleton(MeshDocument* m, Tokenizer& tk) {
        if (!ExpectQuotedString(tk))
            ThrowError("missing skeleton name");

        m->skeleton_name = GetName(tk);
    }

    #if 0
    static void FinalizeFrame(MeshDocument* m, PendingCurve* pending_curves, int pending_curve_count) {
        Update(m);

        for (int i = 0; i < pending_curve_count; i++) {
            int edge = GetEdge(m, pending_curves[i].v0, pending_curves[i].v1);
            if (edge != -1) {
                GetCurrentFrame(m)->geom.edges[edge].curve.offset = pending_curves[i].offset;
                GetCurrentFrame(m)->geom.edges[edge].curve.weight = pending_curves[i].weight;
            }
        }

        MarkDirty(m);
        ToMesh(m, false);
    }
    #endif

    void LoadMeshData(MeshDocument* m, Tokenizer& tk) {
        //..PendingCurve pending_curves[MESH_MAX_EDGES];
        int pending_curve_count = 0;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "f")) {
                ParseFace(m, tk);
            } else if (ExpectIdentifier(tk, "frame")) {
                if (GetCurrentFrame(m)->geom.vert_count > 0) {
                    //FinalizeFrame(m, pending_curves, pending_curve_count);
                    pending_curve_count = 0;

                    // Add a new frame
                    if (m->frame_count < MESH_MAX_FRAMES) {
                        m->frame_count++;
                        m->current_frame = m->frame_count - 1;
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
                ParseEdge(m, tk);
            } else {
                char error[1024];
                GetString(tk, error, sizeof(error) - 1);
                ThrowError("invalid token '%s' in mesh", error);
            }
        }

        // Finalize the last (or only) frame
        //FinalizeFrame(m, pending_curves, pending_curve_count);

        // Reset to first frame for editing
        m->current_frame = 0;
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

    static void LoadMeshData(Document* a) {
        assert(a);
        assert(a->def->type == ASSET_TYPE_MESH);
        MeshDocument* m = static_cast<MeshDocument *>(a);

        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path.value);
        Tokenizer tk;
        Init(tk, contents.c_str());
        LoadMeshData(m, tk);
    }

    MeshDocument* LoadMeshData(const std::filesystem::path& path) {
        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
        Tokenizer tk;
        Init(tk, contents.c_str());

        MeshDocument* m = static_cast<MeshDocument*>(CreateDocument(path));
        assert(m);
        InitMeshDocument(m);
        LoadMeshData(m);
        return m;
    }

    static void LoadMeshMetaData(Document* a, Props* meta) {
        assert(a);
        assert(a->def->type == ASSET_TYPE_MESH);
        MeshDocument* m = static_cast<MeshDocument*>(a);
        m->palette = meta->GetInt("mesh", "palette", m->palette);
        // atlas_name is set by atlas post-load, not stored in mesh metadata
    }

    static void SaveMeshMetaData(Document* a, Props* meta) {
        assert(a);
        assert(a->def->type == ASSET_TYPE_MESH);
        MeshDocument* m = static_cast<MeshDocument*>(a);
        meta->SetInt("mesh", "palette", m->palette);
        // atlas_name is owned by the atlas, not saved in mesh metadata
    }

    static void SaveFrameData(MeshFrameData* frame, Stream* stream) {
        for (int i=0; i<frame->geom.vert_count; i++) {
            const VertexData& v = frame->geom.verts[i];
            WriteCSTR(stream, "v %f %f", v.position.x, v.position.y);
            WriteCSTR(stream, "\n");
        }

        WriteCSTR(stream, "\n");

        for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++) {
            const EdgeData* e = GetEdge(frame, edge_index);
            WriteCSTR(stream, "e %d %d", e->v0, e->v1);
            if (LengthSqr(e->curve.offset) > 0.0001f)
                WriteCSTR(stream, " %f %f %f", e->curve.offset.x, e->curve.offset.y, e->curve.weight);
            WriteCSTR(stream, "\n");
        }

        for (int i=0; i<frame->geom.face_count; i++) {
            const FaceData& f = frame->geom.faces[i];

            WriteCSTR(stream, "f");
            for (u16 edge_index=0; edge_index<f.edge_count; edge_index++)
                WriteCSTR(stream, " %d", f.edges[edge_index]);

            WriteCSTR(stream, " c %d", f.color);
            if (f.opacity < 1.0f)
                WriteCSTR(stream, " %f", f.opacity);

            WriteCSTR(stream, "\n");
        }

        // Write curve data for curved edges
        for (int i = 0; i < frame->geom.edge_count; i++) {
            const EdgeData& e = frame->geom.edges[i];
            if (LengthSqr(e.curve.offset) > 0.0001f) {
                WriteCSTR(stream, "curve %d %d %f %f %f\n", e.v0, e.v1, e.curve.offset.x, e.curve.offset.y, e.curve.weight);
            }
        }
    }

    void SaveMeshData(MeshDocument* m, Stream* stream) {
        if (m->skeleton_name != nullptr)
            WriteCSTR(stream, "s \"%s\"\n", m->skeleton_name->value);

        WriteCSTR(stream, "d %f\n", (m->depth - MESH_MIN_DEPTH) / (float)(MESH_MAX_DEPTH - MESH_MIN_DEPTH));
        WriteCSTR(stream, "p %d\n", m->palette);
        WriteCSTR(stream, "\n");

        // Write each frame
        for (int frame_index = 0; frame_index < m->frame_count; frame_index++) {
            MeshFrameData* frame = &m->frames[frame_index];

            // Write frame header (only if multiple frames or frame has hold)
            if (m->frame_count > 1 || frame->hold > 0) {
                WriteCSTR(stream, "frame");
                if (frame->hold > 0)
                    WriteCSTR(stream, " h %d", frame->hold);
                WriteCSTR(stream, "\n");
            }

            SaveFrameData(frame, stream);

            if (frame_index < m->frame_count - 1)
                WriteCSTR(stream, "\n");
        }
    }

    static void SaveMeshData(Document* doc, const std::filesystem::path& path) {
        assert(doc->def->type == ASSET_TYPE_MESH);
        MeshDocument* mdoc = static_cast<MeshDocument*>(doc);

        if (NeedsAtlasAssignment(mdoc)) {
            AutoAssignMeshToAtlas(mdoc);
        }

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        SaveMeshData(mdoc, stream);
        SaveStream(stream, path);
        Free(stream);
    }

    Document* NewMeshData(const std::filesystem::path& path) {
        constexpr const char* default_mesh =
            "v -1 -1\n"
            "v 1 -1\n"
            "v 1 1\n"
            "v -1 1\n"
            "\n"
            "f 0 1 2 3 c 0 0\n";

        std::string text = std::format(default_mesh);

        if (g_workspace.selected_asset_count == 1) {
            Document* selected = GetFirstSelectedAsset();
            assert(selected);
            if (selected->def->type == ASSET_TYPE_MESH)
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

    static void CloneMeshDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_MESH);
        MeshDocument* mdoc = static_cast<MeshDocument*>(doc);
        mdoc->playing = nullptr;
    }

    int GetSelectedVertices(MeshDocument* m, int vertices[MESH_MAX_VERTICES]) {
        MeshFrameData* frame = GetCurrentFrame(m);
        int selected_vertex_count=0;
        for (u16 vertex_index=0; vertex_index<frame->geom.vert_count; vertex_index++) {
            VertexData* v = GetVertex(frame, vertex_index);
            if (!IsSelected(v)) continue;
            vertices[selected_vertex_count++] = vertex_index;
        }
        return selected_vertex_count;
    }

    u16 GetSelectedEdges(MeshDocument* m, u16 edges[MESH_MAX_EDGES]) {
        MeshFrameData* frame = GetCurrentFrame(m);
        u16 selected_edge_count=0;
        for (u16 edge_index=0; edge_index<frame->geom.edge_count; edge_index++) {
            EdgeData* e = GetEdge(frame, edge_index);
            if (!IsSelected(e)) continue;
            edges[selected_edge_count++] = edge_index;
        }

        return selected_edge_count;
    }

    Vec2 HitTestSnap(MeshDocument* m, const Vec2& position) {
        float best_dist_sqr = LengthSqr(position);
        Vec2 best_snap = VEC2_ZERO;
        return best_snap;
    }

    Vec2 GetEdgePoint(MeshFrameData* frame, u16 edge_index, float t) {
        EdgeData* e = GetEdge(frame, edge_index);
        const Vec2& p0 = GetVertex(frame, e->v0)->position;
        const Vec2& p2 = GetVertex(frame, e->v1)->position;

        if (HasCurve(e)) {
            Vec2 p1 = GetEdgeControlPoint(frame, edge_index);
            float weight = e->curve.weight > 0.0f ? e->curve.weight : 1.0f;
            return EvalQuadraticBezier(p0, p1, p2, t, weight);
        }

        return Mix(p0, p2, t);
    }

    void SetOrigin(MeshDocument* m, const Vec2& origin) {
        Vec2 delta = m->position - origin;
        for (int vertex_index = 0; vertex_index < GetCurrentFrame(m)->geom.vert_count; vertex_index++)
            GetCurrentFrame(m)->geom.verts[vertex_index].position += delta;

        m->position = origin;
        Update(m);
        MarkDirty(m);
    }

    void SetBone(MeshDocument* mdoc, u8 bone_index) {
        mdoc->bone_index = bone_index;
        Update(mdoc);
        MarkDirty(mdoc);
    }

    void ClearBone(MeshDocument* m) {
        SetBone(m, U8_MAX);
    }

    EdgeData* GetEdge(MeshFrameData* frame, u16 edge_index) {
        assert(frame);
        assert(edge_index < frame->geom.edge_count);
        return frame->geom.edges + edge_index;
    }

    int GetEdge(MeshFrameData* frame, int v0, int v1) {
        for (int i = 0; i < frame->geom.edge_count; i++) {
            EdgeData& e = frame->geom.edges[i];
            if ((e.v0 == v0 && e.v1 == v1) || (e.v0 == v1 && e.v1 == v0))
                return i;
        }
        return -1;
    }

    static void AddFaceToPath(
        Rasterizer* rasterizer,
        MeshFrameData* frame,
        FaceData* f,
        const Vec2& offset) {
        constexpr int CURVE_SEGMENTS = 8;

    #if 0
        const float scale = static_cast<float>(g_editor.atlas.dpi);

        for (u16 edge_index = 0; edge_index < f.edge_count; edge_index++) {
            EdgeData* e = GetFaceEdge(frame, f, edge_index);
            Vec2 p0 = frame->geom.verts[e->v0].position;
            Vec2 p1 = frame->geom.verts[e->v1].position;

            Vec2 edge_normal = {0, 0};
            Vec2 pos0 = p0 + edge_normal + offset;

            if (edge_index == 0)
                MoveTo(rasterizer, pos0.x * scale, pos0.y * scale);

            if (edge_index >= 0) {
                EdgeData& e = frame->geom.edges[edge_idx];
                if (LengthSqr(e.curve.offset) > 0.0001f) {
                    // Tessellate curved edge using rational bezier
                    Vec2 control = (p0 + p1) * 0.5f + e.curve.offset;
                    float w = e.curve.weight;

                    // Flip curve direction if edge is reversed relative to face winding
                    bool reversed = (e.v0 != v0_idx);

                    for (int s = 1; s <= CURVE_SEGMENTS; s++) {
                        float t = (float)s / CURVE_SEGMENTS;
                        if (reversed) t = 1.0f - t;

                        Vec2 pt = EvalQuadraticBezier(p0, control, p1, reversed ? 1.0f - t : t, w);
                        pt += edge_normal;
                        pt += offset;
                        LineTo(rasterizer, pt.x * scale, pt.y * scale);
                    }
                    continue;
                }
            }

            Vec2 pos1 = p1 + edge_normal + offset;
            LineTo(rasterizer, pos1.x * scale, pos1.y * scale);
        }
    #endif
    }

    void Rasterize(
        MeshDocument* mdoc,
        int frame_index,
        Rasterizer* rasterizer,
        PixelData* pixels,
        const Vec2Int& position,
        int padding) {

        MeshFrameData* frame = GetFrame(mdoc, frame_index);

        RectInt rect = {
            position.x - padding,
            position.y - padding,
            position.x + mdoc->size.x + padding * 2,
            position.y + mdoc->size.y + padding * 2};

        Set(pixels, rect, COLOR32_TRANSPARENT);

        int palette_index = g_editor.palette_map[mdoc->palette];

        for (u16 face_index = 0; face_index < frame->geom.face_count; face_index++) {
            FaceData* f = GetFace(frame, face_index);
            if (f->edge_count < 3) continue;

            Color color = g_editor.palettes[palette_index].colors[f->color];
            SetColor(rasterizer, color.r, color.g, color.b, color.a * f->opacity);
            BeginPath(rasterizer);
            AddFaceToPath(rasterizer, frame, f, -mdoc->bounds.min);
            Fill(rasterizer, Vec2Int{position.x, position.y});
        }
    }

    static void PostLoadMeshData(Document* a) {
        assert(a);
        assert(a->def->type == ASSET_TYPE_MESH);
        MeshDocument* m = static_cast<MeshDocument*>(a);
        if (m->skeleton_name)
            m->skeleton = static_cast<SkeletonDocument*>(FindDocument(ASSET_TYPE_SKELETON, m->skeleton_name));

        // Auto-snap all vertices to pixel grid
        bool modified = false;
        for (int frame_index = 0; frame_index < m->frame_count; frame_index++) {
            MeshFrameData* frame = &m->frames[frame_index];
            for (int vi = 0; vi < frame->geom.vert_count; vi++) {
                Vec2 world_pos = m->position + frame->geom.verts[vi].position;
                Vec2 snapped = SnapToPixelGrid(world_pos);
                if (world_pos.x != snapped.x || world_pos.y != snapped.y) {
                    frame->geom.verts[vi].position = snapped - m->position;
                    modified = true;
                }
            }

            frame->dirty = true;
        }

        if (modified) {
            Update(m);
            MarkDirty(m);
            MarkModified(a);
        }
    }

    static void DestroyMeshData(Document* a) {
        MeshDocument* m = static_cast<MeshDocument*>(a);

        // Free all frame meshes
        for (int i = 0; i < m->frame_count; i++) {
            Free(m->frames[i].mesh);
            Free(m->frames[i].outline);
        }

        // Free playing mesh if any
        Free(m->playing);

        Free(m);
        m = nullptr;
    }

    static Mesh* ToMeshWithAtlasQuad(MeshDocument* mdoc, AtlasDocument* adoc, const AtlasRect& rect) {
        if (!mdoc) return nullptr;

        // Use tight rect around mesh bounds
        Vec2 min_pos = rect.mesh_bounds.min;
        Vec2 max_pos = rect.mesh_bounds.max;

        // 4 vertices, 2 triangles
        MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_DEFAULT, 4, 6);

        float depth = 0.01f + 0.99f * (mdoc->depth - MESH_MIN_DEPTH) / (float)(MESH_MAX_DEPTH - MESH_MIN_DEPTH);
        int atlas_idx = GetAtlasIndex(adoc);

        // Get bone index if skinned (single bone only)
        int bone_index = mdoc->bone_index;

        // Quad corners: bottom-left, bottom-right, top-right, top-left
        Vec2 corners[4] = {
            {min_pos.x, min_pos.y},
            {max_pos.x, min_pos.y},
            {max_pos.x, max_pos.y},
            {min_pos.x, max_pos.y}
        };

        for (int i = 0; i < 4; i++) {
            MeshVertex v = {};
            v.position = corners[i];
            v.depth = depth;
            v.opacity = 1.0f;
            v.uv = GetAtlasUV(adoc, rect, rect.mesh_bounds, corners[i]);
            v.atlas_index = atlas_idx;


            AddVertex(builder, v);
        }

        // Two triangles for quad
        AddTriangle(builder, 0, 1, 2);
        AddTriangle(builder, 0, 2, 3);

        Mesh* result = CreateMesh(ALLOCATOR_DEFAULT, builder, mdoc->name, false);
        Free(builder);

        // Set animation info for animated meshes
        if (rect.frame_count > 1) {
            float frame_width_pixels = static_cast<float>(rect.width) / static_cast<float>(rect.frame_count);
            float frame_width_uv = frame_width_pixels / static_cast<float>(adoc->size.x);
            SetAnimationInfo(result, rect.frame_count, 12, frame_width_uv);
        }

        return result;
    }

    static void ImportMesh(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        (void)config;
        (void)meta;

        assert(doc);
        assert(doc->def->type == ASSET_TYPE_MESH);
        MeshDocument* mesh_data = static_cast<MeshDocument*>(doc);

        Mesh* m = nullptr;

        // Check if mesh is in an atlas - use tight rect quad
        AtlasRect* rect = nullptr;
        AtlasDocument* atlas = FindAtlasForMesh(mesh_data->name, &rect);
        if (atlas && rect) {
            m = ToMeshWithAtlasQuad(mesh_data, atlas, *rect);
        }

        // Fall back to normal mesh if no atlas or pixel hull failed
        if (!m) {
            m = ToMesh(mesh_data, false);
        }

        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_MESH;
        header.version = 1;

        Stream* stream = CreateStream(nullptr, 4096);
        WriteAssetHeader(stream, &header);
        SerializeMesh(m, stream);
        SaveStream(stream, path);
        Free(stream);
    }

    static void InitMeshDocument(MeshDocument* m) {
        m->vtable = {
            .destructor = DestroyMeshData,
            .load = LoadMeshData,
            .post_load = PostLoadMeshData,
            .save = SaveMeshData,
            .load_metadata = LoadMeshMetaData,
            .save_metadata = SaveMeshMetaData,
            .draw = DrawMesh,
            .clone = CloneMeshDocument
        };

        InitMeshEditor(m);
    }

    static void InitMeshDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_MESH);
        InitMeshDocument(static_cast<MeshDocument*>(doc));
    }

    void InitMeshDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_MESH,
            .size = sizeof(MeshDocument),
            .ext = ".mesh",
            .init_func = InitMeshDocument,
            .import_func = ImportMesh
        });
    }
}
