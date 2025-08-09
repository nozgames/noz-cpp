#pragma once

#include "Object.h"

namespace noz
{
	template<typename T> class ISingleton : public Object
	{
	public:
		
		static std::shared_ptr<T> instance()
		{
			return _instance;
		}
		
		static std::shared_ptr<T> load()
		{
			assert(!_instance);			
			_instance = std::shared_ptr<T>(new T());
			return _instance;
		}
		
		static void unload()
		{
			assert(_instance);
			_instance.reset();
		}
		
	protected:

		ISingleton() = default;
		virtual ~ISingleton() = default;
		
		// Prevent copying
		ISingleton(const ISingleton&) = delete;
		ISingleton& operator=(const ISingleton&) = delete;
		
		// Prevent moving
		ISingleton(ISingleton&&) = delete;
		ISingleton& operator=(ISingleton&&) = delete;

		// Friend declaration to allow access to protected methods
		friend class ISingleton<T>;

	private:

		static std::shared_ptr<T> _instance;
	};

	template<typename T> std::shared_ptr<T> ISingleton<T>::_instance = nullptr;
}