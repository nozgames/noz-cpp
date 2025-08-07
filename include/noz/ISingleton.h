#pragma once

namespace noz
{
	template<typename T> class ISingleton
	{
	public:
		
		static T* instance()
		{
			return _instance;
		}
		
		static T* load()
		{
			assert(!_instance);			
			_instance = new T();
			return _instance;
		}
		
		static void unload()
		{
			assert(_instance);
			delete _instance;
			_instance = nullptr;
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

		static T* _instance;
	};

	template<typename T> T* ISingleton<T>::_instance = nullptr;
}