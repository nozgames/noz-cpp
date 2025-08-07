#pragma once

namespace std 
{
    template <>
    struct hash<std::pair<int32_t, int32_t>> {
        std::size_t operator()(const std::pair<int32_t, int32_t>& k) const noexcept {
            return std::hash<int32_t>()(k.first) ^ (std::hash<int32_t>()(k.second) << 1);
        }
    };
} 