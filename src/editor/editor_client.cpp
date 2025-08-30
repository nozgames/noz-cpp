//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef NOZ_EDITOR

#include "noz/editor.h"
#include "editor_messages.h"
#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>

// @types
struct EditorClientEvent
{
    EditorEvent event_type;
    std::string data;
};

typedef void (*inspect_ack_callback_t)(Stream* inspector_data);

static ENetHost* g_client = nullptr;
static ENetPeer* g_server = nullptr;
static hotload_callback_t g_callback = nullptr;
static std::atomic<bool> g_initialized = false;
static std::atomic<bool> g_should_stop = false;
static std::thread g_hotload_thread;
static std::queue<EditorClientEvent> g_event_queue;
static inspect_ack_callback_t g_inspect_ack_callback = nullptr;
static std::mutex g_queue_mutex;
static std::condition_variable g_thread_cv;

// @helpers
static void SendInspectAck(ENetPeer* server, const std::string& search_filter)
{
    std::cout << "SendInspectAck called with server: " << server << std::endl;
    
    if (!server)
    {
        std::cout << "Server is null, returning" << std::endl;
        return;
    }
    
    // Create fake inspector data for now
    std::cout << "Creating fake inspector data..." << std::endl;
    PushScratch();
    Stream* fake_data = CreateStream(ALLOCATOR_SCRATCH, 1024);
    WriteString(fake_data, "FakeObject");
    WriteString(fake_data, "position");
    WriteString(fake_data, "vec3");
    WriteFloat(fake_data, 1.0f);
    WriteFloat(fake_data, 2.0f);
    WriteFloat(fake_data, 3.0f);
    WriteString(fake_data, "rotation");
    WriteString(fake_data, "quat");
    WriteFloat(fake_data, 0.0f);
    WriteFloat(fake_data, 0.0f);
    WriteFloat(fake_data, 0.0f);
    WriteFloat(fake_data, 1.0f);
    
    std::cout << "Creating EDITOR_EVENT_INSPECT_ACK message..." << std::endl;
    EditorMessage ack_msg = CreateInspectAckMessage(fake_data);
    std::cout << "ACK message created with event_id: " << ack_msg.event_id << ", data_size: " << ack_msg.data_size << std::endl;
    
    // Serialize and send
    const size_t buffer_size = sizeof(uint32_t) * 2 + ack_msg.data_size;
    uint8_t* buffer = new uint8_t[buffer_size];
    size_t serialized_size = SerializeMessage(ack_msg, buffer, buffer_size);
    std::cout << "Serialized message size: " << serialized_size << " bytes" << std::endl;
    
    if (serialized_size > 0)
    {
        ENetPacket* packet = enet_packet_create(buffer, serialized_size, ENET_PACKET_FLAG_RELIABLE);
        if (packet)
        {
            std::cout << "Sending packet to server..." << std::endl;
            enet_peer_send(server, 0, packet);
            std::cout << "Packet sent successfully" << std::endl;
        }
        else
        {
            std::cout << "Failed to create ENet packet" << std::endl;
        }
    }
    else
    {
        std::cout << "Failed to serialize message" << std::endl;
    }
    
    delete[] buffer;
    FreeMessage(ack_msg);
    PopScratch();
}

// @thread
static void EditorClientThread(const char* server_address, int port)
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
                        // Deserialize editor message
                        EditorMessage editor_msg;
                        if (DeserializeMessage(event.packet->data, event.packet->dataLength, editor_msg))
                        {
                            EditorClientEvent client_event;
                            client_event.event_type = static_cast<EditorEvent>(editor_msg.event_id);
                            
                            switch (editor_msg.event_id)
                            {
                                case EDITOR_EVENT_HOTLOAD:
                                {
                                    std::string asset_name;
                                    if (ParseHotloadMessage(editor_msg, asset_name))
                                    {
                                        client_event.data = asset_name;
                                        std::lock_guard<std::mutex> lock(g_queue_mutex);
                                        g_event_queue.push(client_event);
                                    }
                                    break;
                                }
                                case EDITOR_EVENT_INSPECT:
                                {
                                    // Server is requesting inspection - send back ACK with fake data
                                    std::cout << "Client received EDITOR_EVENT_INSPECT request" << std::endl;
                                    std::string search_filter;
                                    ParseInspectMessage(editor_msg, search_filter);
                                    std::cout << "Parsing inspect message with filter: '" << search_filter << "'" << std::endl;
                                    std::cout << "Calling SendInspectAck..." << std::endl;
                                    SendInspectAck(server, search_filter);
                                    std::cout << "SendInspectAck completed" << std::endl;
                                    break;
                                }
                            }
                            
                            FreeMessage(editor_msg);
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
bool InitEditorClient(const char* server_address, int port)
{
    if (g_initialized.load())
        return true;

    g_should_stop.store(false);
    
    try
    {
        // Start the hotload thread
        g_hotload_thread = std::thread(EditorClientThread, server_address, port);
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

void ShutdownEditorClient()
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
        std::queue<EditorClientEvent> empty;
        std::swap(g_event_queue, empty);
    }
    
    g_callback = nullptr;
    g_initialized.store(false);
    
    std::cout << "Hotload system shutdown\n";
}

// @client
void UpdateEditorClient()
{
    if (!g_initialized.load())
        return;

    // Process all queued editor events
    std::queue<EditorClientEvent> events_to_process;
    
    // Move events from the shared queue to local queue to minimize lock time
    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        events_to_process = g_event_queue;
        std::queue<EditorClientEvent> empty;
        std::swap(g_event_queue, empty);
    }
    
    // Process events on main thread
    while (!events_to_process.empty())
    {
        const EditorClientEvent& event = events_to_process.front();
        
        if (event.event_type == EDITOR_EVENT_HOTLOAD && g_callback)
        {
            g_callback(event.data.c_str());
        }
        
        events_to_process.pop();
    }
}

// @callback
void SetHotloadCallback(hotload_callback_t callback)
{
    g_callback = callback;
}

void SetInspectAckCallback(inspect_ack_callback_t callback)
{
    g_inspect_ack_callback = callback;
}

#endif