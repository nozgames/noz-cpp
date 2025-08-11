/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
	class Node;
    class DirectionalLight;
    class Camera;

    class Scene : public Object
    {
    public:

		NOZ_DECLARE_TYPEID(Scene, noz::Object)

        ~Scene();

        // Scene management
        void setRoot(std::shared_ptr<Node> root);
        std::shared_ptr<Node> root() const { return _root; }

        // Scene lifecycle
        virtual void update();
        virtual void lateUpdate();
        virtual void start();
        virtual void render(noz::renderer::CommandBuffer* commandBuffer);
        
        // Start management
        void markForStart(std::shared_ptr<Node> node);
        
        // Destroy management
        void markForDestroy(std::shared_ptr<Node> node);
        void processDestroyList();

        // Scene properties
        const std::string& name() const { return _name; }
        void setName(const std::string& name) { _name = name; }

        // Light management
        void setActiveDirectionalLight(std::shared_ptr<DirectionalLight> light);
        std::shared_ptr<DirectionalLight> activeDirectionalLight() const { return _activeDirectionalLight; }

		std::shared_ptr<Camera> camera() const { return _camera; }  

    protected:

        Scene();

        virtual void initialize() {}

    private:

        friend class Camera;

		bool _started;
		std::string _name;
		std::shared_ptr<Node> _root;
        std::shared_ptr<Node> _startRoot;
        std::shared_ptr<Camera> _camera;
        std::vector<std::shared_ptr<Node>> _destroyList;
        std::shared_ptr<DirectionalLight> _activeDirectionalLight;
    };
}
