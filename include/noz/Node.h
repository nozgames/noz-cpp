/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
    class CommandBuffer;
}

namespace noz::node
{
	class Scene;

    enum class NodeState : uint8_t
    {
        None,           // Not in a scene, not started
        InScene,        // In a scene but not started yet
        Active,         // In a scene, started and active
        PendingDestroy, // Marked for destruction at end of frame
        Destroyed       // Fully destroyed
    };

	class Node : public Object
	{
		friend class Scene;

	public:

		NOZ_DECLARE_TYPEID(Node, Object)

		virtual ~Node();

		// Node hierarchy management  
		static void addChild(std::shared_ptr<Node> parent, std::shared_ptr<Node> child);
		void add(std::shared_ptr<Node> child);
		void remove(std::shared_ptr<Node> child);
		void removeFromParent();

		std::shared_ptr<Node> child(size_t index) const;
		std::shared_ptr<Node> child(const noz::TypeId& typeId) const;
		size_t childCount() const;
		std::shared_ptr<Node> parent() const;

		// Template wrappers for type safety with inheritance support
		template<typename T>
		std::shared_ptr<T> child() const
		{
			return std::static_pointer_cast<T>(child(T::TypeId));
		}

		template<typename T>
		std::shared_ptr<T> parent() const
		{
			auto parentNode = parent();
			while (parentNode)
			{
				if (parentNode->isA(T::TypeId))
				{
					return std::static_pointer_cast<T>(parentNode);
				}
				parentNode = parentNode->parent();
			}
			return nullptr;
		}

		bool hasName() const { return !_name.empty(); }

		const std::string& name() const { return _name; }

		void setName(const std::string& name);

		// Node traversal
		std::shared_ptr<Node> findChild(const std::string& name) const;
		std::shared_ptr<Node> findChildRecursive(const std::string& name) const;
		std::shared_ptr<Node> findCommonAncestor(std::shared_ptr<Node> other) const;

		// Internal lifecycle methods (called by Scene/system)
		void updateInternal();
		void lateUpdateInternal();
		void startInternal();
		void renderInternal(noz::renderer::CommandBuffer* commandBuffer);
		void destroyInternal();

		// Virtual lifecycle methods (override in derived classes)
		virtual void update() {}
		virtual void lateUpdate() {}
		virtual void start() {}
		virtual void render(noz::renderer::CommandBuffer* commandBuffer) {}
		virtual void destroy() {}

		// State management
		NodeState state() const { return _state; }
		bool isActive() const { return _state == NodeState::Active; }

		// Start management
		bool isStarted() const { return _state == NodeState::Active; }
		bool needsStart() const { return _state == NodeState::InScene; }

		// Scene access
		std::shared_ptr<Scene> scene() const { return _scene; }
		void setScene(std::shared_ptr<Scene> scene);

		// Node ID for identification
		uint64_t id() const { return _id; }

	protected:

		friend class Node2d;
		friend class Node3d;

		Node();

		virtual void initialize();

		// Called when this node is added to a parent
		virtual void onAttachedToParent();

		// Called when this node is removed from its parent
		virtual void onDetachedFromParent();

		// Called when this node (or its parent hierarchy) is attached to a scene
		virtual void onAttachToScene() {}

		// Called when this node (or its parent hierarchy) is detached from a scene
		virtual void onDetachFromScene() {}

		// Called when parent's transform changes (override in transform nodes)
		virtual void onParentTransformChanged() {}

		virtual void onNameChanged() {}

    private:
        std::string _name;
        uint64_t _id;
        std::shared_ptr<Scene> _scene;
        
        std::weak_ptr<Node> _parent;
        std::vector<std::shared_ptr<Node>> _children;
        
        // State management
        NodeState _state;
        
        static uint64_t _nextId;
        
        // Static vector used as a stack for safe iteration during updates
        static std::vector<std::shared_ptr<Node>> s_updateStack;
    };
} 