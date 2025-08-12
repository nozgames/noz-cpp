/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "GLTFLoader.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace noz::import
{
    GLTFLoader::GLTFLoader()
        : _data(nullptr)
    {
    }
        
    GLTFLoader::~GLTFLoader()
    {
        close();
    }
        
    bool GLTFLoader::open(const std::string& path)
    {
        close();
            
        cgltf_options options = {};
        cgltf_data* data = nullptr;            
        
		cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
        if (result != cgltf_result_success)
        {
			Log::error("GLTFLoader", path + ": invalid gltf file");
            return false;
        }
            
        result = cgltf_load_buffers(&options, data, path.c_str());
        if (result != cgltf_result_success)
        {
			Log::error("GLTFLoader", path + ": invalid gltf file");
            cgltf_free(data);
            return false;
        }
            
        _data = data;
        _path = path;
            
        return true;
    }
        
    void GLTFLoader::close()
    {
		if (!_data)
			return;

		cgltf_free(_data);
        _data = nullptr;
    }
        
    GLTFLoader::BoneFilter GLTFLoader::BoneFilter::fromMetaFile(const std::string& metaPath)
    {
		BoneFilter config;
            
        if (!std::filesystem::exists(metaPath))
            return config;
                
        auto metaFile = MetaFile::parse(metaPath);
            
        // Parse excludeBones from comma-separated string
        auto excludeBonesStr = metaFile.getString("Mesh", "excludeBones", "");
        if (!excludeBonesStr.empty())
        {
            std::stringstream ss(excludeBonesStr);
            std::string boneName;
            while (std::getline(ss, boneName, ','))
            {
                // Remove quotes and whitespace
                boneName.erase(std::remove(boneName.begin(), boneName.end(), '"'), boneName.end());
                boneName.erase(std::remove(boneName.begin(), boneName.end(), ' '), boneName.end());
                if (!boneName.empty())
                {
                    config.excludeBones.push_back(boneName);
                }
            }
        }
            
        config.keepLeafBones = metaFile.getBool("Mesh", "keepLeafBones", false);
            
        return config;
    }

    std::vector<GLTFLoader::Bone> GLTFLoader::readBones(const BoneFilter& filter)
    {
        std::vector<Bone> bones;
            
        if (!_data || !_data->nodes_count)
            return bones;
            
        // First, identify which nodes are root nodes (have no parents)
        std::vector<bool> isRootNode(_data->nodes_count, true);
		const cgltf_node* root = nullptr;
        for (size_t i = 0; nullptr == root && i < _data->nodes_count; ++i)
        {
            const cgltf_node* node = &_data->nodes[i];
			std::string nodeName(node->name);
			if (nodeName == "root")
				root = node;
        }

		if (nullptr == root)
			return bones;
            
		readBones(root, bones, -1, filter);
            
        return bones;
    }
        
    void GLTFLoader::readBones(
        const cgltf_node* node,
        std::vector<Bone>& bones,
        int parentIndex,
		const BoneFilter& filter)
    {
        if (!node)
            return;
                
        // Check if this bone should be excluded
        if (std::find(filter.excludeBones.begin(), filter.excludeBones.end(), node->name) != filter.excludeBones.end())
            return;
                
        // Check if this is a leaf bone (no bone children) and we don't want to keep them
        if (!filter.keepLeafBones && isBoneLeaf(node, filter.excludeBones))
            return;
                
        readBone(node, bones, parentIndex, filter);
    }
        
    bool GLTFLoader::readBone(
        const cgltf_node* node,
        std::vector<Bone>& bones,
        int parentIndex,
		const BoneFilter& filter)
    {
		assert(node);
                
        Bone bone;
        bone.name = node->name ? node->name : "";
        bone.index = static_cast<int>(bones.size());
        bone.parentIndex = parentIndex;
		bone.position = convertVector3(node->translation);
		bone.rotation = convertQuaternion(node->rotation);
		bone.scale = convertVector3(node->scale);
		bone.localToWorld =
			glm::translate(bone.position) *
			glm::mat4_cast(bone.rotation) *
			glm::scale(bone.scale);

		if (parentIndex >= 0)
			bone.localToWorld = bones[parentIndex].localToWorld * bone.localToWorld;

        bone.worldToLocal = glm::inverse(bone.localToWorld);
        bone.length = 0.0f;
        bone.direction = glm::vec3(0, 1, 0);
        bones.push_back(bone);
            
        for (auto i = 0; i < node->children_count; ++i)
            readBones(node->children[i], bones, bone.index, filter);
            
        return true;
    }
        
    bool GLTFLoader::isBoneLeaf(const cgltf_node* node, const std::vector<std::string>& excludeBones)
    {
		assert(node);
		assert(node->name);
		assert(node->name[0] != 0);

		// if the name ends in _leaf the nit is a leaf bone
		if (std::string(node->name).find("_leaf") != std::string::npos)
			return true;
            
		return false;
    }
        
    void GLTFLoader::updateParentBoneLength(const Bone& bone, Bone& parentBone)
    {
        const auto& parentPos = parentBone.position;
		const auto& bonePos = bone.position;
		auto direction = glm::normalize(bonePos - parentPos);
            
        parentBone.direction = direction;
        parentBone.length = glm::length(bonePos - parentPos);
    }
        
    std::shared_ptr<GLTFLoader::Animation> GLTFLoader::readAnimation(const std::vector<Bone>& meshBones, const std::string& name)
    {
        std::vector<std::shared_ptr<Animation>> animations;
		if (!_data || !_data->animations_count)
			return nullptr;
            
        return readAnimation(&_data->animations[0], meshBones, name);
    }
        
    std::shared_ptr<GLTFLoader::Animation> GLTFLoader::readAnimation(
		const cgltf_animation* gltfAnimation,
		const std::vector<Bone>& meshBones,
		const std::string& name)
    {
		assert(gltfAnimation);
		assert(_data);
                
        auto animation = std::make_shared<Animation>();
		animation->frameCount = readFrameCount(gltfAnimation, meshBones);
			
		auto tracks = readAnimationTracks(gltfAnimation, meshBones, animation);
		for (const auto& trackPair : tracks)
            readTrackData(animation, trackPair.second, trackPair.first);

        return animation;
    }
        
    int GLTFLoader::readFrameCount(const cgltf_animation* animation, const std::vector<Bone>& meshBones)
    {
        if (!animation || !_data)
            return 0;
                
        int maxFrames = 0;
            
        for (size_t i = 0; i < animation->channels_count; ++i)
        {
            const cgltf_animation_channel* channel = &animation->channels[i];
            if (channel->sampler && channel->sampler->input)
            {
                int frames = static_cast<int>(channel->sampler->input->count);
                maxFrames = std::max(maxFrames, frames);
            }
        }
            
        return maxFrames;
    }
        
    std::vector<std::pair<const cgltf_accessor*, noz::renderer::AnimationTrack>> GLTFLoader::readAnimationTracks(
        const cgltf_animation* gltfAnimation,
        const std::vector<Bone>& meshBones,
        std::shared_ptr<Animation>& animation)
    {
        std::vector<std::pair<const cgltf_accessor*, noz::renderer::AnimationTrack>> tracks;
            
        if (!gltfAnimation || !_data)
            return tracks;
                
		size_t dataOffset = 0;
        for (size_t i = 0; i < gltfAnimation->channels_count; ++i)
        {
            const cgltf_animation_channel* channel = &gltfAnimation->channels[i];
            if (!channel->target_node || !channel->sampler)
                continue;
                    
            // Find bone index
            std::string boneName = channel->target_node->name ? channel->target_node->name : "";
            int boneIndex = -1;
            for (size_t j = 0; j < meshBones.size(); ++j)
            {
                if (meshBones[j].name == boneName)
                {
                    boneIndex = static_cast<int>(j);
                    break;
                }
            }
                
            if (boneIndex == -1)
                continue;
                    
			// TrackType
			auto trackType = noz::renderer::AnimationTrackType::Translation;                
            if (channel->target_path == cgltf_animation_path_type_rotation)
				trackType = noz::renderer::AnimationTrackType::Rotation;
            else if (channel->target_path == cgltf_animation_path_type_scale)
				trackType = noz::renderer::AnimationTrackType::Scale;

			// Check if this track matches bone's local values (no actual animation)
			const cgltf_accessor* accessor = channel->sampler->output;
			if (isTrackAllDefaults(accessor, trackType, meshBones[boneIndex]))
			{
				noz::Log::info("GLTFLoader", "Skipping track for bone '" + boneName + "' - no animation (matches bone's local transform)");
				continue;
			}

            // Create track and add to animation
			noz::renderer::AnimationTrack track{};
			track.boneIndex = (uint8_t)boneIndex;
			track.type = trackType;
			track.dataOffset = (int)dataOffset;

			dataOffset += trackType == noz::renderer::AnimationTrackType::Rotation ? 4 : 3;

			animation->tracks.push_back(track);
            tracks.push_back(std::make_pair(channel->sampler->output, track));
        }

		animation->frameStride = (int)dataOffset;
		animation->data.resize(dataOffset * animation->frameCount);
            
        return tracks;
    }
        
    void GLTFLoader::readTrackData(
		std::shared_ptr<Animation>& animation,
		const noz::renderer::AnimationTrack& track,
		const cgltf_accessor* accessor)
    {
		assert(animation);
		assert(accessor);
                
        const void* data = getBufferData(accessor);
		auto stride = getBufferStride(accessor);
        auto count = accessor->count;
		auto componentCount = track.type == noz::renderer::AnimationTrackType::Rotation ? 4 : 3;
            
        for (size_t i = 0; i < count; ++i)
        {
            auto* src = reinterpret_cast<const float*>(static_cast<const char*>(data) + i * stride);
			auto* dst = animation->data.data() + track.dataOffset + i * animation->frameStride;
			memcpy(dst, src, sizeof(float) * componentCount);
        }
    }
        
    std::vector<const cgltf_animation_channel*> GLTFLoader::findChannelsForBoneInAnimation(
        const cgltf_animation* animation,
        const std::string& boneName)
    {
        std::vector<const cgltf_animation_channel*> channels;
            
        if (!animation)
            return channels;
                
        for (size_t i = 0; i < animation->channels_count; ++i)
        {
            const cgltf_animation_channel* channel = &animation->channels[i];
            if (channel->target_node && channel->target_node->name && 
                std::string(channel->target_node->name) == boneName)
            {
                channels.push_back(channel);
            }
        }
            
        return channels;
    }
        
    std::vector<const cgltf_animation_channel*> GLTFLoader::findChannelsForBone(const std::string& boneName)
    {
        std::vector<const cgltf_animation_channel*> channels;
            
        if (!_data)
            return channels;
                
        for (size_t i = 0; i < _data->animations_count; ++i)
        {
            auto animChannels = findChannelsForBoneInAnimation(&_data->animations[i], boneName);
            channels.insert(channels.end(), animChannels.begin(), animChannels.end());
        }
            
        return channels;
    }
        
    GLTFLoader::MeshData GLTFLoader::readMesh(const std::vector<Bone>& bones)
    {
        MeshData meshData;
            
        if (!_data || !_data->meshes_count)
            return meshData;
                
        const cgltf_mesh* mesh = &_data->meshes[0];
        const cgltf_skin* skin = _data->skins_count > 0
			? &_data->skins[0]
			: nullptr;
            
        return readMesh(mesh, bones, skin);
    }
        
    GLTFLoader::MeshData GLTFLoader::readMesh(const cgltf_mesh* mesh, const std::vector<Bone>& bones, const cgltf_skin* skin)
    {
		assert(mesh);

        MeshData meshData;
            
		// Build a mapping from joint node to bone index when we have a filtered bone list
		std::unordered_map<const cgltf_node*, int> jointToBoneIndex;
		if (skin && !bones.empty())
		{
			// First create a map of bone names to indices in our filtered bone list
			std::unordered_map<std::string, int> boneNameToIndex;
			for (size_t i = 0; i < bones.size(); ++i)
			{
				boneNameToIndex[bones[i].name] = static_cast<int>(i);
			}
			
			// Then map each joint node to its corresponding bone index
			for (size_t i = 0; i < skin->joints_count; ++i)
			{
				const cgltf_node* jointNode = skin->joints[i];
				if (jointNode && jointNode->name)
				{
					auto it = boneNameToIndex.find(jointNode->name);
					if (it != boneNameToIndex.end())
					{
						jointToBoneIndex[jointNode] = it->second;
					}
				}
			}
		}
		
        if (!mesh || !mesh->primitives_count)
            return meshData;
                
        // Read first primitive
        const cgltf_primitive* primitive = &mesh->primitives[0];
            
        // Read positions
        if (primitive->attributes_count > 0)
        {
            for (size_t i = 0; i < primitive->attributes_count; ++i)
            {
                const cgltf_attribute* attr = &primitive->attributes[i];
                    
                if (attr->type == cgltf_attribute_type_position)
                {
                    const cgltf_accessor* accessor = attr->data;
                    size_t count = accessor->count;
                    meshData.positions.resize(count);
                        
                    const void* data = getBufferData(accessor);
                    size_t stride = getBufferStride(accessor);
                        
                    for (size_t j = 0; j < count; ++j)
                    {
                        const float* vertexData = reinterpret_cast<const float*>(static_cast<const char*>(data) + j * stride);
                        meshData.positions[j] = convertVector3(vertexData);
                    }
                }
                else if (attr->type == cgltf_attribute_type_normal)
                {
                    const cgltf_accessor* accessor = attr->data;
                    size_t count = accessor->count;
                    meshData.normals.resize(count);
                        
                    const void* data = getBufferData(accessor);
                    size_t stride = getBufferStride(accessor);
                        
                    for (size_t j = 0; j < count; ++j)
                    {
                        const float* vertexData = reinterpret_cast<const float*>(static_cast<const char*>(data) + j * stride);
                        meshData.normals[j] = convertVector3(vertexData);
                    }
                }
                else if (attr->type == cgltf_attribute_type_texcoord)
                {
                    const cgltf_accessor* accessor = attr->data;
                    size_t count = accessor->count;
                    meshData.uvs.resize(count);
                        
                    const void* data = getBufferData(accessor);
                    size_t stride = getBufferStride(accessor);
                        
                    for (size_t j = 0; j < count; ++j)
                    {
                        const float* vertexData = reinterpret_cast<const float*>(static_cast<const char*>(data) + j * stride);
                        meshData.uvs[j] = convertUv(vertexData);
                    }
                }
                else if (attr->type == cgltf_attribute_type_joints)
                {
                    const cgltf_accessor* accessor = attr->data;
                    size_t count = accessor->count;
                    meshData.boneIndices.resize(count);
                        
                    const void* data = getBufferData(accessor);
                    size_t stride = getBufferStride(accessor);
                        
                    for (size_t j = 0; j < count; ++j)
                    {
						uint32_t jointIndex = 0;
						
                        // Joint indices can be stored as various component types
                        if (accessor->component_type == cgltf_component_type_r_8u)
                        {
                            const uint8_t* jointData = reinterpret_cast<const uint8_t*>(static_cast<const char*>(data) + j * stride);
                            jointIndex = static_cast<uint32_t>(jointData[0]); // Use first joint for single bone per vertex
                        }
                        else if (accessor->component_type == cgltf_component_type_r_16u)
                        {
                            const uint16_t* jointData = reinterpret_cast<const uint16_t*>(static_cast<const char*>(data) + j * stride);
                            jointIndex = static_cast<uint32_t>(jointData[0]); // Use first joint for single bone per vertex
                        }
                        else if (accessor->component_type == cgltf_component_type_r_32u)
                        {
                            const uint32_t* jointData = reinterpret_cast<const uint32_t*>(static_cast<const char*>(data) + j * stride);
                            jointIndex = jointData[0]; // Use first joint for single bone per vertex
                        }
						
						// Map the joint index to our filtered bone index
						if (skin && !jointToBoneIndex.empty() && jointIndex < skin->joints_count)
						{
							const cgltf_node* jointNode = skin->joints[jointIndex];
							auto it = jointToBoneIndex.find(jointNode);
							if (it != jointToBoneIndex.end())
							{
								meshData.boneIndices[j] = static_cast<uint32_t>(it->second);
							}
							else
							{
								// Joint not found in filtered bones, use 0
								meshData.boneIndices[j] = 0;
							}
						}
						else
						{
							// No skin or no filtering, use joint index directly
							meshData.boneIndices[j] = jointIndex;
						}
                    }
                }
            }
        }
            
        // Read indices
        if (primitive->indices)
        {
            const cgltf_accessor* accessor = primitive->indices;
            size_t count = accessor->count;
            meshData.indices.resize(count);
                
            const void* data = getBufferData(accessor);
            size_t stride = getBufferStride(accessor);
                
            for (size_t i = 0; i < count; ++i)
            {
                if (accessor->component_type == cgltf_component_type_r_16u)
                {
                    const uint16_t* indexData = reinterpret_cast<const uint16_t*>(static_cast<const char*>(data) + i * stride);
                    meshData.indices[i] = *indexData;
                }
                else if (accessor->component_type == cgltf_component_type_r_32u)
                {
                    const uint32_t* indexData = reinterpret_cast<const uint32_t*>(static_cast<const char*>(data) + i * stride);
                    if (*indexData > 65535)
                    {
                        std::cerr << "Warning: Index " << *indexData << " exceeds 16-bit limit, truncating to " << (*indexData & 0xFFFF) << std::endl;
                    }
                    meshData.indices[i] = static_cast<uint16_t>(*indexData);
                }
            }
        }
            
        return meshData;
    }
        
    // Buffer access helpers
    const void* GLTFLoader::getBufferData(const cgltf_accessor* accessor)
    {
        if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer)
            return nullptr;
                
        const cgltf_buffer_view* view = accessor->buffer_view;
        const cgltf_buffer* buffer = view->buffer;
            
        return static_cast<const char*>(buffer->data) + view->offset;
    }
        
    size_t GLTFLoader::getBufferStride(const cgltf_accessor* accessor)
    {
        if (!accessor || !accessor->buffer_view)
            return 0;
                
        return accessor->buffer_view->stride
			? accessor->buffer_view->stride
			: cgltf_num_components(accessor->type) * cgltf_component_size(accessor->component_type);
    }
        
    size_t GLTFLoader::getBufferSize(const cgltf_accessor* accessor)
    {
        if (!accessor || !accessor->buffer_view)
            return 0;
                
        return accessor->buffer_view->size;
    }
        
    // Conversion helpers
    glm::mat4 GLTFLoader::convertMatrix(const float* matrix)
    {
        if (!matrix)
            return glm::mat4(1.0f);
                
        return glm::mat4(
            matrix[0], matrix[1], matrix[2], matrix[3],
            matrix[4], matrix[5], matrix[6], matrix[7],
            matrix[8], matrix[9], matrix[10], matrix[11],
            matrix[12], matrix[13], matrix[14], matrix[15]
        );
    }
        
    glm::vec3 GLTFLoader::convertVector3(const float* vector)
    {
        if (!vector)
            return glm::vec3(0.0f);
                
        return glm::vec3(vector[0], vector[2], vector[1]);
    }
                
    glm::vec2 GLTFLoader::convertVector2(const float* vector2)
    {
        if (!vector2)
            return glm::vec2(0.0f);
                
        return glm::vec2(vector2[0], vector2[1]);
    }
        
    glm::vec2 GLTFLoader::convertUv(const float* vector2)
    {
        if (!vector2)
            return glm::vec2(0.0f);
                
        return glm::vec2(vector2[0], vector2[1]);
    }
        
    glm::quat GLTFLoader::convertQuaternion(const float* quaternion)
    {
        if (!quaternion)
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                
        return glm::quat(quaternion[3], quaternion[0], quaternion[1], quaternion[2]);
    }
    
    bool GLTFLoader::isTrackAllDefaults(const cgltf_accessor* accessor, noz::renderer::AnimationTrackType trackType, const Bone& bone)
    {
        if (!accessor)
            return true;
            
        const void* data = getBufferData(accessor);
        if (!data)
            return true;
            
        size_t stride = getBufferStride(accessor);
        size_t count = accessor->count;
        
        // Check if all values match the bone's local transform
        // If they do, this track doesn't actually animate anything
        for (size_t i = 0; i < count; ++i)
        {
            const float* values = reinterpret_cast<const float*>(static_cast<const char*>(data) + i * stride);
            
            switch (trackType)
            {
            case noz::renderer::AnimationTrackType::Translation:
                // Check if position matches bone's local position
                if (!noz::math::approximately(values[0], bone.position.x) ||
                    !noz::math::approximately(values[1], bone.position.y) ||
                    !noz::math::approximately(values[2], bone.position.z))
                {
                    return false;
                }
                break;
                
            case noz::renderer::AnimationTrackType::Rotation:
                // Check if rotation matches bone's local rotation
                // Note: GLTF stores as (x, y, z, w), glm::quat is (w, x, y, z)
                if (!noz::math::approximately(values[0], bone.rotation.x) || 
                    !noz::math::approximately(values[1], bone.rotation.y) || 
                    !noz::math::approximately(values[2], bone.rotation.z) ||
                    !noz::math::approximately(values[3], bone.rotation.w))
                {
                    return false;
                }
                break;
                
            case noz::renderer::AnimationTrackType::Scale:
                // Check if scale matches bone's local scale
                if (!noz::math::approximately(values[0], bone.scale.x) || 
                    !noz::math::approximately(values[1], bone.scale.y) || 
                    !noz::math::approximately(values[2], bone.scale.z))
                {
                    return false;
                }
                break;
            }
        }
        
        return true;
    }
}
