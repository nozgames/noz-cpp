//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef _HOTLOAD

#include "noz/hotload.h"
#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>

// @types
struct HotloadEvent
{
    std::string asset_name;
};

static ENetHost* g_client = nullptr;
static ENetPeer* g_server = nullptr;
static hotload_callback_t g_callback = nullptr;
static std::atomic<bool> g_initialized = false;
static std::atomic<bool> g_should_stop = false;
static std::thread g_hotload_thread;
static std::queue<HotloadEvent> g_event_queue;
static std::mutex g_queue_mutex;
static std::condition_variable g_thread_cv;

// @thread
static void HotloadThreadFunction(const char* server_address, int port)
{
    ENetHost* client = nullptr;
    ENetPeer* server = nullptr;
    bool connected = false;
    
    if (enet_initialize() != 0)
    {
        std::cerr << "Hotload thread: Failed to initialize ENet\n";
        return;
    }

    while (!g_should_stop.load())
    {
        if (!connected)
        {
            // Try to connect to server
            client = enet_host_create(nullptr, 1, 2, 0, 0);
            if (client)
            {
                ENetAddress address;
                if (enet_address_set_host(&address, server_address) >= 0)
                {
                    address.port = port;
                    server = enet_host_connect(client, &address, 2, 0);
                    if (server)
                    {
                        // Wait for connection with timeout
                        ENetEvent event;
                        if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
                        {
                            connected = true;
                            std::cout << "Hotload thread: Connected to server at " << server_address << ":" << port << "\n";
                        }
                        else
                        {
                            // Connection failed, clean up and retry later
                            enet_peer_reset(server);
                            server = nullptr;
                        }
                    }
                }
                
                if (!connected)
                {
                    enet_host_destroy(client);
                    client = nullptr;
                }
            }
            
            if (!connected)
            {
                // Wait before retrying connection
                std::unique_lock<std::mutex> lock(g_queue_mutex);
                g_thread_cv.wait_for(lock, std::chrono::milliseconds(500));
                continue;
            }
        }
        
        if (connected && client)
        {
            ENetEvent event;
            while (enet_host_service(client, &event, 100) > 0)
            {
                switch (event.type)
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                    {
                        // Received asset change notification
                        char* asset_name = reinterpret_cast<char*>(event.packet->data);
                        
                        // Ensure null termination
                        size_t data_length = event.packet->dataLength;
                        std::string asset_name_str;
                        if (data_length > 0 && asset_name[data_length - 1] != '\0')
                        {
                            asset_name_str = std::string(asset_name, data_length);
                        }
                        else
                        {
                            asset_name_str = std::string(asset_name);
                        }
                        
                        // Queue the hotload event for the main thread
                        {
                            std::lock_guard<std::mutex> lock(g_queue_mutex);
                            g_event_queue.push({asset_name_str});
                        }
                        
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    
                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        std::cout << "Hotload thread: Disconnected from server\n";
                        connected = false;
                        server = nullptr;
                        break;
                    }
                }
            }
        }
    }
    
    // Cleanup
    if (server)
    {
        enet_peer_disconnect(server, 0);
        
        // Wait for disconnection to complete
        ENetEvent event;
        while (enet_host_service(client, &event, 3000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
        }
    }
    
    if (client)
    {
        enet_host_destroy(client);
    }
    
    enet_deinitialize();
    std::cout << "Hotload thread: Shutdown complete\n";
}

// @init
bool InitHotload(const char* server_address, int port)
{
    if (g_initialized.load())
        return true;

    g_should_stop.store(false);
    
    try
    {
        // Start the hotload thread
        g_hotload_thread = std::thread(HotloadThreadFunction, server_address, port);
        g_initialized.store(true);
        
        std::cout << "Hotload system initialized (running on background thread)\n";
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to start hotload thread: " << e.what() << "\n";
        return false;
    }
}

void ShutdownHotload()
{
    if (!g_initialized.load())
        return;

    // Signal thread to stop
    g_should_stop.store(true);
    g_thread_cv.notify_all();
    
    // Wait for thread to finish
    if (g_hotload_thread.joinable())
    {
        g_hotload_thread.join();
    }
    
    // Clear the event queue
    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        std::queue<HotloadEvent> empty;
        std::swap(g_event_queue, empty);
    }
    
    g_callback = nullptr;
    g_initialized.store(false);
    
    std::cout << "Hotload system shutdown\n";
}

// @client
void UpdateHotload()
{
    if (!g_initialized.load())
        return;

    // Process all queued hotload events
    std::queue<HotloadEvent> events_to_process;
    
    // Move events from the shared queue to local queue to minimize lock time
    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        events_to_process = g_event_queue;
        std::queue<HotloadEvent> empty;
        std::swap(g_event_queue, empty);
    }
    
    // Process events on main thread
    while (!events_to_process.empty())
    {
        const HotloadEvent& event = events_to_process.front();
        
        if (g_callback)
        {
            g_callback(event.asset_name.c_str());
        }
        
        events_to_process.pop();
    }
}

// @callback
void SetHotloadCallback(hotload_callback_t callback)
{
    g_callback = callback;
}

#endif