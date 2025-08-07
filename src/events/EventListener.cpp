#include <noz/events/EventListener.h>
#include <noz/EventManager.h>

namespace noz
{
	EventListenerHandle::EventListenerHandle()
		: _id(0)
	{
	}

	EventListenerHandle::EventListenerHandle(size_t id, std::function<void(size_t)> unlistenFunc)
		: _id(id), _unlistenFunc(std::move(unlistenFunc))
	{
	}

	EventListenerHandle::EventListenerHandle(EventListenerHandle&& other) noexcept
		: _id(other._id), _unlistenFunc(std::move(other._unlistenFunc))
	{
		other.reset();
	}

	EventListenerHandle& EventListenerHandle::operator=(EventListenerHandle&& other) noexcept
	{
		if (this != &other)
		{
			// Clean up current state
			unlisten();
			
			// Move from other
			_id = other._id;
			_unlistenFunc = std::move(other._unlistenFunc);
			
			// Reset other
			other.reset();
		}
		return *this;
	}

	EventListenerHandle::~EventListenerHandle()
	{
		unlisten();
	}

	bool EventListenerHandle::isValid() const
	{
		return _id != 0;
	}

	void EventListenerHandle::unlisten()
	{
		if (_id != 0 && _unlistenFunc)
		{
			_unlistenFunc(_id);
			reset();
		}
	}

	void EventListenerHandle::reset()
	{
		_id = 0;
		_unlistenFunc = nullptr;
	}

	ScopedEventListener::ScopedEventListener()
	{
	}

	ScopedEventListener::ScopedEventListener(EventListenerHandle&& handle)
		: _handle(std::move(handle))
	{
	}

	ScopedEventListener::ScopedEventListener(ScopedEventListener&& other) noexcept
		: _handle(std::move(other._handle))
	{
	}

	ScopedEventListener& ScopedEventListener::operator=(ScopedEventListener&& other) noexcept
	{
		if (this != &other)
		{
			_handle = std::move(other._handle);
		}
		return *this;
	}

	ScopedEventListener::~ScopedEventListener()
	{
		unlisten();
	}

	bool ScopedEventListener::isValid() const
	{
		return _handle.isValid();
	}

	void ScopedEventListener::unlisten()
	{
		_handle.unlisten();
	}
}