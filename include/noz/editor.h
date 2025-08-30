//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef NOZ_EDITOR

// @init
bool InitEditorClient(const char* server_address, int port);
void ShutdownEditorClient();

// @client
void UpdateEditorClient();

// @callback
typedef void (*hotload_callback_t)(const char* asset_name);
void SetHotloadCallback(hotload_callback_t callback);

// Forward declaration for Stream
struct Stream;
typedef void (*inspect_ack_callback_t)(Stream* inspector_data);
void SetInspectAckCallback(inspect_ack_callback_t callback);

#endif
