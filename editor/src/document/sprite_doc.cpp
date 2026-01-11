//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"
#include "atlas_doc.h"

using namespace noz::editor::shape;

namespace noz::editor {

    extern void InitSpriteEditor(SpriteDocument* sdoc);

    static void DrawSpriteDocument(Document* doc) {
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);

        // Try to draw from atlas
        AtlasRect* rect = nullptr;
        AtlasDocument* atlas = FindAtlasForSprite(sdoc->name, &rect);
        if (atlas && rect) {
            // Ensure atlas is rendered and texture is uploaded to GPU
            if (!atlas->pixels) {
                RegenerateAtlas(atlas);
            }
            SyncAtlasTexture(atlas);
            if (!atlas->material) goto fallback;

            // Get geometry at exact bounds
            Vec2 min, max;
            float u_min, v_min, u_max, v_max;
            GetExportQuadGeometry(atlas, *rect, &min, &max, &u_min, &v_min, &u_max, &v_max);

            // Build textured quad
            MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
            MeshVertex v = {};
            v.depth = 0.5f;
            v.opacity = 1.0f;

            // Bottom-left
            v.position = min;
            v.uv = {u_min, v_min};
            AddVertex(builder, v);

            // Bottom-right
            v.position = {max.x, min.y};
            v.uv = {u_max, v_min};
            AddVertex(builder, v);

            // Top-right
            v.position = max;
            v.uv = {u_max, v_max};
            AddVertex(builder, v);

            // Top-left
            v.position = {min.x, max.y};
            v.uv = {u_min, v_max};
            AddVertex(builder, v);

            AddTriangle(builder, 0, 1, 2);
            AddTriangle(builder, 0, 2, 3);

            Mesh* quad = CreateMesh(ALLOCATOR_SCRATCH, builder, nullptr, false);
            Free(builder);

            BindMaterial(atlas->material);
            BindColor(COLOR_WHITE);
            DrawMesh(quad, Translate(doc->position));
            return;
        }

    fallback:
        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindDepth(0.0f);
        DrawMesh(g_workspace.quad_mesh, Translate(doc->position));
    }

    static void ParseAnchor(Shape* shape, Path* p, Tokenizer& tk) {
        if (shape->anchor_count >= SHAPE_MAX_ANCHORS)
            ThrowError("too many anchors");

        Anchor* a = &shape->anchors[shape->anchor_count++];
        p->anchor_count++;
        a->position.x = ExpectFloat(tk);
        a->position.y = ExpectFloat(tk);
        a->curve = ExpectFloat(tk, 0.0f);
    }

    static void ParsePath(SpriteFrame* f, Tokenizer& tk) {
        Shape* shape = &f->shape;
        Path* p = &shape->paths[shape->path_count++];
        p->anchor_start = shape->anchor_count;
        p->anchor_count = 0;
        p->stroke_color = 0;
        p->fill_color = 0;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "c")) {
                p->fill_color = (u8)ExpectInt(tk);
            } else if (ExpectIdentifier(tk, "a")) {
                ParseAnchor(shape, p, tk);
            } else {
                break;
            }
        }
    }

    static void LoadSpriteDocument(SpriteDocument* sdoc, Tokenizer& tk) {
        SpriteFrame* f = sdoc->frames + sdoc->frame_count++;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "p")) {
                ParsePath(f, tk);
            } else if (ExpectIdentifier(tk, "c")) {
                sdoc->palette = (u8)ExpectInt(tk);
            } else if (ExpectIdentifier(tk, "d")) {
                ExpectFloat(tk);
            } else if (ExpectIdentifier(tk, "f")) {
                f = sdoc->frames + sdoc->frame_count++;
            } else {
                char error[1024];
                GetString(tk, error, sizeof(error) - 1);
                ThrowError("invalid token '%s' in sprite", error);
            }
        }

        for (u16 fi = 0; fi < sdoc->frame_count; ++fi) {
            shape::UpdateSamples(&sdoc->frames[fi].shape);
            shape::UpdateBounds(&sdoc->frames[fi].shape);
        }
    }

    void UpdateBounds(SpriteDocument* sdoc) {
        if (sdoc->frame_count <= 0)
            return;
        
        Bounds2 bounds = sdoc->frames[0].shape.bounds;
        for (u16 fi = 1; fi < sdoc->frame_count; ++fi)
            bounds = Union(bounds, sdoc->frames[fi].shape.bounds);
        sdoc->bounds = bounds;
    }

    static void LoadSpriteDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SPRITE);
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);

        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, doc->path.value);
        Tokenizer tk;
        Init(tk, contents.c_str());
        LoadSpriteDocument(sdoc, tk);
        UpdateBounds(sdoc);
    }

    static void ImportSprite(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {        
        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_SPRITE;
        header.version = 1;

        Stream* stream = CreateStream(nullptr, 4096);
        WriteAssetHeader(stream, &header);
        SaveStream(stream, path);
        Free(stream);
    }

    static void SaveSpriteFrame(SpriteFrame* f, Stream* stream) {
        Shape* shape = &f->shape;

        for (u16 p_idx = 0; p_idx < shape->path_count; ++p_idx) {
            Path* path = &shape->paths[p_idx];

            // Write path header with fill color
            WriteCSTR(stream, "p c %d\n", path->fill_color);

            // Write anchors
            for (u16 a_idx = 0; a_idx < path->anchor_count; ++a_idx) {
                Anchor* a = GetAnchor(shape, path, a_idx);
                WriteCSTR(stream, "a %f %f", a->position.x, a->position.y);
                if (Abs(a->curve) > FLT_EPSILON)
                    WriteCSTR(stream, " %f", a->curve);
                WriteCSTR(stream, "\n");
            }

            WriteCSTR(stream, "\n");
        }
    }

    static void SaveSpriteDocument(SpriteDocument* sdoc, Stream* stream) {
        if (HasSkin(sdoc))
            WriteCSTR(stream, "s \"%s\"\n", sdoc->skin.skeleton_name->value);

        //WriteCSTR(stream, "d %f\n", (sdoc->depth - MESH_MIN_DEPTH) / (float)(MESH_MAX_DEPTH - MESH_MIN_DEPTH));
        WriteCSTR(stream, "c %d\n", sdoc->palette);
        WriteCSTR(stream, "\n");

        // Write each frame
        for (u16 frame_index = 0; frame_index < sdoc->frame_count; frame_index++) {
            SpriteFrame* f = GetFrame(sdoc, frame_index);

            // Write frame header (only if multiple frames or frame has hold)
            if (sdoc->frame_count > 1 || f->hold > 0) {
                WriteCSTR(stream, "f");
                if (f->hold > 0)
                    WriteCSTR(stream, " h %d", f->hold);
                WriteCSTR(stream, "\n");
            }

            SaveSpriteFrame(f, stream);

            if (frame_index < sdoc->frame_count - 1)
                WriteCSTR(stream, "\n");
        }
    }

    static void SaveSpriteDocument(Document* doc, const std::filesystem::path& path) {
        assert(doc->def->type == ASSET_TYPE_SPRITE);
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);
        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        SaveSpriteDocument(sdoc, stream);
        SaveStream(stream, path);
        Free(stream);
    }
    
    void InitSpriteDocument(Document* doc) {
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);
        sdoc->vtable.load = LoadSpriteDocument;
        sdoc->vtable.draw = DrawSpriteDocument;
        sdoc->vtable.save = SaveSpriteDocument;

        InitSpriteEditor(sdoc);
    }

    void InitSpriteDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_SPRITE,
            .size = static_cast<int>(sizeof(SpriteDocument)),
            .ext = ".sprite",
            .init_func = InitSpriteDocument,
            .import_func = ImportSprite
        });
    }
} // namespace noz::editor