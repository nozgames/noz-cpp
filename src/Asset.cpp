/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
	NOZ_DEFINE_TYPEID(Asset)

    Asset::Asset()
    {
    }

    Asset::~Asset()
    {
        // Unregister from the AssetDatabase singleton if we were registered
        if (_index != SIZE_MAX)
        {
            AssetDatabase::instance()->unregisterAsset(this);
        }
    }

    void Asset::initialize(const std::string& name)
    {
        _name = name;
    }

    void Asset::autoUnload()
    {
        if (_index != SIZE_MAX)
            return;

        AssetDatabase::instance()->registerAsset(this);
    }
}
