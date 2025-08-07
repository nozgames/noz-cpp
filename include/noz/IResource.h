#pragma once

namespace noz
{
    template <typename T> class ResourceCache 
    {
    public:

        static std::shared_ptr<T> get(const std::string& name)
        {
            auto it = _resources.find(name);
            if (it == _resources.end()) 
                return nullptr;

            return it->second;
        }

        static std::shared_ptr<T> set(const std::string& name, const std::shared_ptr<T>& resource) 
        {
            if (!resource) 
                return nullptr;

            _resources[name] = resource;
            return resource;
        }

        static void clear()
        {
            _resources.clear();
        }

    private:    
        
        friend class Resources;

        static std::unordered_map<std::string, std::shared_ptr<T>> _resources;
    };

    class IResource : public Object
    {
    public:

		NOZ_DECLARE_TYPEID(IResource, Object)

        IResource(const std::string& path);

        virtual ~IResource();

        const std::string& name() const;

        // Reload the resource from its original source
        virtual void reload() {}

        // Enable automatic unloading during shutdown
        void autoUnload();

        // Index in the Resources vector for efficient removal
        size_t _resourceIndex = SIZE_MAX;

    private:

        std::string _name;
    };

    inline const std::string& IResource::name() const 
    {
        return _name;
    }

    template <typename T>
    std::unordered_map<std::string, std::shared_ptr<T>> ResourceCache<T>::_resources = {};

} // namespace noz 