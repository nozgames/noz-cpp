//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef _HOTLOAD

// @init
bool InitHotload(const char* server_address, int port);
void ShutdownHotload();

// @client
void UpdateHotload();

// @callback
typedef void (*hotload_callback_t)(const char* asset_name);
void SetHotloadCallback(hotload_callback_t callback);

#endif
