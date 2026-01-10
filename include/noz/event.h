//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz {

    typedef int EventId;

    constexpr EventId MAX_EVENTS = 512;
    constexpr u32 MAX_CORE_EVENTS = 128;

    typedef void (*EventCallback)(EventId event, const void* event_data);

    void Listen(EventId event, EventCallback callback);
    void Unlisten(EventId event, EventCallback callback);
    void Send(EventId event, const void* event_data);
}
