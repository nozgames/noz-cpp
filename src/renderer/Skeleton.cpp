/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    Skeleton::Skeleton(const std::string& path)
        : noz::Asset(path)
    {
    }

    Skeleton::~Skeleton()
    {
    }

    std::shared_ptr<Animation> Skeleton::animation(const std::string& name) const
    {
        for (const auto& animation : _animations)
        {
            if (animation && animation->name() == name)
                return animation;
        }
        return nullptr;
    }

    std::shared_ptr<Animation> Skeleton::animation(int index) const
    {
        if (index >= 0 && index < static_cast<int>(_animations.size()))
            return _animations[index];
        return nullptr;
    }

    std::shared_ptr<Skeleton> Skeleton::load(const std::string& name)
    {
        return loadInternal(AssetDatabase::getFullPath(name, "skeleton"), name);
    }
    
    std::shared_ptr<Skeleton> Skeleton::loadInternal(const std::string& filePath, const std::string& resourceName)
    {
        auto skeleton = std::make_shared<Skeleton>(resourceName);
        
        noz::StreamReader reader;
        if (!reader.loadFromFile(filePath))
			throw std::runtime_error("file not found");
        
        if (!reader.readFileSignature("SKEL"))
			throw std::runtime_error("invalid signature");
        
        uint32_t version = reader.readUInt32();
        if (version != 1)
			throw std::runtime_error("invalid version");
                
		// Bones
        auto boneCount = reader.readUInt32();
        skeleton->_bones.reserve(boneCount);
        
        for (uint32_t i = 0; i < boneCount; ++i)
        {
			Bone bone = {};
            bone.name = reader.readString();
            bone.index = reader.readInt32();
            bone.parentIndex = reader.readInt32();
            bone.localToWorld = reader.read<glm::mat4>();
			bone.worldToLocal = reader.read<glm::mat4>();
			bone.transform.position = reader.read<glm::vec3>();
			bone.transform.rotation = reader.read<glm::quat>();
			bone.transform.scale = reader.read<glm::vec3>();
            bone.length = reader.readFloat();
			bone.direction = reader.read<glm::vec3>();
            
            skeleton->_bones.push_back(bone);
        }
        
        return skeleton;
    }

	int Skeleton::boneIndex(const std::string& name) const
	{
		for (size_t i = 0; i < _bones.size(); ++i)
		{
			if (_bones[i].name == name)
				return static_cast<int>(i);
		}

		return -1;
	}
}
