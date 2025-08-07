/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz
{
    class TypeId 
    {
    public:

		using IdType = uint32_t;
        
    private:
        IdType _id;
        
    public:

        explicit TypeId(const char* typeName) : _id(hash(typeName)) {}

        IdType id() const { return _id; }
        
        bool operator==(const TypeId& other) const { return _id == other._id; }
        bool operator!=(const TypeId& other) const { return _id != other._id; }
        bool operator<(const TypeId& other) const { return _id < other._id; }
        
        // Static methods for getting TypeId
        template<typename T>
        static const TypeId& of() 
        {
			return T::TypeId;
        }       
        
    private:
        
		// Simple FNV-1a hash function
        static constexpr IdType hash(const char* str)
        {
            IdType hash = 2166136261u; // FNV offset basis
            while (*str)
            {
                hash ^= static_cast<IdType>(*str++);
                hash *= 16777619u; // FNV prime
            }
            return hash;
        }
    };
    
    #define NOZ_DECLARE_TYPEID(ClassName, BaseClassName) \
        friend class noz::Object; \
        static const noz::TypeId TypeId; \
        noz::TypeId typeId() const override { return TypeId; } \
		template<typename T> bool isA() const { return isA(noz::TypeId::of<T>()); } \
        bool isA(const noz::TypeId& a) const override { return TypeId == a || BaseClassName::isA(a); }

    #define NOZ_DEFINE_TYPEID(ClassName) \
        const noz::TypeId ClassName::TypeId(#ClassName);
}

// Hash function for TypeId to work with std::unordered_map
namespace std 
{
    template<>
    struct hash<noz::TypeId>
    {
        std::size_t operator()(const noz::TypeId& typeId) const noexcept
        {
            return static_cast<std::size_t>(typeId.id());
        }
    };
}