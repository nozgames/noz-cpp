//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef NOZ_EDITOR

#include "editor_messages.h"
#include "noz/editor.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <enet/enet.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

// @types
struct EditorClientEvent
{
    EditorEvent event_type;
    std::string data;
};

typedef void (*inspect_ack_callback_t)(Stream* inspector_data);

static ENetHost* g_client = nullptr;
static ENetPeer* g_server = nullptr;
static HotloadCallbackFunc g_callback = nullptr;
static std::atomic g_initialized = false;
static std::atomic g_should_stop = false;
static std::thread g_hotload_thread;
static std::queue<EditorClientEvent> g_event_queue;
static inspect_ack_callback_t g_inspect_ack_callback = nullptr;
static std::mutex g_queue_mutex;
static std::condition_variable g_thread_cv;

void WriteInspectorEntity(Stream* stream, Entity* entity);

Stream* CreateEditorMessage(EditorEvent event)
{
    Stream* output_stream = CreateStream(ALLOCATOR_SCRATCH, 1024);
    WriteEditorMessage(output_stream, event);
    return output_stream;
}

void SendEditorMessage(Stream* stream)
{
    if (ENetPacket* packet = enet_packet_create(GetData(stream), GetSize(stream), ENET_PACKET_FLAG_RELIABLE))
        enet_peer_send(g_server, 0, packet);

    Destroy(stream);
}

static void HandleInspect(Stream* input_stream)
{
    assert(input_stream);

    auto stream = CreateEditorMessage(EDITOR_EVENT_INSPECT_ACK);
    WriteInspectorEntity(stream, GetRootEntity());
    SendEditorMessage(stream);
}

static void HandleEditorMessage(ENetPeer* server, void* data, size_t data_size)
{
    PushScratch();
    Stream* stream = LoadStream(ALLOCATOR_SCRATCH, (u8*)data, data_size);
    switch (ReadEditorMessage(stream))
    {
    case EDITOR_EVENT_HOTLOAD:
        break;

    case EDITOR_EVENT_INSPECT:
        HandleInspect(stream);
        break;

    default:
        break;
    }
    Destroy(stream);
    PopScratch();
}

// @thread
static void EditorClientThread(const char* server_address, int port)
{
    ENetHost* client = nullptr;
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
                    g_server = enet_host_connect(client, &address, 2, 0);
                    if (g_server)
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
                            enet_peer_reset(g_server);
                            g_server = nullptr;
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
                        HandleEditorMessage(g_server, event.packet->data, event.packet->dataLength);
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    
                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        std::cout << "Hotload thread: Disconnected from server\n";
                        connected = false;
                        g_server = nullptr;
                        break;
                    }
                }
            }
        }
    }
    
    // Cleanup
    if (g_server)
    {
        enet_peer_disconnect(g_server, 0);
        
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
void InitEditorClient(const char* server_address, int port)
{
    if (g_initialized.load())
        return;

    g_should_stop.store(false);
    
    try
    {
        // Start the hotload thread
        g_hotload_thread = std::thread(EditorClientThread, server_address, port);
        g_initialized.store(true);
        
        std::cout << "Hotload system initialized (running on background thread)\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to start hotload thread: " << e.what() << "\n";
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
        std::lock_guard lock(g_queue_mutex);
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
void SetHotloadCallback(HotloadCallbackFunc callback)
{
    g_callback = callback;
}

void SetInspectAckCallback(inspect_ack_callback_t callback)
{
    g_inspect_ack_callback = callback;
}

void BeginInspectorObject(Stream* stream, type_t type, const char* name)
{
    WriteU8(stream, INSPECTOR_OBJECT_COMMAND_BEGIN);
    WriteString(stream, GetTypeName(type));
    WriteString(stream, name);
}

static void WriteInspectorProperty(Stream* stream, const char* name, InspectorObjectCommand value_type)
{
    WriteU8(stream, INSPECTOR_OBJECT_COMMAND_PROPERTY);
    WriteString(stream, name);
    WriteU8(stream, value_type);
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

void WriteInspectorProperty(Stream* stream, const char* name, const vec3& value)
{
    WriteInspectorProperty(stream, name, INSPECTOR_OBJECT_COMMAND_VEC3);
    WriteVec3(stream, value);
}

void EndInspectorObject(Stream* stream)
{
    WriteU8(stream, INSPECTOR_OBJECT_COMMAND_END);
}

#endif
