//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    extern void ImportSound(Document* doc, const std::filesystem::path& path, Props* config, Props* meta);

    static void CloneSoundDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_SOUND);
        SoundDocument* sdoc = static_cast<SoundDocument*>(doc);
        sdoc->handle = {};
    }

    static void DestroySoundDocument(Document* doc) {
        SoundDocument* s = static_cast<SoundDocument*>(doc);
        Free(s->sound);
        s->sound = nullptr;
    }

    static void DrawSoundDocument(Document* a) {
        BindMaterial(g_workspace.editor_mesh_material);
        BindColor(COLOR_WHITE);
        //DrawMesh(MESH_ASSET_ICON_SOUND, Translate(a->position));
    }

    static void PlaySoundDocument(Document* doc) {
        SoundDocument* sdoc = static_cast<SoundDocument*>(doc);
        if (!sdoc->sound)
            sdoc->sound = static_cast<Sound*>(LoadAssetInternal(ALLOCATOR_DEFAULT, sdoc->name, ASSET_TYPE_SOUND, LoadSound));

        Play(sdoc->sound, 1.0f, 1.0f);
    }

    static void InitSoundDocument(SoundDocument* sdoc) {
        sdoc->vtable = {
            .destructor = DestroySoundDocument,
            .draw = DrawSoundDocument,
            .play = PlaySoundDocument,
            .clone = CloneSoundDocument,
        };
    }

    static void InitSoundDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_SOUND);
        InitSoundDocument(static_cast<SoundDocument*>(doc));
    }

    void InitSoundDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_SOUND,
            .size = sizeof(SoundDocument),
            .ext = ".wav",
            .init_func = InitSoundDocument,
            .import_func = ImportSound
        });
    }
}
