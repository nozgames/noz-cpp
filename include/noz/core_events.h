//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr EventId EVENT_FOCUS_CHANGED = -1;
constexpr EventId EVENT_HOTLOAD = -2;
constexpr EventId EVENT_TEXTINPUT_CHANGED = -3;

struct FocusChangedEvent
{
    bool has_focus;
};

struct AssetLoadedEvent
{
    const Name* name;
    AssetSignature signature;
};
