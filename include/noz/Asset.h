/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz
{
    template <typename T> class AssetCache
    {
    public:

        static std::shared_ptr<T> get(const std::string& name)
        {
            auto it = _assets.find(name);
            if (it == _assets.end()) 
                return nullptr;

            return it->second;
        }

        static std::shared_ptr<T> set(const std::string& name, const std::shared_ptr<T>& asset) 
        {
            if (!asset) 
                return nullptr;

            _assets[name] = asset;
            return asset;
        }

        static void clear()
        {
            _assets.clear();
        }

    private:    
        
        friend class Assets;

        static std::unordered_map<std::string, std::shared_ptr<T>> _assets;
    };

    class Asset : public Object
    {
    public:

		NOZ_DECLARE_TYPEID(Asset, Object)

        Asset(const std::string& path);

        virtual ~Asset();

        const std::string& name() const;

        virtual void reload() {}

        void autoUnload();

        size_t _index = SIZE_MAX;

        template<typename TAsset> static std::shared_ptr<TAsset> load(const std::string& name);

    private:

        std::string _name;
    };

    inline const std::string& Asset::name() const 
    {
        return _name;
    }

    template <typename T>
    std::unordered_map<std::string, std::shared_ptr<T>> AssetCache<T>::_assets = {};
}