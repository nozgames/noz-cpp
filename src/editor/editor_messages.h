//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "noz/stream.h"
#include <string>

enum EditorEvent
{
    EDITOR_EVENT_HOTLOAD = 1,
    EDITOR_EVENT_INSPECT = 2,
    EDITOR_EVENT_INSPECT_ACK = 3
};

struct EditorMessage
{
    uint32_t event_id;
    uint32_t data_size;
    uint8_t* data;
};

// Message creation helpers
EditorMessage CreateHotloadMessage(const std::string& asset_name);
EditorMessage CreateInspectMessage(const std::string& search_filter = "");
EditorMessage CreateInspectAckMessage(Stream* inspector_data);

// Message parsing helpers
bool ParseHotloadMessage(const EditorMessage& msg, std::string& asset_name);
bool ParseInspectMessage(const EditorMessage& msg, std::string& search_filter);
bool ParseInspectAckMessage(const EditorMessage& msg, Stream** inspector_data);

// Message serialization/deserialization
size_t SerializeMessage(const EditorMessage& msg, uint8_t* buffer, size_t buffer_size);
bool DeserializeMessage(const uint8_t* buffer, size_t buffer_size, EditorMessage& msg);

// Memory management
void FreeMessage(EditorMessage& msg);