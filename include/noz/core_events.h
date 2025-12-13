//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr EventId EVENT_FOCUS_CHANGED = -1;
constexpr EventId EVENT_HOTLOAD = -2;
constexpr EventId EVENT_GAMEPAD_ACTIVATED = -3;
constexpr EventId EVENT_GAMEPAD_DEACTIVATED = -4;

struct FocusChangedEvent {
    bool has_focus;
};

struct AssetLoadedEvent {
    const Name* name;
    AssetType type;
};
