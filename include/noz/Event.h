/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "EventManager.h"
#include "events/EventListener.h"
#include "events/EventRegistry.h"

namespace noz
{
	class Object;
	
	class Event
	{
	public:

		template<typename EventType, typename ListenerType>
		static EventListenerHandle listen(std::weak_ptr<ListenerType> listener)
		{
			return EventRegistry<EventType>::addListener(listener);
		}

		template<typename EventType, typename ListenerType>
		static EventListenerHandle listen(ListenerType* listener)
		{
			return EventRegistry<EventType>::addListener(std::weak_ptr<ListenerType>(listener->as<ListenerType>()));
		}

		// Listen to events from specific sender - calls handle() method on the listener object
		template<typename EventType, typename ListenerType>
		static EventListenerHandle listen(std::weak_ptr<ListenerType> listener, std::weak_ptr<Object> sender)
		{
			return EventRegistry<EventType>::addListener(listener, sender);
		}

		template<typename EventType, typename ListenerType>
		static EventListenerHandle listen(ListenerType* listener, std::weak_ptr<Object> sender)
		{
			return EventRegistry<EventType>::addListener(std::weak_ptr<ListenerType>(listener->as<ListenerType>()), sender);
		}

		// Scoped listening - calls handle() method on the listener object
		template<typename EventType, typename ListenerType>
		static ScopedEventListener scopedListen(std::weak_ptr<ListenerType> listener)
		{
			return ScopedEventListener(listen<EventType>(listener));
		}

		template<typename EventType, typename ListenerType>
		static ScopedEventListener scopedListen(ListenerType* listener, std::weak_ptr<Object> sender)
		{
			return ScopedEventListener(listen<EventType>(std::weak_ptr<ListenerType>(listener->as<ListenerType>()), sender));
		}

		// Scoped listening with sender filter - calls handle() method on the listener object
		template<typename EventType, typename ListenerType>
		static ScopedEventListener scopedListen(std::weak_ptr<ListenerType> listener, std::weak_ptr<Object> sender)
		{
			return ScopedEventListener(listen<EventType>(listener, sender));
		}

		template<typename EventType>
		static bool send(std::weak_ptr<Object> sender, const EventType& event)
		{
			return EventRegistry<EventType>::sendEvent(event, sender);
		}

		template<typename EventType>
		static void send(Object* sender, const EventType& event)
		{
			EventRegistry<EventType>::sendEvent(event, std::weak_ptr<Object>(sender->as<Object>()));
		}

		template<typename EventType>
		static void send(const EventType& event)
		{
			EventRegistry<EventType>::sendEvent(event, std::weak_ptr<Object>());
		}

		template<typename EventType>
		static void queue(Object* sender, EventType&& event)
		{
			EventManager::instance()->queue<EventType>(std::weak_ptr<Object>(sender->as<Object>()), std::move(event));
		}

		template<typename EventType>
		static void queue(std::weak_ptr<Object> sender, EventType&& event)
		{
			EventManager::instance()->queue<EventType>(sender, std::move(event));
		}

		template<typename EventType>
		static void queue(std::weak_ptr<Object> sender, const EventType& event)
		{
			EventManager::instance()->queue<EventType>(sender, event);
		}

		template<typename EventType>
		static void queue(EventType&& event)
		{
			EventManager::instance()->queue<EventType>(std::move(event));
		}

		template<typename EventType>
		static void queue(const EventType& event)
		{
			EventManager::instance()->queue<EventType>(event);
		}

		static void update()
		{
			EventManager::instance()->update();
		}

		static void unlisten(EventListenerHandle& handle)
		{
			handle.unlisten();
		}
	};
}