//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    static void DrawBinDocument(Document* doc) {
        BindMaterial(g_view.editor_mesh_material);
        BindColor(COLOR_WHITE);
        DrawMesh(MESH_ASSET_ICON_BIN, Translate(doc->position));
    }

    static void InitBinDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_BIN);
        doc->vtable = {
            .draw = DrawBinDocument,
        };
    }

    static void ImportBin(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        Stream* stream = CreateStream(nullptr, 4096);

        AssetHeader header = {};
        header.type = ASSET_TYPE_BIN;
        header.signature = ASSET_SIGNATURE;
        header.version = 0;
        WriteAssetHeader(stream, &header);

        Stream* input_stream = LoadStream(ALLOCATOR_DEFAULT, doc->path.value);
        WriteU32(stream, GetSize(input_stream));
        Copy(stream, input_stream);
        Free(input_stream);

        SaveStream(stream, path);
        Free(stream);
    }

    void InitBinDocumentDef() {
        InitDocumentDef({
            .type=ASSET_TYPE_BIN,
            .size=sizeof(BinDocument),
            .ext=".bin",
            .init_func=InitBinDocument,
            .import_func=ImportBin
        });
    }
}

