//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "platform.h"

constexpr int PREFS_MAX_INTS = 64;
constexpr u16 PREFS_VERSION = 1;

constexpr u32 PREFS_SIGNATURE = FourCC('N', 'Z', 'P', 'R');

struct Prefs {
    u64 ints_keys[PREFS_MAX_INTS];
    i32 ints[PREFS_MAX_INTS];
    Map ints_map;
};

static Prefs g_prefs = {};

static void LoadPrefsInternal(const std::filesystem::path& path)
{
    Stream* stream = LoadStream(ALLOCATOR_SCRATCH, path);
    if (!stream)
        return;

    u32 sig = ReadU32(stream);
    if (sig != PREFS_SIGNATURE)
        return;

    u16 version = ReadU16(stream);
    if (version > PREFS_VERSION)
        return;

    u16 int_count = ReadU16(stream);
    if (int_count > PREFS_MAX_INTS)
        return;

    for (u16 i = 0; i < int_count; ++i)
    {
        u64 key = ReadU64(stream);
        i32 value = ReadI32(stream);
        SetValue(g_prefs.ints_map, key, &value);
    }
}

void LoadPrefs()
{
    g_prefs = {};

    Init(g_prefs.ints_map, g_prefs.ints_keys, g_prefs.ints, PREFS_MAX_INTS, sizeof(i32), 0);

    PushScratch();
    LoadPrefsInternal(platform::GetSaveGamePath() / "prefs.dat");
    PopScratch();
}

static void SavePrefsInternal(const std::filesystem::path& path)
{
    Stream* stream = CreateStream(ALLOCATOR_SCRATCH, 4096);
    if (!stream)
        return;

    WriteU32(stream, PREFS_SIGNATURE);
    WriteU16(stream, PREFS_VERSION);
    WriteU16(stream, (u16)g_prefs.ints_map.count);

    for (u32 i = 0; i < g_prefs.ints_map.count; ++i)
    {
        WriteU64(stream, g_prefs.ints_map.keys[i]);
        WriteI32(stream, g_prefs.ints[i]);
    }

    SaveStream(stream, path);
}

void SavePrefs()
{
    PushScratch();
    SavePrefsInternal(platform::GetSaveGamePath() / "prefs.dat");
    PopScratch();
}

void SetPrefInt(const Name* name, i32 value)
{
    SetValue(g_prefs.ints_map, Hash(name->value), &value);
}

i32 GetPrefInt(const Name* name, i32 default_value)
{
    void* value = GetValue(g_prefs.ints_map, Hash(name->value));
    if (!value)
        return default_value;

    return *(i32*)value;
}
