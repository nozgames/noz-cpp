//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "../../noz/src/vfx/vfx_internal.h"

constexpr int MAX_EMITTERS_PER_VFX = 32;


struct EditorVfxEmitter
{
    const Name* name;
    VfxEmitterDef def;
};

struct VfxDataImpl {
    VfxFloat duration;
    bool loop;
    EditorVfxEmitter emitters[MAX_EMITTERS_PER_VFX];
    int emitter_count;
    Vfx* vfx;
    VfxHandle handle;
    bool playing;
};

struct VfxData : AssetData {
    VfxDataImpl* impl;
};

extern void InitVfxData(AssetData* ea);
extern VfxData* LoadEditorVfx(const std::filesystem::path& path);
extern Vfx* ToVfx(Allocator* allocator, VfxData* v, const Name* name);
extern void Serialize(VfxData* v, Stream* stream);
extern VfxData* Clone(Allocator* allocator, VfxData* evfx);
extern void DrawEditorVfx(AssetData* ea);
extern AssetData* NewVfxData(const std::filesystem::path& path);
