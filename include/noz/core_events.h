//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

const EventId EVENT_FOCUS_CHANGED = -1;
const EventId EVENT_HOTLOAD = -2;

struct FocusChangedEvent
{
    bool has_focus;
};

struct HotloadEvent
{
    const char* asset_name;
};
