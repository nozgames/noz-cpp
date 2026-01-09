//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "../../noz/src/vfx/vfx_internal.h"

namespace noz::editor {
    constexpr int MAX_EMITTERS_PER_VFX = 32;

    struct EditorVfxEmitter {
        const Name* name;
        VfxEmitterDef def;
    };

    struct VfxDocument : Document {
        VfxFloat duration;
        bool loop;
        EditorVfxEmitter emitters[MAX_EMITTERS_PER_VFX];
        int emitter_count;
        Vfx* vfx;
        VfxHandle handle;
        bool playing;
    };

    extern void InitVfxDocument(Document* doc);
    extern VfxDocument* LoadVfxDocument(const std::filesystem::path& path);
    extern VfxDocument* NewVfxDocument(const std::filesystem::path& path);
    extern VfxDocument* Clone(VfxDocument* evfx);
    extern Vfx* ToVfx(Allocator* allocator, VfxDocument* v, const Name* name);
    extern void Serialize(VfxDocument* v, Stream* stream);
    extern void DrawEditorVfx(Document* doc);
}
