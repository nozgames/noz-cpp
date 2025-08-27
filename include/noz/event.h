//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once


typedef int event_t;

constexpr event_t EVENT_TEST = -1;

constexpr event_t MAX_CORE_EVENTS = 128;

typedef void (*EventCallback)(event_t event, void* event_data);


void Listen(event_t event, EventCallback callback);
void Unlisten(event_t event, EventCallback callback);
void Send(event_t event, void* event_data);
