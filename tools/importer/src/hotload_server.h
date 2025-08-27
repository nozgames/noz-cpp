//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <string>

// @init
bool InitHotloadServer(int port);
void ShutdownHotloadServer();

// @server
void UpdateHotloadServer();

// @broadcast
void BroadcastAssetChange(const std::string& asset_name);