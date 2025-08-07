/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

// Forward declarations for cgltf types
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;
struct cgltf_animation;
struct cgltf_animation_channel;
struct cgltf_skin;
struct cgltf_primitive;
struct cgltf_accessor;
struct cgltf_buffer_view;
struct cgltf_buffer;

namespace noz::import
{
    class GLTFLoader
    {
    public:

		struct Bone
		{
			std::string name;
			int index;
			int parentIndex;
			glm::mat4 worldToLocal;
			glm::mat4 localToWorld;
			glm::vec3 position;
			glm::quat rotation;
			glm::vec3 scale;
			float length;
			glm::vec3 direction;
		};

		struct Animation
		{
			int frameCount;
			int frameStride;
			std::vector<noz::renderer::AnimationTrack> tracks;
			std::vector<float> data;
		};

		struct BoneFilter
		{
			std::vector<std::string> excludeBones;
			bool keepLeafBones = false;

			// Static method to create config from meta file
			static BoneFilter fromMetaFile(const std::string& metaPath);
		};

        GLTFLoader();
        ~GLTFLoader();
            
        // File operations
        bool open(const std::string& path);
        void close();
            
        std::vector<Bone> readBones(const BoneFilter& config = BoneFilter{});
            
        std::shared_ptr<Animation> readAnimation(const std::vector<Bone>& meshBones, const std::string& name);
            

        // Mesh data structure
        struct MeshData
        {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;
            std::vector<uint32_t> boneIndices;
            std::vector<uint16_t> indices;
        };

        // Read mesh data
        MeshData readMesh(const std::vector<Bone>& bones = {});
            
    private:
        // Internal state
        cgltf_data* _data;
        std::string _path;
            
        // Internal helper methods
        MeshData readMesh(const cgltf_mesh* mesh, const std::vector<Bone>& bones, const cgltf_skin* skin);
            
        void readBones(
            const cgltf_node* node,
            std::vector<Bone>& bones,
            int parentIndex,
			const BoneFilter& filter);

        bool readBone(
            const cgltf_node* node,
            std::vector<Bone>& bones,
            int parentIndex,
			const BoneFilter& filter);

        // Helper to determine if a node is a leaf bone (has no bone children)
        bool isBoneLeaf(const cgltf_node* node, const std::vector<std::string>& excludeBones);

        void updateParentBoneLength(const Bone& bone, Bone& parentBone);

        int readFrameCount(const cgltf_animation* animation, const std::vector<Bone>& meshBones);
        std::vector<std::pair<const cgltf_accessor*, noz::renderer::AnimationTrack>> readAnimationTracks(
            const cgltf_animation* animation,
            const std::vector<Bone>& meshBones,
            std::shared_ptr<Animation>& animationObj);
            
		std::shared_ptr<Animation> readAnimation(const cgltf_animation* animation, const std::vector<Bone>& meshBones, const std::string& name);

        void readTrackData(
            std::shared_ptr<Animation>& animation,
            const noz::renderer::AnimationTrack& track,
            const cgltf_accessor* accessor);

        void readAnimationTrack(
            const cgltf_animation_channel* channel,
            const Bone& bone,
            const std::vector<Bone>& meshBones,
            Animation* animation,
            std::vector<std::pair<const cgltf_accessor*, noz::renderer::AnimationTrack>>& accessorTracks);
                
        std::vector<const cgltf_animation_channel*> findChannelsForBoneInAnimation(
            const cgltf_animation* animation,
            const std::string& boneName);
        std::vector<const cgltf_animation_channel*> findChannelsForBone(
            const std::string& boneName);
                        
    private:
            
        // Buffer access helpers
        static const void* getBufferData(const cgltf_accessor* accessor);
        static size_t getBufferStride(const cgltf_accessor* accessor);
        static size_t getBufferSize(const cgltf_accessor* accessor);
            
        // Conversion helpers
        static glm::mat4 convertMatrix(const float* matrix);
        static glm::vec3 convertVector3(const float* vector);
        static glm::vec2 convertVector2(const float* vector2);
        static glm::vec2 convertUv(const float* vector2);
        static glm::quat convertQuaternion(const float* quaternion);
        
        // Helper to check if track matches bone's local values (no actual animation)
        static bool isTrackAllDefaults(const cgltf_accessor* accessor, noz::renderer::AnimationTrackType trackType, const Bone& bone);
    };
}
