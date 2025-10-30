//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include <xxhash.h>

u64 Hash(const void* data, size_t size) {
    return XXH64(data, size, 0);
}

u64 Hash(const char* str) {
    if (!str) return 0;
    return XXH64(str, strlen(str), 0);
}

u64 Hash(void* data, size_t size, u64 seed) {
    return XXH64(data, size, seed);
}

u64 Hash(const text_t& text) {
    return XXH64(text.value, text.length, 0);
}

u64 HashFile(const std::filesystem::path& path) {
    FILE* file = fopen(path.string().c_str(), "rb");
    if (!file)
        return 0;

    XXH64_state_t* state = XXH64_createState();
    if (!state) {
        fclose(file);
        return 0;
    }

    XXH64_reset(state, 0);

    constexpr size_t BUFFER_SIZE = 64 * 1024; // 64KB chunks
    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        XXH64_update(state, buffer, bytesRead);
    }

    u64 hash = XXH64_digest(state);

    XXH64_freeState(state);
    fclose(file);

    return hash;
}


