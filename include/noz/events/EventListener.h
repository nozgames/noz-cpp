#pragma once

#include <functional>
#include <memory>
#include <typeindex>

namespace noz
{
	class EventListenerHandle
	{
	public:
		EventListenerHandle();
		EventListenerHandle(size_t id, std::function<void(size_t)> unlistenFunc);
		
		// No copy - move only
		EventListenerHandle(const EventListenerHandle&) = delete;
		EventListenerHandle& operator=(const EventListenerHandle&) = delete;
		
		// Move semantics
		EventListenerHandle(EventListenerHandle&& other) noexcept;
		EventListenerHandle& operator=(EventListenerHandle&& other) noexcept;
		
		~EventListenerHandle();

		bool isValid() const;
		void unlisten();  // Unregister and clear handle

	private:
		size_t _id;
		std::function<void(size_t)> _unlistenFunc;
		
		void reset();  // Clear handle without unregistering (for move semantics)
	};

	class ScopedEventListener
	{
	public:
		ScopedEventListener();
		ScopedEventListener(EventListenerHandle&& handle);
		ScopedEventListener(const ScopedEventListener&) = delete;
		ScopedEventListener& operator=(const ScopedEventListener&) = delete;
		ScopedEventListener(ScopedEventListener&& other) noexcept;
		ScopedEventListener& operator=(ScopedEventListener&& other) noexcept;
		~ScopedEventListener();

		bool isValid() const;
		void unlisten();

	private:
		EventListenerHandle _handle;
	};

}