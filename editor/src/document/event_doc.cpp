//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    static void InitEventDocument(Document* doc);

    static void DrawEventDocument(Document* doc) {
        BindMaterial(g_workspace.editor_mesh_material);
        BindColor(COLOR_WHITE);
        DrawMesh(MESH_ASSET_ICON_EVENT, Translate(doc->position));
    }

    static void LoadEventDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_EVENT);
        EventDocument* edoc = static_cast<EventDocument*>(doc);

        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, doc->path.value);
        Props* props = Props::Load(contents.c_str(), contents.size());
        edoc->id = props->GetInt("event", "id", 0);
    }

    static EventDocument* LoadEventDocument(const std::filesystem::path& path) {
        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
        Tokenizer tk;
        Init(tk, contents.c_str());

        EventDocument* edoc = static_cast<EventDocument*>(CreateDocument(path));
        assert(edoc);
        InitEventDocument(edoc);
        LoadEventDocument(edoc);
        return edoc;
    }

    static void ReloadEventData(Document* a) {
        assert(a);
        assert(a->def->type == ASSET_TYPE_EVENT);
        EventDocument* e = static_cast<EventDocument*>(a);
        LoadEventDocument(e);
    }

    EventDocument* NewEventDocument(const std::filesystem::path& path) {
        constexpr const char* default_event = "\n";

        std::filesystem::path full_path = path.is_relative() ?  std::filesystem::path(g_editor.project_path) / "assets" / "events" / path : path;
        full_path += ".event";

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        WriteCSTR(stream, default_event);
        SaveStream(stream, full_path);
        Free(stream);

        return LoadEventDocument(full_path);
    }

    static void ImportEvent(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        Stream* stream = CreateStream(nullptr, 4096);

        AssetHeader header = {};
        header.type = ASSET_TYPE_EVENT;
        header.version = 0;
        WriteAssetHeader(stream, &header);

        SaveStream(stream, path);
        Free(stream);
    }

    static void InitEventDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_EVENT);
        EventDocument* e = static_cast<EventDocument*>(doc);
        e->editor_only = true;
        e->vtable = {
            .load = LoadEventDocument,
            .reload = ReloadEventData,
            .draw = DrawEventDocument,
        };
    }

    void InitEventDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_EVENT,
            .size = sizeof(EventDocument),
            .ext = ".event",
            .init_func = InitEventDocument,
            .import_func = ImportEvent
        });
    }
}

