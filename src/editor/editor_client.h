//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef NOZ_EDITOR

typedef void (*HotloadCallbackFunc)(const char* asset_name);

void InitEditorClient(const char* host, int port);
void ShutdownEditorClient();
void UpdateEditorClient();
void SetHotloadCallback(HotloadCallbackFunc func);

#endif
