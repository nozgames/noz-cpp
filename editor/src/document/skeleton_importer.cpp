//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

static void ImportSkeleton(Document* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_SKELETON);
    SkeletonDocument* s = (SkeletonDocument*)a;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
    Serialize(s, stream);
    SaveStream(stream, path);
    Free(stream);
}

DocumentImporter GetSkeletonImporter()
{
    return {
        .type = ASSET_TYPE_SKELETON,
        .ext = ".skel",
        .import_func = ImportSkeleton
    };
}

