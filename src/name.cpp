//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <noz/name.h>
#include <mutex>
#include <map>

struct NameSystem {
    std::mutex mutex;
    std::map<u64, Name>* map;
    Name none = {""};
};

static NameSystem g_name_system;

Name* NAME_NONE;

Name* GetName(const char* value) {
    if (!value || *value == 0)
        return NAME_NONE;

    std::lock_guard lock(g_name_system.mutex);

    auto key = Hash(value);
    auto it = g_name_system.map->find(key);
    if (it != g_name_system.map->end())
        return &it->second;

    assert(static_cast<u32>(strlen(value)) < MAX_NAME_LENGTH - 1);

    Name& name = (*g_name_system.map)[key];
    Copy(name.value, MAX_NAME_LENGTH, value);
    CleanPath(name.value);
    return &name;
}

void InitName(ApplicationTraits*) {
    g_name_system.map = new std::map<u64, Name>();
    NAME_NONE = &g_name_system.none;
}

void ShutdownName() {
    delete g_name_system.map;
    g_name_system.map = nullptr;
}