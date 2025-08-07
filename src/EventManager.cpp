#include <noz/EventManager.h>

namespace noz
{
	EventManager::EventManager()
	{
	}

	EventManager::~EventManager()
	{
	}

	void EventManager::load()
	{
		ISingleton<EventManager>::load();
		instance()->loadInternal();
	}

	void EventManager::unload()
	{
		instance()->unloadInternal();
		ISingleton<EventManager>::unload();
	}

	void EventManager::loadInternal()
	{
		while (!_mainQueue.empty())
		{
			_mainQueue.pop();
		}
	}

	void EventManager::unloadInternal()
	{
		while (!_mainQueue.empty())
		{
			_mainQueue.pop();
		}
	}

	void EventManager::update()
	{
		while (!_mainQueue.empty())
		{
			auto func = _mainQueue.front();
			_mainQueue.pop();
			
			func();
		}
	}
}