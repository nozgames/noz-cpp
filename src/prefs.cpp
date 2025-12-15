//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "platform.h"
#include <noz/tokenizer.h>

constexpr u16 PREFS_VERSION = 4;

struct Pref {
    Text string_value;
    int int_value;
    bool is_default;
};

struct Prefs {
    Pref* values;
    i32 max_prefs;
};

static Prefs g_prefs = {};

static void LoadPrefsInternal(const std::filesystem::path& path) {
    Stream* stream = LoadStream(ALLOCATOR_SCRATCH, path);
    if (!stream)
        return;

    Tokenizer tk;
    Init(tk, stream);

    while (!IsEOF(tk)) {
        int pref_index = 0;
        if (ExpectIdentifier(tk, "v")) {
            ExpectInt(tk);
        } else if (ExpectInt(tk, &pref_index)) {
            int pref_int_value = 0;
            if (ExpectInt(tk, &pref_int_value)) {
                if (pref_index >= 0 && pref_index < g_prefs.max_prefs)
                    g_prefs.values[pref_index] = {
                        .string_value = {},
                        .int_value = pref_int_value,
                        .is_default = false
                    };
            } else if (ExpectQuotedString(tk)) {
                if (pref_index >= 0 && pref_index < g_prefs.max_prefs) {
                    g_prefs.values[pref_index] = {
                        .string_value = {},
                        .int_value = pref_int_value,
                        .is_default = false
                    };
                    SetValue(g_prefs.values[pref_index].string_value, tk.current_token.raw, tk.current_token.length);
                }
            }
        } else {
            break;
        }
    }
}

void LoadPrefs() {
    for (int i=0; i<g_prefs.max_prefs; i++) {
        g_prefs.values[i] = {
            .string_value = {},
            .int_value = 0,
            .is_default = true
        };
    }

    PushScratch();
    LoadPrefsInternal(PlatformGetSaveGamePath() / "prefs.dat");
    PopScratch();
}

static void SavePrefsInternal(const std::filesystem::path& path) {
    Stream* stream = CreateStream(ALLOCATOR_SCRATCH, 4096);
    if (!stream)
        return;

    WriteCSTR(stream, "v %d\n", PREFS_VERSION);


    for (int i=0; i<g_prefs.max_prefs; i++) {
        const Pref& pref = g_prefs.values[i];
        if (pref.is_default) continue;

        if (!IsEmpty(pref.string_value)) {
            WriteCSTR(stream, "%d \"%s\"\n", i, pref.string_value.value);
        } else if (pref.int_value != 0) {
            WriteCSTR(stream, "%d %d\n", i, pref.int_value);
        }
    }

    SaveStream(stream, path);
}

void SavePrefs() {
    PushScratch();
    SavePrefsInternal(PlatformGetSaveGamePath() / "prefs.dat");
    PopScratch();
}

void SetIntPref(i32 id, i32 value) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    g_prefs.values[id] = {
        .string_value = {},
        .int_value = value,
        .is_default = false
    };
}

void SetStringPref(i32 id, const char* value) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    g_prefs.values[id] = {
        .string_value = {},
        .int_value = 0,
        .is_default = false
    };
    Text& text = g_prefs.values[id].string_value;
    SetValue(text, value);
}

i32 GetIntPref(int id, i32 default_value) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    if (g_prefs.values[id].is_default)
        return default_value;
    return g_prefs.values[id].int_value;
}

const char* GetStringPref(int id, const char* default_value) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    if (g_prefs.values[id].is_default)
        return default_value;
    return g_prefs.values[id].string_value.value;
}

void ClearPref(int id) {
    id += PREF_CORE_COUNT;
    assert(id >= 0 && id < g_prefs.max_prefs);
    g_prefs.values[id] = {
        .string_value = {},
        .int_value = 0,
        .is_default = true
    };
}

void InitPrefs(const ApplicationTraits& traits) {
    g_prefs = {};
    g_prefs.max_prefs = static_cast<i32>(traits.max_prefs) + PREF_CORE_COUNT;
    g_prefs.values = static_cast<Pref*>(Alloc(ALLOCATOR_DEFAULT, sizeof(Pref) * g_prefs.max_prefs));
    LoadPrefs();
}

void ShutdownPrefs() {
    SavePrefs();

    Free(g_prefs.values);
    g_prefs = {};
}