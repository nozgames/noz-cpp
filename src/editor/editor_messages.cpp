//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef NOZ_EDITOR

#include "editor_messages.h"
#include "noz/noz.h"
#include <cstring>

void WriteEditorMessage(Stream* stream, EditorMessage event)
{
    WriteU8(stream, (u8)event);
}

EditorMessage ReadEditorMessage(Stream* stream)
{
    return (EditorMessage)ReadU8(stream);
}

#if 0
EditorMessage CreateHotloadMessage(const std::string& asset_name)
{
    EditorMessage msg{};
    msg.event_id = EDITOR_MESSAGE_HOTLOAD;
    msg.data_size = static_cast<uint32_t>(asset_name.length() + 1); // +1 for null terminator
    msg.data = new uint8_t[msg.data_size];
    std::memcpy(msg.data, asset_name.c_str(), msg.data_size);
    return msg;
}

EditorMessage CreateInspectMessage(const std::string& search_filter)
{
    EditorMessage msg{};
    msg.event_id = EDITOR_MESSAGE_INSPECT;
    msg.data_size = static_cast<uint32_t>(search_filter.length() + 1); // +1 for null terminator
    msg.data = new uint8_t[msg.data_size];
    std::memcpy(msg.data, search_filter.c_str(), msg.data_size);
    return msg;
}

EditorMessage CreateInspectAckMessage(Stream* inspector_data)
{
    EditorMessage msg{};
    msg.event_id = EDITOR_MESSAGE_INSPECT_ACK;
    msg.data_size = static_cast<uint32_t>(GetSize(inspector_data));
    msg.data = new uint8_t[msg.data_size];
    std::memcpy(msg.data, GetData(inspector_data), msg.data_size);
    return msg;
}

bool ParseHotloadMessage(const EditorMessage& msg, std::string& asset_name)
{
    if (msg.event_id != EDITOR_MESSAGE_HOTLOAD || msg.data_size == 0)
        return false;
    
    asset_name = std::string(reinterpret_cast<const char*>(msg.data), msg.data_size - 1);
    return true;
}

bool ParseInspectMessage(const EditorMessage& msg, std::string& search_filter)
{
    if (msg.event_id != EDITOR_MESSAGE_INSPECT || msg.data_size == 0)
        return false;
    
    search_filter = std::string(reinterpret_cast<const char*>(msg.data), msg.data_size - 1);
    return true;
}

size_t SerializeMessage(const EditorMessage& msg, uint8_t* buffer, size_t buffer_size)
{
    const size_t header_size = sizeof(uint32_t) * 2; // event_id + data_size
    const size_t total_size = header_size + msg.data_size;
    
    if (buffer_size < total_size)
        return 0;
    
    // Write header
    std::memcpy(buffer, &msg.event_id, sizeof(uint32_t));
    std::memcpy(buffer + sizeof(uint32_t), &msg.data_size, sizeof(uint32_t));
    
    // Write data
    if (msg.data_size > 0 && msg.data)
    {
        std::memcpy(buffer + header_size, msg.data, msg.data_size);
    }
    
    return total_size;
}

bool DeserializeMessage(const uint8_t* buffer, size_t buffer_size, EditorMessage& msg)
{
    const size_t header_size = sizeof(uint32_t) * 2;
    if (buffer_size < header_size)
        return false;
    
    // Read header
    std::memcpy(&msg.event_id, buffer, sizeof(uint32_t));
    std::memcpy(&msg.data_size, buffer + sizeof(uint32_t), sizeof(uint32_t));
    
    // Validate data size
    if (buffer_size < header_size + msg.data_size)
        return false;
    
    // Copy data
    if (msg.data_size > 0)
    {
        msg.data = new uint8_t[msg.data_size];
        std::memcpy(msg.data, buffer + header_size, msg.data_size);
    }
    else
    {
        msg.data = nullptr;
    }
    
    return true;
}

void FreeMessage(EditorMessage& msg)
{
    if (msg.data)
    {
        delete[] msg.data;
        msg.data = nullptr;
    }
    msg.data_size = 0;
}
#endif
#endif