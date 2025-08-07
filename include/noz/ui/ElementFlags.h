/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::ui
{
    enum class ElementFlags : uint8_t
    {
        None = 0,
        LayoutDirty = 1 << 0,
        StyleDirty = 1 << 1,
    };
    
    inline ElementFlags operator|(ElementFlags a, ElementFlags b)
    {
        return static_cast<ElementFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    
    inline ElementFlags operator&(ElementFlags a, ElementFlags b)
    {
        return static_cast<ElementFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }
    
    inline ElementFlags operator~(ElementFlags a)
    {
        return static_cast<ElementFlags>(~static_cast<uint32_t>(a));
    }
    
    inline ElementFlags& operator|=(ElementFlags& a, ElementFlags b)
    {
        return a = a | b;
    }
    
    inline ElementFlags& operator&=(ElementFlags& a, ElementFlags b)
    {
        return a = a & b;
    }
    
    inline bool hasFlag(ElementFlags flags, ElementFlags flag)
    {
        return (flags & flag) != ElementFlags::None;
    }
    
    inline bool hasAnyFlag(ElementFlags flags, ElementFlags mask)
    {
        return (flags & mask) != ElementFlags::None;
    }
    
    inline bool hasAllFlags(ElementFlags flags, ElementFlags mask)
    {
        return (flags & mask) == mask;
    }
}
