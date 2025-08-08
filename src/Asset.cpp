/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
	NOZ_DEFINE_TYPEID(Asset)

    Asset::Asset(const std::string& name)
        : _name(name)
    {
    }

    Asset::~Asset()
    {
        // Unregister from the AssetDatabase singleton if we were registered
        if (_index != SIZE_MAX)
        {
            auto* resources = AssetDatabase::instance();
            if (resources)
            {
                resources->unregisterAsset(this);
            }
        }
    }

    void Asset::autoUnload()
    {
        if (_index != SIZE_MAX)
            return;

        auto* resources = AssetDatabase::instance();
        if (resources)
        {
            resources->registerAsset(this);
        }
    }
}
