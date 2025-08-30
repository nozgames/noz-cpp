// @STL

#include "server.h"
#include "../../src/editor/editor_messages.h"
#include <noz/log.h>
#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <string>

// Forward declaration for editor functions
extern void ForwardInspectionData(Stream* inspector_data);

// @types
static ENetHost* g_server = nullptr;
static std::vector<ENetPeer*> g_clients;

// Function to check if any clients are connected
bool HasConnectedClients()
{
    return !g_clients.empty();
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
    
    g_clients.clear();
    enet_deinitialize();
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
                LogInfo("Editor client connected from %d.%d.%d.%d:%d",
                       (event.peer->address.host & 0xFF),
                       ((event.peer->address.host >> 8) & 0xFF),
                       ((event.peer->address.host >> 16) & 0xFF),
                       ((event.peer->address.host >> 24) & 0xFF),
                       event.peer->address.port);
                
                g_clients.push_back(event.peer);
                break;
            }
            
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                LogInfo("Editor client disconnected");
                
                // Remove client from the list
                auto it = std::find(g_clients.begin(), g_clients.end(), event.peer);
                if (it != g_clients.end())
                {
                    g_clients.erase(it);
                }
                break;
            }
            
            case ENET_EVENT_TYPE_RECEIVE:
            {
                // Handle editor messages from clients
                EditorMessage editor_msg;
                if (DeserializeMessage(event.packet->data, event.packet->dataLength, editor_msg))
                {
                    switch (editor_msg.event_id)
                    {
                        case EDITOR_EVENT_INSPECT_ACK:
                        {
                            // Received inspection data from client
                            LogInfo("Received EDITOR_EVENT_INSPECT_ACK message from client");
                            LogInfo("Message data size: %d bytes", editor_msg.data_size);
                            
                            // Parse the inspection data and forward to inspector
                            Stream* inspector_data;
                            if (ParseInspectAckMessage(editor_msg, &inspector_data))
                            {
                                LogInfo("Successfully parsed inspection data, forwarding to inspector");
                                ForwardInspectionData(inspector_data);
                            }
                            else
                            {
                                LogWarning("Failed to parse inspection data from client");
                            }
                            break;
                        }
                    }
                    
                    FreeMessage(editor_msg);
                }
                enet_packet_destroy(event.packet);
                break;
            }
        }
    }
}

// @broadcast
void BroadcastAssetChange(const std::string& asset_name)
{
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
}

void SendInspectRequest(const std::string& search_filter)
{
    if (!g_server || g_clients.empty())
        return;

    EditorMessage inspect_msg = CreateInspectMessage(search_filter);
    
    const size_t buffer_size = sizeof(uint32_t) * 2 + inspect_msg.data_size;
    uint8_t* buffer = new uint8_t[buffer_size];
    size_t serialized_size = SerializeMessage(inspect_msg, buffer, buffer_size);
    
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
            
            enet_host_flush(g_server);
            
            LogInfo("Sent inspect request with filter: %s", search_filter.c_str());
        }
    }
    
    delete[] buffer;
    FreeMessage(inspect_msg);
}