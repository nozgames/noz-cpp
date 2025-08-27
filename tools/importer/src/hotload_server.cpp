// @STL

#include "hotload_server.h"
#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <string>

// @types
static ENetHost* g_server = nullptr;
static std::vector<ENetPeer*> g_clients;

// @init
bool InitHotloadServer(int port)
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to initialize ENet\n";
        return false;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    // Create a server with up to 32 clients and 2 channels
    g_server = enet_host_create(&address, 32, 2, 0, 0);
    if (!g_server)
    {
        std::cerr << "Failed to create ENet server on port " << port << "\n";
        enet_deinitialize();
        return false;
    }

    std::cout << "Hotload server started on port " << port << "\n";
    return true;
}

void ShutdownHotloadServer()
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
void UpdateHotloadServer()
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
                std::cout << "Hotload client connected from " 
                         << (event.peer->address.host & 0xFF) << "."
                         << ((event.peer->address.host >> 8) & 0xFF) << "."
                         << ((event.peer->address.host >> 16) & 0xFF) << "."
                         << ((event.peer->address.host >> 24) & 0xFF) 
                         << ":" << event.peer->address.port << "\n";
                
                g_clients.push_back(event.peer);
                break;
            }
            
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                std::cout << "Hotload client disconnected\n";
                
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
                // We don't expect to receive data from clients for now
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

    // Create packet with the asset name
    ENetPacket* packet = enet_packet_create(asset_name.c_str(), 
                                           asset_name.length() + 1, 
                                           ENET_PACKET_FLAG_RELIABLE);
    if (!packet)
    {
        std::cerr << "Failed to create packet for asset: " << asset_name << "\n";
        return;
    }

    // Send to all connected clients
    for (ENetPeer* client : g_clients)
    {
        enet_peer_send(client, 0, packet);
    }

    // Flush to ensure the packet is sent immediately
    enet_host_flush(g_server);
    
    std::cout << "Broadcasted hotload for asset: " << asset_name << "\n";
}