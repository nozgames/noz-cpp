//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {
    void ImportFontDocument(Document* ea, const std::filesystem::path& path, Props* config, Props* meta);

    static void DrawFontDocument(Document* a) {
        BindMaterial(g_workspace.editor_mesh_material);
        BindColor(COLOR_WHITE);
        DrawMesh(SPRITE_ASSET_ICON_FONT, Translate(a->position));
    }

    static void InitFontDocument(FontDocument* doc) {
        doc->vtable = {
            .draw = DrawFontDocument,
        };
    }

    static void InitFontDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_FONT);
        InitFontDocument(static_cast<FontDocument*>(doc));
    }

    void InitFontDocumentDef() {
        InitDocumentDef({
            .type=ASSET_TYPE_FONT,
            .size=sizeof(FontDocument),
            .ext=".ttf",
            .init_func=InitFontDocument,
            .import_func=ImportFontDocument
        });
    }
}
