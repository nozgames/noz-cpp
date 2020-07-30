///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Node_h__
#define __noz_Node_h__

#include "Component.h"
#include "NodePath.h"

namespace noz {

  class Layout;
  class Node;
  class Transform;
  class WindowNode;
  class Viewport;

  struct NodeAttributes : public Attributes {
    NodeAttributes(noz_uint32 value=0) : Attributes(value) {}
    static const noz_uint32 ApplicationRoot = NOZ_BIT(0);
    static const noz_uint32 SceneRoot = NOZ_BIT(1);
    static const noz_uint32 Viewport = NOZ_BIT(2);
    static const noz_uint32 WindowRoot = NOZ_BIT(3);
    static const noz_uint32 AllowRename = NOZ_BIT(4);
    static const noz_uint32 Default = AllowRename;
  };

  enum class NodeEvent {
    Update,
    LateUpdate,
    FixedUpdate,
    EditorUpdate,
    Animate,
    Mouse,
    Touch
  };

  NOZ_ENUM() enum class Visibility  {
    Visible,
    Hidden,
    Collapsed
  };

  NOZ_ENUM() enum class HitTestResult {
    /// The node was not hit by the hit test
    None,

    /// Indicates that the HitTest was over the render rectangle but not the actual node rectangle.  This 
    /// can happen when the Node is rotated for example since the render rectangle is axis aligned.
    RenderRect,

    /// Indicates that the HitTest was found to be within the nodes rectangle.
    Rect
  };

  class NodeManager {
    private: static NodeManager* this_;

    /// Master list of all nodes in sorted depth first order
    private: std::vector<ObjectPtr<Node>> nodes_;

    private: bool invalid_;

    public: NodeManager (void);

    public: static void Initialize (void);
    public: static void Uninitialize (void);
    public: static void Invalidate (void);
    public: static void Update (void);
    public: static bool IsInvalid (void) {return this_->invalid_;}

    public: static const std::vector<ObjectPtr<Node>>& GetNodes(void) {return this_->nodes_;}

    private: void Update (Node* n);
  };

  class Node : public Object {
    NOZ_OBJECT()
   
    friend class Viewport;
    friend class WindowNode;

    private: enum class DirtyState {
      Dirty,
      Clean,
      Cleaning,
    };

    private: struct {
      noz_uint32 auto_destroy_:1;

      /// True if the transform is invalid for this node.
      noz_uint32 invalid_transform_:1;

      /// Indicates the node is awake within a valid tree heirarchy
      noz_uint32 awake_:1;

      /// Indicates whether the Node is visible.  This value is indepenent of
      /// the nodes visibility setting as it reflects parental influences as well.
      noz_uint32 visible_:1;

      /// True if the node is visible in the arrangement.  This allows a node 
      /// to be hidden during arrangement without affecting its "Visibility"
      noz_uint32 arrange_visible_:1;

      noz_uint32 mouse_over_:1;

      /// True if the rendering of this node was culled for any reason by the rendering system.
      noz_uint32 render_culled_ : 1;

      /// True if children should be clipped to the node's rectangle
      noz_uint32 clip_children_ : 1;

      /// True if the node is an orphan
      noz_uint32 orphan_ : 1;

      noz_uint32 private_ : 1;

      noz_uint32 logical_children_only_ : 1;
    };

    NOZ_PROPERTY(Name=ClipChildren,Type=bool,Set=SetClipChildren,Get=IsClippingChildren,EditorVisible=false);

    NOZ_PROPERTY(Name=Components,Add=AddComponent,EditorVisible=false);
    private: std::vector<ObjectPtr<Component>> components_;

    /// Visibility state of the node
    NOZ_PROPERTY(Name=Visibility,Set=SetVisibility)
    private: Visibility visibility_;

    /// Optional transform 
    NOZ_PROPERTY(Name=Transform,Set=SetTransform,EditorVisible=false)
    private: ObjectPtr<Transform> transform_;

    private: NOZ_PROPERTY(Name=Name) Name name_;

    /// Opacity of the node
    private: NOZ_PROPERTY(Name=Opacity) noz_float opacity_;

    /// Viewport the node belongs to.
    private: Viewport* viewport_;

    /// Window the node belongs to
    private: Window* window_;

    /// Node attributes
    protected: NodeAttributes attr_;

    /// World coordinates of transform
    protected: Vector2 position_;

    /// Transform used to convert local coordinates to viewport coordinates
    protected: Matrix3 local_to_viewport_;

    /// Transform used to convert viewport coordinates to local coordinates
    protected: Matrix3 viewport_to_local_;

    /// Rectangle that represents the node in the world
    private: Rect rect_;    

    /// Rectangle in screen coordinates that the node was rendered at
    protected: Rect render_rect_;

    /// Measured size of the node.  This value is the cached result of a call to Measure.  
    private: Vector2 measured_size_;

    /// Rectangle last used to arrange this node.  This value is cached to allow the
    /// node arrangement to be updated without requiring the entire parent tree to
    /// be rearranged.
    private: Rect cached_arrange_rect_;

    /// Availble size last used to measure this node.  This value is cached to allow
    /// the node to be remeasured without remeasuring all parents.
    private: Vector2 cached_available_size_;

    /// Node Children
    NOZ_PROPERTY(Name=Children,EditorNotEditable,Serialize=SerializeChildren,Deserialize=DeserializeChildren,Add=AddChild,Remove=RemoveChildAt,Insert=InsertChild,Clear=RemoveAllChildren,Release=ReleaseChildAt)
    private: std::vector<ObjectPtr<Node>> children_;

    private: noz_uint32 private_child_count_;

    /// Pointer to the parent node 
    private: Node* parent_;

    private: Node* logical_parent_;

    /// Index of the node within its logical parent.
    private: noz_int32 logical_index_;

    /// Index of the node within its parent
    private: noz_int32 index_;

    /// Vector of all invalid nodes
    private: static std::vector<ObjectPtr<Node>> invalid_transforms_;

    private: static std::vector<ObjectPtr<Node>> destroyed_nodes_;

    /// Set to true during the UpdateDirty call
    private: static DirtyState dirty_state_;

    /// Default constructor
    public: Node(Name name=Name::Empty,NodeAttributes attr=NodeAttributes::Default);
   
    /// Default destructor
    public: ~Node(void);

    /// Destroy the node and remove it from the world
    public: void Destroy(void);

    public: void DestroyImmediate (void);

    public: virtual void Animate (void);


    public: void AddChild (Node* child);

    protected: void AddPrivateChild (Node* child);

    public: void InsertChild (noz_uint32 i, Node* child);

    public: void RemoveChildAt (noz_uint32 i);

    public: Node* ReleaseChildAt (noz_uint32 i);

    public: void RemoveAllChildren (void);

    public: void SetChildCapacity (noz_uint32 capacity);

    public: virtual void SortChildren (noz_int32 (*sort_proc) (const Node* node1, const Node* node2));

    public: void AddComponent (Component* component);

    public: template <typename T> T* AddComponent (void);

    public: void ReleaseComponent (Component* component);

    public: Component* GetComponent (Type* type) const;

    public: template <typename T> T* GetComponent (void) const {return (T*)GetComponent(typeof(T));}

    public: Component* GetComponent (noz_uint32 index) const {return components_[index];}

    public: noz_uint32 GetComponentCount (void) const {return components_.size();}
    
    /// Return the parent node
    public: Node* GetParent (void) const {return parent_ ? parent_ : logical_parent_;}

    /// Return the logical parent
    public: Node* GetLogicalParent (void) const {return logical_parent_;}

    public: Node* GetAncestor (Type* type) const;

    public: template <typename T> T* GetAncestor (void) const {return (T*)GetAncestor(typeof(T));}

    public: Node* GetNextSibling (void) const;

    public: Node* GetPrevSibling (void) const;

    public: Node* GetNextLogicalSibling (void) const;

    public: Node* GetPrevLogicalSibling (void) const;

    /// Return the viewport the node belongs to
    public: Viewport* GetViewport(void) const {return viewport_;}

    /// Return the scene the node belongs to
    public: Scene* GetScene(void) const;

    /// Return the window the node belongs to.
    public: Window* GetWindow(void) const {return window_;}

    /// Orphan the node from the world
    public: void Orphan (bool logical=true);



    /// Return true if the node is an orphan
    public: bool IsOrphan(void) const {return orphan_;}

    /// Return true if the node is awake
    public: bool IsAwake (void) const {return awake_;}

    /// Return true if the node is an immediate child of the given parent node
    public: bool IsChildOf (Node* parent) const {return parent_ == parent;}

    /// Return true if the node is a decendent of the given ancestor
    public: bool IsDescendantOf (Node* ancestor) const;

    /// Return true if the node is a viewport
    public: bool IsViewport (void) const {return !!(attr_ & NodeAttributes::Viewport);}

    /// Return true if the node is the scene root node
    public: bool IsSceneRoot (void) const {return !!(attr_ & NodeAttributes::SceneRoot); }

    public: bool IsWindowRoot (void) const {return !!(attr_ & NodeAttributes::WindowRoot); }

    public: bool IsApplicationRoot (void) const {return !!(attr_ & NodeAttributes::ApplicationRoot); }

    /// Return true if the node can be renamed in the editor
    public: bool IsRenameable (void) const {return !!(attr_ & NodeAttributes::AllowRename); }

    /// Returns the true visible state of the node.  A node may be marked visible
    /// but may not actually be visible due to a parents visibility state.
    public: bool IsVisible(void) const {return visible_;}
    
    public: virtual bool IsArrangeDependentOnChildren (void) const;

    public: bool IsMouseOver (void) const {return mouse_over_;}


    public: bool HasCapture(void) const;

    public: bool HasChildren (void) const {return GetChildCount()>0;}

    public: bool HasLogicalChildren (void) const {return GetLogicalChildCount()>0;}

    public: bool HasComponents (void) const {return components_.size() != 0;}

    public: bool HasTransform (void) const {return transform_ != nullptr;}

    /// Render the node and all children
    public: virtual void Render(RenderContext* rc);

    public: virtual bool Render(RenderContext* rc, Rect& render_rect);

    protected: virtual void RenderOverride (RenderContext* rc);
    
    /// Arrange the node using the cached arrange rectangle.
    public: void Arrange (void) {Arrange(cached_arrange_rect_);}

    /// Arrange the node using the given rectangle.  
    public: virtual void Arrange (const Rect& r);

    /// Arrange the node's children
    protected: virtual void ArrangeChildren (const Rect& r);

    /// Arrange the node by hiding it.
    public: void ArrangeHide (void);

    /// Measure the node using the cached available size
    public: Vector2 Measure (void) {return Measure(cached_available_size_);}

    /// Measure the node layout using the given available size if the node has a layout
    public: virtual Vector2 Measure (const Vector2& available_size);

    protected: virtual Vector2 MeasureChildren (const Vector2& available_size);

    protected: virtual void ApplyTransform(const Rect& r, Matrix3& mat);

    /// Invalidate the node transform marking it to be measured and arranged.
    public: void InvalidateTransform (void);


    public: void SetClipChildren (bool clip);

    /// Helper method to parent one node to another.
    public: void SetParent(Node* parent) {noz_assert(parent); parent->AddChild(this);}

    /// Set the visibility of the node
    public: void SetVisibility(Visibility v);

    public: void SetName (const Name& name);

    public: void SetCursor (Cursor* cursor);

    public: void SetPosition(const Vector2& position);

    public: virtual void SetAnimationState (const Name& name);

    public: void SetTransform (Transform* transform);

    public: void SetPrivate (bool v);

    public: void SetCapture (void);


    /// Return the child node at the given index.
    public: Node* GetChild (noz_uint32 index) const {return children_[index+private_child_count_];}

    /// Return the number of children in the node
    public: noz_uint32 GetChildCount (void) const {return (children_.size() - private_child_count_) * !logical_children_only_;}

    /// Returns the index of the node within its parent
    public: noz_int32 GetIndex (void) const {return (!private_ * index_) + (private_ * -1);}

    /// Returns the index of the node within its logical parent
    public: noz_int32 GetLogicalIndex (void) const {return logical_index_;}

    /// Returns the private index of the node within its parent.
    public: noz_int32 GetPrivateIndex (void) const {return ((!private_) * -1) + (private_ * index_);}


    public: const Vector2& GetMeasuredSize (void) const {return measured_size_;}

    public: Visibility GetVisibility (void) const {return visibility_;}

    public: const Name& GetName(void) const {return name_;}

    public: const Vector2& GetPosition(void) {CleanDirtyNodesInternal(); return position_;}

    public: Node* GetPrivateChild (noz_uint32 i) const {return children_[i];}

    public: noz_uint32 GetPrivateChildCount (void) const {return private_child_count_;}

    public: const Rect& GetRectangle(void) {CleanDirtyNodesInternal(); return rect_;}

    public: const Rect& GetRenderRectangle (void) const {return render_rect_;}

    public: Transform* GetTransform (void) const {return transform_;}

    public: template <typename T> T* GetTransform (void) const {return Cast<T>(transform_);}


    public: bool IsPrivate (void) const {return private_;}

    public: bool IsClippingChildren (void) const {return clip_children_;}

    /// Find a child node with the give name.  This method is recursive and returns
    /// the first child found with the given name.
    public: Node* FindChild (const String& name, bool recursive=false) {return FindChild(name.ToCString(), recursive);}
    public: Node* FindChild (const char* name, bool recursive=false);
    public: Node* FindChild (const Name& name, bool recursive=false);
    public: Node* FindChild (const NodePath& path, bool recursive=false);

    private: Node* FindChild (const NodePath& path, noz_int32 index, bool recursive);

    /// Update all invalid transforms
    public: static void UpdateTransforms (void);

    public: static void DestroyNodes (void);

    /// Return the local to viewport transform
    public: const Matrix3& GetLocalToViewport(void) {CleanDirtyNodesInternal(); return local_to_viewport_;}

    /// Return the world to local transform
    public: const Matrix3& GetViewportToLocal(void) {CleanDirtyNodesInternal(); return viewport_to_local_;}



    /// Transform the given vector from the node's local coordinate space to the
    /// parent viewport's coordinate space.
    public: Vector2 LocalToViewport (const Vector2& v) {return GetLocalToViewport() * v;}

    /// Transform the given vector from the node's local coordinate space to the
    /// parent window's coordinate space.
    public: Vector2 LocalToWindow (const Vector2& v);

    /// Transform the given vector from the node's local coordinate space to the
    /// parent screens's coordinate space.
    public: Vector2 LocalToScreen (const Vector2& v);

    
    public: Vector2 WindowToLocal (const Vector2& v);

    public: Vector2 ViewportToLocal (const Vector2& v) {return GetViewportToLocal() * v;}



    public: Rect ViewportToLocal (const Rect& v);

    public: Rect LocalToViewport (const Rect& r);

    public: Rect LocalToWindow (const Rect& r);
  


    /// Called to awaken the node into active state
    public: void Awaken (void);

    public: void ReleaseCapture (void);

    protected: void RemoveAllPrivateChildren (void);

    protected: void RemovePrivateChildAt (noz_uint32 i);
    
    /// Called by inline Get methods to check to see if there are any dirty transforms before 
    /// returning a transform dependent value.
    private: static inline void CleanDirtyNodesInternal(void) {if(dirty_state_==DirtyState::Dirty) UpdateTransforms();}

    private: void UpdateVisible (bool recursive=true);

    private: bool SerializeChildren (Serializer& s);

    private: bool DeserializeChildren (Deserializer& s);

    public: virtual void HandleMouseMoveEvent (SystemEvent* e);

    public: virtual void HandleMouseButtonEvent (SystemEvent* e);

    public: virtual HitTestResult HitTest (const Vector2& spos);

    public: static Node* HitTest (Node* root, const Vector2& spos);

    public: virtual void Update (void);;

    /// Called when the node is woken up for the first time.
    protected: virtual void OnAwake (void) {}

    public: virtual void OnPreviewKeyDown (SystemEvent* e) { }
    public: virtual void OnKeyDown (SystemEvent* e);

    public: virtual void OnKeyUp (SystemEvent* e);

    /// Called when the lineage of a node has changed in any way.  This method will
    /// be called after OnParentChanged and for each decendent node
    protected: virtual void OnLineageChanged (void) { }

    protected: virtual void OnPreviewMouseDown (SystemEvent* e);

    protected: virtual void OnMouseDown (SystemEvent* e);

    protected: virtual void OnMouseUp (SystemEvent* e);
    
    protected: virtual void OnMouseWheel (SystemEvent* e);

    /// Called when the mouse is first detected to be over the node.  The base 
    /// implementation forwards the event to all components as well.
    protected: virtual void OnMouseEnter (void);

    /// Called when the mouse leaves the node.  The base implementation forwards 
    /// the event to all components as well.
    protected: virtual void OnMouseLeave (void);

    /// Called every frame that the mouse is over this node
    protected: virtual void OnMouseOver (SystemEvent* e);

    /// Called when the immediate parent of a node changes
    protected: virtual void OnParentChanged (void) {}  

    protected: virtual void OnChildRemoved (Node* node) { }

    protected: virtual void OnChildAdded (Node* node) { }

    protected: void PropagateParentChange (Node* originator);

    protected: void SetLogicalChildrenOnly(void) {logical_children_only_=true;}
    

    public: noz_uint32 GetLogicalChildCount (void) const {return children_.size() - private_child_count_;}

    public: Node* GetLogicalChild (noz_uint32 i) const {return children_[private_child_count_+i];}


    private: noz_uint32 GetRealChildCount (void) const {return logical_children_only_ * private_child_count_ + children_.size() * !logical_children_only_;}

#if defined(NOZ_DEBUG)
    public: void PrintTree(noz_uint32 depth=0, bool private_nodes=false);
#endif
  };

  template <typename T> inline T* Node::AddComponent (void) {
    T* component = new T;
    AddComponent(component);
    return component;
  }

} // namespace noz

#include "Components/Transform/Transform.h"

#endif //__noz_Node_h__


