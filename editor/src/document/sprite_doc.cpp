//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"

namespace noz::editor {

    extern void InitSpriteEditor(SpriteDocument* sdoc);

    static void DrawSpriteDocument(Document* doc) {
        BindMaterial(g_workspace.editor_material);
        BindColor(COLOR_WHITE);
        BindDepth(0.0f);
        DrawMesh(g_workspace.quad_mesh, Translate(doc->position));
    }

    static void ParseAnchor(SpriteFrame& f, Tokenizer& tk) {
        if (f.shape.anchor_count >= shape::SHAPE_MAX_ANCHORS)
            ThrowError("too many anchors");

        shape::Anchor& a = f.shape.anchors[f.shape.anchor_count++];
        a.position.x = ExpectFloat(tk);
        a.position.y = ExpectFloat(tk);
    }

    static void ParseSegment(SpriteFrame& f, Tokenizer& tk) {
        shape::Segment& s = f.shape.segments[f.shape.segment_count++];
        s.anchor0 = (u16)ExpectInt(tk);
        s.anchor1 = (u16)ExpectInt(tk);        
        s.curve.offset.x = ExpectFloat(tk);
        s.curve.offset.y = ExpectFloat(tk);
        s.curve.weight = ExpectFloat(tk, 0.0f);
    }

    static void ParsePath(SpriteFrame& f, Tokenizer& tk) {
        shape::Path& p = f.shape.paths[f.shape.path_count++];

        while (!IsEOF(tk)) {
            int s_idx = ExpectInt(tk, -1);
            if (s_idx == -1) break;
            p.segments[p.segment_count++] = (u16)s_idx;
        }

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "c")) {
                p.fill_color = (u8)ExpectInt(tk);
            } else {
                break;
            }
        }
    }

    static void LoadSpriteDocument(SpriteDocument* sdoc, Tokenizer& tk) {
        SpriteFrame* f = sdoc->frames + sdoc->frame_count++;

        while (!IsEOF(tk)) {
            if (ExpectIdentifier(tk, "a")) {
                ParseAnchor(*f, tk);
            } else if (ExpectIdentifier(tk, "s")) {
                ParseSegment(*f, tk);
            } else if (ExpectIdentifier(tk, "p")) {
                ParsePath(*f, tk);
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
            .size = sizeof(SpriteDocument),
            .ext = ".sprite",
            .init_func = InitSpriteDocument,
            .import_func = ImportSprite
        });
    }
} // namespace noz::editor