/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
	NOZ_DEFINE_TYPEID(IResource)

    IResource::IResource(const std::string& name)
        : _name(name)
    {
    }

    IResource::~IResource()
    {
        // Unregister from the Resources singleton if we were registered
        if (_resourceIndex != SIZE_MAX)
        {
            auto* resources = Resources::instance();
            if (resources)
            {
                resources->unregisterResource(this);
            }
        }
    }

    void IResource::autoUnload()
    {
        if (_resourceIndex != SIZE_MAX)
            return;

        auto* resources = Resources::instance();
        if (resources)
        {
            resources->registerResource(this);
        }
    }
}
