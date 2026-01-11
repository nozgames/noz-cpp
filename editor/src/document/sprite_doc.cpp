//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"

using namespace noz::editor::shape;

namespace noz::editor {

    extern void InitSpriteEditor(SpriteDocument* sdoc);

    static void DrawSpriteDocument(Document* doc) {
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

        for (u16 fi = 0; fi < sdoc->frame_count; ++fi)
            shape::UpdateSamples(&sdoc->frames[fi].shape);
    }

    static void LoadSpriteDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SPRITE);
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);

        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, doc->path.value);
        Tokenizer tk;
        Init(tk, contents.c_str());
        LoadSpriteDocument(sdoc, tk);
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

    void InitSpriteDocument(Document* doc) {
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);
        sdoc->vtable.load = LoadSpriteDocument;
        sdoc->vtable.draw = DrawSpriteDocument;

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