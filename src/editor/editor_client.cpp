//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef NOZ_EDITOR

#include "editor_messages.h"
#include "editor_client.h"
#include <enet/enet.h>

typedef void (*inspect_ack_callback_t)(Stream* inspector_data);

struct EditorClient
{
    ENetAddress address;
    ENetHost* host;
    ENetPeer* peer;
    bool connected;
};

static EditorClient g_client = {};
static inspect_ack_callback_t g_inspect_ack_callback = nullptr;

Stream* CreateEditorMessage(EditorMessage event)
{
    Stream* output_stream = CreateStream(ALLOCATOR_SCRATCH, 1024);
    WriteEditorMessage(output_stream, event);
    return output_stream;
}

void SendEditorMessage(Stream* stream)
{
    if (ENetPacket* packet = enet_packet_create(GetData(stream), GetSize(stream), ENET_PACKET_FLAG_RELIABLE))
        enet_peer_send(g_client.peer, 0, packet);

    Free(stream);
}

static void HandleHotload(Stream* input_stream)
{
    assert(input_stream);
    AssetType asset_type = (AssetType)ReadU8(input_stream);
    const Name* name = ReadName(input_stream);
    const AssetLoadedEvent event = { .name = name, .type = asset_type };
    Send(EVENT_HOTLOAD, &event);
}

static void HandleStats(Stream* input_stream)
{
    assert(input_stream);
    auto stream = CreateEditorMessage(EDITOR_MESSAGE_STATS_ACK);
    WriteI32(stream, (i32)GetCurrentFPS());
    SendEditorMessage(stream);
}

void SetInspectAckCallback(inspect_ack_callback_t callback)
{
    g_inspect_ack_callback = callback;
}

void BeginInspectorObject(Stream* stream, const char* type, const char* name)
{
    WriteU8(stream, INSPECTOR_OBJECT_COMMAND_BEGIN);
    WriteString(stream, type);
    WriteString(stream, name);
}

static void WriteInspectorProperty(Stream* stream, const char* name, InspectorObjectCommand value_type)
{
    WriteU8(stream, INSPECTOR_OBJECT_COMMAND_PROPERTY);
    WriteString(stream, name);
    WriteU8(stream, (u8)value_type);
}

void WriteInspectorProperty(Stream* stream, const char* name, const char* value)
{
    WriteInspectorProperty(stream, name, INSPECTOR_OBJECT_COMMAND_STRING);
    WriteString(stream, value);
}

void WriteInspectorProperty(Stream* stream, const char* name, bool value)
{
    WriteInspectorProperty(stream, name, INSPECTOR_OBJECT_COMMAND_BOOL);
    WriteBool(stream, value);
}

void WriteInspectorProperty(Stream* stream, const char* name, float value)
{
    WriteInspectorProperty(stream, name, INSPECTOR_OBJECT_COMMAND_FLOAT);
    WriteFloat(stream, value);
}

void WriteInspectorProperty(Stream* stream, const char* name, const Vec3& value)
{
    WriteInspectorProperty(stream, name, INSPECTOR_OBJECT_COMMAND_VEC3);
    WriteVec3(stream, value);
}

void WriteInspectorProperty(Stream* stream, const char* name, const Rect& value)
{
    WriteInspectorProperty(stream, name, INSPECTOR_OBJECT_COMMAND_RECT);
    WriteRect(stream, value);
}

void EndInspectorObject(Stream* stream)
{
    WriteU8(stream, INSPECTOR_OBJECT_COMMAND_END);
}

static void HandleEditorMessage(void* data, u32 data_size)
{
    PushScratch();
    Stream* stream = LoadStream(ALLOCATOR_SCRATCH, (u8*)data, data_size);
    switch (ReadEditorMessage(stream))
    {
    case EDITOR_MESSAGE_HOTLOAD:
        HandleHotload(stream);
        break;

    case EDITOR_MESSAGE_INSPECT:
        break;

    case EDITOR_MESSAGE_STATS:
        HandleStats(stream);
        break;

    default:
        break;
    }
    Free(stream);
    PopScratch();
}

// @update
void UpdateEditorClient()
{
    if (!g_client.host)
        return;

    ENetEvent event;
    while (enet_host_service(g_client.host, &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                g_client.connected = true;
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                HandleEditorMessage(event.packet->data, (u32)event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                g_client.connected = false;
                g_client.peer = enet_host_connect(g_client.host, &g_client.address, 2, 0);
                break;
            }

            default:
                break;
        }
    }
}

// @init
void InitEditorClient(const char* server_address, int port)
{
    enet_initialize();

    if (enet_address_set_host(&g_client.address, server_address) < 0)
        return;

    g_client.address.port = (u16)port;
    g_client.host = enet_host_create(nullptr, 1, 2, 0, 0);
    g_client.peer = enet_host_connect(g_client.host, &g_client.address, 2, 0);
}

void ShutdownEditorClient()
{
    if (g_client.peer)
        enet_peer_disconnect(g_client.peer, 0);

    if (g_client.host)
        enet_host_destroy(g_client.host);

    enet_deinitialize();
}

#endif
