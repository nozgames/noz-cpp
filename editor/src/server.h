//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include <string>

// @broadcast
void BroadcastAssetChange(const Name* name, AssetType asset_type);

// @connection
bool HasConnectedClient();