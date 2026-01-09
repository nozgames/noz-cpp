//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

static void ImportAnimation(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(doc);
    assert(doc->type == ASSET_TYPE_ANIMATION);
    AnimationDocument* adoc = static_cast<AnimationDocument*>(doc);
    SkeletonDocument* sdoc = static_cast<SkeletonDocument*>(FindDocument(ASSET_TYPE_SKELETON, adoc->skeleton_name));
    if (!sdoc)
        ThrowError("invalid skeleton");

    Stream* stream = CreateStream(nullptr, 4096);
    Serialize(adoc, stream, sdoc);
    SaveStream(stream, path);
    Free(stream);
}

static bool DoesAnimationDependOn(Document* ea, Document* dependency)
{
    assert(ea);
    assert(ea->type == ASSET_TYPE_ANIMATION);
    assert(dependency);

    if (dependency->type != ASSET_TYPE_SKELETON)
        return false;

#if 0
    AnimationDocument* en = LoadEditorAnimation(ea->path);
    if (!en)
        return false;

    // does the path end with en->skeleton_name + ".skel"?
    std::string expected_path = en->skeleton_name->value;
    std::string path_str = dependency_path.string();
    expected_path += ".skel";

    CleanPath(expected_path.data());
    CleanPath(path_str.data());

    bool result = path_str.ends_with(expected_path);

    Free(en);

    return result;
#else
    return false;
#endif
}

DocumentImporter GetAnimationImporter() {
    return {
        .type = ASSET_TYPE_ANIMATION,
        .ext = ".anim",
        .import_func = ImportAnimation,
        .does_depend_on = DoesAnimationDependOn,
    };
}
