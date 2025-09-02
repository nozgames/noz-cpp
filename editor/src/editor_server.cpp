// @STL

// Isolate ENet from Windows GUI conflicts
#ifdef _WIN32
#undef NOUSER
#undef NOGDI
#endif

#include <enet/enet.h>

// Re-apply restrictions after ENet
#ifdef _WIN32
#define NOGDI  
#define NOUSER
#endif

#include "server.h"
#include "../../src/editor/editor_messages.h"
#include "views/inspector_object.h"

void HandleInspectorObject(std::unique_ptr<InspectorObject> object);

// @types
static ENetHost* g_server = nullptr;
static ENetPeer* g_client = nullptr;

static Stream* CreateEditorMessage(EditorMessage event)
{
    Stream* output_stream = CreateStream(ALLOCATOR_SCRATCH, 1024);
    WriteEditorMessage(output_stream, event);
    return output_stream;
}

static void SendEditorMessage(Stream* stream)
{
    if (ENetPacket* packet = enet_packet_create(GetData(stream), GetSize(stream), ENET_PACKET_FLAG_RELIABLE))
    {
        enet_peer_send(g_client, 0, packet);
        enet_host_flush(g_server);
    }

    Free(stream);
}

bool HasConnectedClient()
{
    return g_client != nullptr;
}

static void HandleInspectAck(Stream* stream)
{
    auto inspectorObject = InspectorObject::CreateFromStream(stream);
    if (!inspectorObject)
        return;

    HandleInspectorObject(std::move(inspectorObject));
}

static void HandleStatsAck(Stream* stream)
{
    i32 fps = ReadI32(stream);
    EditorEventStats stats{
        .fps = fps
    };
    Send(EDITOR_EVENT_STATS, &stats);
}

static void HandleClientMessage(void* data, size_t data_size)
{
    auto stream = LoadStream(ALLOCATOR_DEFAULT, (u8*)data, data_size);
    switch (ReadEditorMessage(stream))
    {
    case EDITOR_MESSAGE_INSPECT_ACK:
        HandleInspectAck(stream);
        break;

    case EDITOR_MESSAGE_STATS_ACK:
        HandleStatsAck(stream);
        break;

    default:
        break;
    }
    Free(stream);
}


// @server
void UpdateEditorServer()
{
    if (!g_server)
        return;

    ENetEvent event;
    while (enet_host_service(g_server, &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                if (g_client != nullptr)
                {
                    LogError("editor client already connected");
                    enet_peer_disconnect_now(event.peer, 0);
                    return;
                }

                LogInfo("Editor client connected from %d.%d.%d.%d:%d",
                       (event.peer->address.host & 0xFF),
                       ((event.peer->address.host >> 8) & 0xFF),
                       ((event.peer->address.host >> 16) & 0xFF),
                       ((event.peer->address.host >> 24) & 0xFF),
                       event.peer->address.port);

                g_client = event.peer;
                break;
            }
            
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                if (g_client != event.peer)
                    return;

                LogInfo("Editor client disconnected");
                g_client = nullptr;
                break;
            }
            
            case ENET_EVENT_TYPE_RECEIVE:
                HandleClientMessage(event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
        }
    }
}

// @broadcast
void BroadcastAssetChange(const std::string& asset_name)
{

#if 0
    if (!g_server || g_clients.empty())
        return;
    EditorMessage hotload_msg = CreateHotloadMessage(asset_name);
    
    const size_t buffer_size = sizeof(uint32_t) * 2 + hotload_msg.data_size;
    uint8_t* buffer = new uint8_t[buffer_size];
    size_t serialized_size = SerializeMessage(hotload_msg, buffer, buffer_size);
    
    if (serialized_size > 0)
    {
        ENetPacket* packet = enet_packet_create(buffer, serialized_size, ENET_PACKET_FLAG_RELIABLE);
        if (packet)
        {
            // Send to all connected clients
            for (ENetPeer* client : g_clients)
            {
                enet_peer_send(client, 0, packet);
            }
            
            // Flush to ensure the packet is sent immediately
            enet_host_flush(g_server);
            
            LogInfo("Broadcasted hotload for asset: %s", asset_name.c_str());
        }
    }
    
    delete[] buffer;
    FreeMessage(hotload_msg);
#endif
}

void SendInspectRequest(const std::string& search_filter)
{
    auto msg = CreateEditorMessage(EDITOR_MESSAGE_INSPECT);
    WriteString(msg, search_filter.c_str());
    SendEditorMessage(msg);
}

void RequestStats()
{
    SendEditorMessage(CreateEditorMessage(EDITOR_MESSAGE_STATS));
}

// @init
bool InitEditorServer(int port)
{
    if (enet_initialize() != 0)
    {
        LogWarning("Failed to initialize ENet");
        return false;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    // Create a server with up to 32 clients and 2 channels
    g_server = enet_host_create(&address, 32, 2, 0, 0);
    if (!g_server)
    {
        LogWarning("Failed to create ENet server on port %d", port);
        enet_deinitialize();
        return false;
    }

    LogInfo("Editor server started on port %d", port);
    return true;
}

void ShutdownEditorServer()
{
    if (g_server)
    {
        enet_host_destroy(g_server);
        g_server = nullptr;
    }

    g_client = nullptr;
    enet_deinitialize();
}
