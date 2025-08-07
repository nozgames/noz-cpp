#pragma once

#include "ISingleton.h"
#include "events/EventListener.h"
#include "events/EventRegistry.h"
#include <memory>
#include <queue>

namespace noz
{
	class Object;
	
	class EventManager : public ISingleton<EventManager>
	{
	public:
		EventManager();
		virtual ~EventManager();

		static void load();
		static void unload();

		void update();

	private:
		friend class ISingleton<EventManager>;

		void loadInternal();
		void unloadInternal();
		std::queue<std::function<void()>> _mainQueue;
	};
}