//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef NOZ_EDITOR

void BeginInspectorObject(Stream* stream, type_t type, const char* name);
void WriteInspectorProperty(Stream* stream, const char* name, const char* value);
void WriteInspectorProperty(Stream* stream, const char* name, float value);
void WriteInspectorProperty(Stream* stream, const char* name, bool value);
void WriteInspectorProperty(Stream* stream, const char* name, const vec3& value);
void EndInspectorObject(Stream* stream);

#endif
