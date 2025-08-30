//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <string>

// @init
bool InitEditorServer(int port);
void ShutdownEditorServer();

// @server
void UpdateEditorServer();

// @broadcast
void BroadcastAssetChange(const std::string& asset_name);
void SendInspectRequest(const std::string& search_filter = "");

// @connection
bool HasConnectedClients();