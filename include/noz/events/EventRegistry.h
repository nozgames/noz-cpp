/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/
#pragma once

#include "EventListener.h"

namespace noz
{
	class EventListenerHandle;
	
	template<typename EventType>
	struct QueuedEventEntry
	{
		std::weak_ptr<Object> sender;
		EventType event;

		QueuedEventEntry(std::weak_ptr<Object> s, EventType&& e)
			: sender(s), event(std::move(e)) {}

		QueuedEventEntry(std::weak_ptr<Object> s, const EventType& e)
			: sender(s), event(e) {}
	};
	
	template<typename EventType>
	struct EventListener
	{
		size_t id;
		std::function<void(const EventType&, std::shared_ptr<Object>)> handleFunc;  // Type-erased method call
		std::weak_ptr<Object> listener;  // Store the listener object
		std::weak_ptr<Object> sender;
		bool hasSenderFilter;

		template<typename ListenerType>
		EventListener(size_t listenerID, std::weak_ptr<ListenerType> listenerPtr)
			: id(listenerID), listener(listenerPtr), hasSenderFilter(false)
		{
			// Store a lambda that captures the typed weak_ptr and calls the handle method
			handleFunc = [listenerPtr](const EventType& event, std::shared_ptr<Object> sender) -> void {
				auto locked = listenerPtr.lock();
				locked->handle(event, sender);
			};
		}

		template<typename ListenerType>
		EventListener(size_t listenerID, std::weak_ptr<ListenerType> listenerPtr, std::weak_ptr<Object> senderPtr)
			: id(listenerID), listener(listenerPtr), sender(senderPtr), hasSenderFilter(true)
		{
			// Store a lambda that captures the typed weak_ptr and calls the handle method
			handleFunc = [listenerPtr](const EventType& event, std::shared_ptr<Object> sender) -> bool {
				auto locked = listenerPtr.lock();
				if (locked) {
					return locked->handle(event, sender);
				}
				return false;
			};
		}
		
		bool isAlive() const
		{
			// Check if the listener object is still alive
			if (listener.expired())
				return false;
			
			// If we have a sender filter, check if the sender is still alive
			if (hasSenderFilter && sender.expired())
				return false;
				
			return true;
		}
	};

	// Helper to detect if a class has a method named handleEventType
	template<typename T, typename EventType>
	class has_handle_method
	{
		template<typename U>
		static auto test(int) -> decltype(std::declval<U>().handle(std::declval<const EventType&>(), std::declval<std::shared_ptr<Object>>()), std::true_type{});
		
		template<typename>
		static std::false_type test(...);
		
	public:
		static constexpr bool value = decltype(test<T>(0))::value;
	};

	template<typename EventType>
	class EventRegistry
	{
	public:
		static std::vector<EventListener<EventType>> listeners;
		static std::queue<QueuedEventEntry<EventType>> queue;
		static size_t nextID;
		
		// Stack to store listener snapshots during event processing
		static std::vector<EventListener<EventType>> processingStack;

		// Add listener - calls handle() method on the object
		template<typename ListenerType>
		static EventListenerHandle addListener(std::weak_ptr<ListenerType> listener)
		{
			static_assert(std::is_base_of_v<Object, ListenerType>, "Listener must inherit from Object");
			static_assert(has_handle_method<ListenerType, EventType>::value, 
				"Listener must have a handle(const EventType&, std::shared_ptr<Object>) method");
			
			// Periodically clean up dead listeners (every 10 additions)
			if (nextID % 10 == 0)
			{
				cleanupDeadListeners();
			}
			
			size_t id = nextID++;
			listeners.emplace_back(id, listener);
			
			auto unlistenFunc = [](size_t listenerId) {
				removeListener(listenerId);
			};
			
			return EventListenerHandle(id, std::move(unlistenFunc));
		}

		// Add listener with sender filter - calls handle() method on the object
		template<typename ListenerType>
		static EventListenerHandle addListener(std::weak_ptr<ListenerType> listener, std::weak_ptr<Object> sender)
		{
			static_assert(std::is_base_of_v<Object, ListenerType>, "Listener must inherit from Object");
			static_assert(has_handle_method<ListenerType, EventType>::value, 
				"Listener must have a handle(const EventType&, std::shared_ptr<Object>) method");
			
			// Periodically clean up dead listeners (every 10 additions)
			if (nextID % 10 == 0)
			{
				cleanupDeadListeners();
			}
			
			size_t id = nextID++;
			listeners.emplace_back(id, listener, sender);
			
			auto unlistenFunc = [](size_t listenerId) {
				removeListener(listenerId);
			};
			
			return EventListenerHandle(id, std::move(unlistenFunc));
		}

		static void removeListener(size_t id)
		{
			auto it = std::find_if(listeners.begin(), listeners.end(),
				[id](const EventListener<EventType>& listener) { return listener.id == id; });
			
			if (it != listeners.end())
			{
				listeners.erase(it);
			}
		}

		static void sendEvent(const EventType& event, std::weak_ptr<Object> sender)
		{
			// Save the starting index and count
			size_t startIndex = processingStack.size();
			size_t listenerCount = listeners.size();
			
			// Push current listeners onto the processing stack
			processingStack.insert(processingStack.end(), listeners.begin(), listeners.end());
			
			// Process listeners from the snapshot
			for (size_t i = 0; i < listenerCount; ++i)
			{
				auto& listener = processingStack[startIndex + i];
				
				// Check if listener is still alive
				if (!listener.isAlive())
				{
					// Mark for cleanup in the main list later
					continue;
				}
				
				bool shouldProcess = true;
				
				// Check sender filter if applicable
				if (listener.hasSenderFilter)
				{
					auto listenerSender = listener.sender.lock();
					auto eventSender = sender.lock();
					shouldProcess = (listenerSender == eventSender);
				}

				if (shouldProcess)
				{
					// Call the handle method via the stored function
					listener.handleFunc(event, sender.lock());
				}
			}
			
			// Remove the processed listeners from the stack
			processingStack.erase(processingStack.begin() + startIndex, processingStack.end());
			
			// Clean up dead listeners from the main list (only if we're not nested)
			if (processingStack.empty())
			{
				listeners.erase(
					std::remove_if(listeners.begin(), listeners.end(),
						[](const EventListener<EventType>& listener) { return !listener.isAlive(); }),
					listeners.end()
				);
			}
		}

		static void processQueue()
		{
			while (!queue.empty())
			{
				const auto& entry = queue.front();
				sendEvent(entry.event, entry.sender);
				queue.pop();
			}
		}

		static void queueEvent(std::weak_ptr<Object> sender, EventType&& event)
		{
			queue.emplace(sender, std::move(event));
		}

		static void queueEvent(std::weak_ptr<Object> sender, const EventType& event)
		{
			queue.emplace(sender, event);
		}

		static void cleanupDeadListeners()
		{
			// Remove all dead listeners
			listeners.erase(
				std::remove_if(listeners.begin(), listeners.end(),
					[](const EventListener<EventType>& listener) { return !listener.isAlive(); }),
				listeners.end()
			);
		}

		static void clear()
		{
			listeners.clear();
			while (!queue.empty())
			{
				queue.pop();
			}
			nextID = 1;
		}
	};

	// Static member definitions
	template<typename EventType>
	std::vector<EventListener<EventType>> EventRegistry<EventType>::listeners;

	template<typename EventType>
	std::queue<QueuedEventEntry<EventType>> EventRegistry<EventType>::queue;

	template<typename EventType>
	size_t EventRegistry<EventType>::nextID = 1;
	
	template<typename EventType>
	std::vector<EventListener<EventType>> EventRegistry<EventType>::processingStack;
}