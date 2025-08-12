/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/TypeId.h>

namespace noz
{
	class Object : public std::enable_shared_from_this<Object>
	{
	public:

		static const noz::TypeId TypeId;

		template<typename T> bool isA() const
		{
			return isA(noz::TypeId::of<T>());
		}

		virtual noz::TypeId typeId() const
		{
			return TypeId;
		}
		
		virtual bool isA(const noz::TypeId& a) const
		{
			return TypeId == a;
		}

		Object() = default;
		virtual ~Object() = default;

		template<typename T>
		std::shared_ptr<T> as()
		{
			assert(isA<T>());
			return std::static_pointer_cast<T>(this->shared_from_this());
		}

		// Factory method template for creating objects with initialization
		template<typename T, typename... Args>
		static std::shared_ptr<T> create(Args&&... args)
		{
			static_assert(std::is_base_of<Object, T>::value, "T must derive from Object");
			
			// Create the shared_ptr using token constructor
			auto obj = std::shared_ptr<T>(new T());
			
			// Call initialize - will fail to compile if it doesn't exist
			obj->initialize(std::forward<Args>(args)...);
			
			return obj;
		}

		// Factory method template for creating objects with initialization
		template<typename T>
		static std::shared_ptr<T> create()
		{
			static_assert(std::is_base_of<Object, T>::value, "T must derive from Object");

			// Create the shared_ptr using token constructor
			auto obj = std::shared_ptr<T>(new T());

			// Call initialize - will fail to compile if it doesn't exist
			obj->initialize();

			return obj;
		}

	protected:

		// Prevent copying (can be overridden by derived classes if needed)
		Object(const Object&) = default;
		Object& operator=(const Object&) = default;

		// Allow moving
		Object(Object&&) = default;
		Object& operator=(Object&&) = default;
	};
}