//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "platform.h"

constexpr u16 PREFS_VERSION = 4;
constexpr u32 PREFS_SIGNATURE = FourCC('N', 'Z', 'P', 'R');
constexpr i32 PREF_DEFAULT = I32_MIN;

struct Prefs {
    i32* ints;
    i32 max_prefs;
};

static Prefs g_prefs = {};

static void LoadPrefsInternal(const std::filesystem::path& path) {
    for (i32 i = 0; i < g_prefs.max_prefs; ++i)
        g_prefs.ints[i] = PREF_DEFAULT;

    Stream* stream = LoadStream(ALLOCATOR_SCRATCH, path);
    if (!stream)
        return;

    u32 sig = ReadU32(stream);
    if (sig != PREFS_SIGNATURE)
        return;

    u16 version = ReadU16(stream);
    if (PREFS_VERSION > version)
        return;

    u16 int_count = ReadU16(stream);
    for (u16 i = 0; i < int_count; ++i) {
        u32 index = ReadI32(stream);
        i32 value = ReadI32(stream);
        g_prefs.ints[index] = value;
    }
}

void LoadPrefs() {
    PushScratch();
    LoadPrefsInternal(PlatformGetSaveGamePath() / "prefs.dat");
    PopScratch();
}

static void SavePrefsInternal(const std::filesystem::path& path) {
    Stream* stream = CreateStream(ALLOCATOR_SCRATCH, 4096);
    if (!stream)
        return;

    WriteU32(stream, PREFS_SIGNATURE);
    WriteU16(stream, PREFS_VERSION);

    i32 pref_count = 0;
    for (i32 i = 0; i < g_prefs.max_prefs; ++i)
        if (g_prefs.ints[i] != PREF_DEFAULT)
            ++pref_count;

    WriteU16(stream, (u16)pref_count);

    for (i32 i = 0; i < g_prefs.max_prefs; ++i) {
        if (g_prefs.ints[i] == PREF_DEFAULT)
            continue;
        WriteI32(stream, i);
        WriteI32(stream, g_prefs.ints[i]);
    }

    SaveStream(stream, path);
}

void SavePrefs() {
    PushScratch();
    SavePrefsInternal(PlatformGetSaveGamePath() / "prefs.dat");
    PopScratch();
}

void SetPrefInt(i32 id, i32 value) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    g_prefs.ints[id] = value;
}

i32 GetPrefInt(int id, i32 default_value) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    if (g_prefs.ints[id] == PREF_DEFAULT)
        return default_value;
    return g_prefs.ints[id];
}

void ClearPref(int id) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    g_prefs.ints[id] = PREF_DEFAULT;
}

void InitPrefs(const ApplicationTraits& traits) {
    g_prefs = {};
    g_prefs.max_prefs = (i32)traits.max_prefs + PREF_CORE_COUNT;
    g_prefs.ints = static_cast<int*>(Alloc(ALLOCATOR_DEFAULT, sizeof(i32) * g_prefs.max_prefs));

    LoadPrefs();
}

void ShutdownPrefs() {
    SavePrefs();

    Free(g_prefs.ints);
    g_prefs = {};
}