///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Animation/Animator.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>

using namespace noz;

std::vector<ObjectPtr<Node>> Node::invalid_transforms_;
std::vector<ObjectPtr<Node>> Node::destroyed_nodes_;

/// Dirty state for all nodes.
Node::DirtyState Node::dirty_state_ = Node::DirtyState::Clean;;

#define ENABLE_COUNTERS   0

#if ENABLE_COUNTERS
#include <noz/Diagnostics/PerformanceCounter.h>
static PerformanceCounter _counter_update_transform ("Node::UpdateTransform");
static PerformanceCounter _counter_node_count ("Nodes");
static noz_uint64 _node_count = 0;
#endif


Node::Node(Name name, NodeAttributes attr) {
#if ENABLE_COUNTERS
  _node_count++;
#endif  

  opacity_ = 1.0f;
  private_child_count_ = 0;
  visibility_ = Visibility::Visible;
  visible_ = !!(attr & NodeAttributes::ApplicationRoot);
  auto_destroy_ = true;
  invalid_transform_ = false;
  awake_ = false;
  name_ = name;
  attr_ = attr;  
  arrange_visible_= true;
  mouse_over_ = false;
  render_culled_ = false;
  clip_children_ = false;
  orphan_ = true;
  private_ = false;

  viewport_ = nullptr;
  window_ = nullptr;
  parent_ = nullptr;

  index_ = -1;

  logical_children_only_ = false;
  logical_parent_ = nullptr;
  logical_index_ = -1;
}


Node::~Node(void) {
#if ENABLE_COUNTERS
  _node_count--;
#endif

  Orphan();

  // Free all components..
  for(noz_uint32 i=0,c=components_.size();i<c;i++) {
    delete components_[i];
  }
  components_.clear();

  // Put the node asleep
  awake_ = false;

  // Delete all children
  RemoveAllChildren();
  RemoveAllPrivateChildren();
}

void Node::Destroy(void) {
  Orphan();

  destroyed_nodes_.push_back(this);
}

void Node::DestroyImmediate(void) {
  delete this;
}

void Node::Orphan (bool remove_logical) {  
  // Handle unparented case
  if(nullptr==parent_ && nullptr==logical_parent_) return;

  // Remove logical if instructed or or the parent is both logical and visual
  remove_logical = remove_logical || parent_ == logical_parent_;

  // Ignore call to orphan if not orphaning from logical parent and there is non non logical parent
  if(!remove_logical && parent_==nullptr) return;

  // Release capture if the node has it
  if(HasCapture()) ReleaseCapture();

  Node* old_parent = parent_;
  Node* old_logical_parent = logical_parent_;

  // Mark the parents transform as invalid
  if(parent_) parent_->InvalidateTransform();

  // Private node
  if(private_) {  
    // Private children cannot be logical only children.
    noz_assert(logical_parent_ == parent_);
    noz_assert(parent_);

    // Remove from the parents children 
    parent_->children_.erase(parent_->children_.begin() + index_);

    // Reduce private child count.
    parent_->private_child_count_--;

    // Patch up the private indexes
    for(noz_uint32 i=index_;i<parent_->private_child_count_;i++) parent_->children_[i]->index_ = i;

  // Non private node
  } else {
    noz_assert(logical_parent_);

    // Remove from parent if attached to one
    if(parent_) {
      parent_->children_.erase(parent_->children_.begin() + parent_->private_child_count_ + index_);
      for(noz_uint32 i=index_+parent_->private_child_count_;i<parent_->children_.size();i++) {
        parent_->children_[i]->index_ = i - parent_->private_child_count_;
      }
    }

    // Removing logical?
    if(remove_logical) {
      // If parents are not the same then remove from the list,
      if(logical_parent_ != parent_) {
        logical_parent_->children_.erase(logical_parent_->children_.begin() + logical_parent_->private_child_count_ + logical_index_);
      }
      for(noz_uint32 i=logical_index_+logical_parent_->private_child_count_;i<logical_parent_->children_.size();i++) {
        logical_parent_->children_[i]->logical_index_ = i - logical_parent_->private_child_count_;
      }
    }
  }

  // Reset all parent related values
  parent_ = nullptr;
  index_ = -1;

  if(remove_logical) {
    logical_parent_ = nullptr;
    logical_index_ = -1;
  }
  
  if(old_parent) {
    PropagateParentChange(this);
    old_parent->OnChildRemoved(this);
  }

  if(remove_logical && old_logical_parent && old_logical_parent != old_parent) old_logical_parent->OnChildRemoved(this);
}

void Node::AddPrivateChild (Node* child) {
  child->SetPrivate(true);
  AddChild(child);
}

void Node::AddChild(Node* child) {
  if(child->private_ ) {
    InsertChild(private_child_count_,child);
  } else {
    InsertChild(GetLogicalChildCount(),child);
  }
}

void Node::InsertChild (noz_uint32 i, Node* child) {
  noz_assert(child);
  noz_assert(child != this);

  // Check for adding the child to the same parent
  if(!child->private_ && logical_children_only_ ) {
    if(child->logical_parent_==this) return;
  } else if(this == child->parent_) {
    return;
  }

  // If the child already has a parent or this node is a logical children container
  // then orphan the child to ensure it is ready to be added
  if(child->IsPrivate() || child->parent_ || logical_children_only_)  child->Orphan();

  // Handle private and non private nodes
  if(child->IsPrivate()) {
    i = Math::Max(i,private_child_count_);

    // Insert the child into the private list
    children_.insert(children_.begin()+i, child);

    private_child_count_++;

    child->index_ = i;
    child->parent_ = this;
    child->logical_index_ = i;
    child->logical_parent_ = this;

    // Patch private indicies.
    for(noz_uint32 ii=i+1, c=private_child_count_; ii<c; ii++) children_[ii]->index_ = ii;

  // Insert into the middle of the non private child list
  } else {
    // A node's children either need to be all logical nodes, all visual nodes, or all both.  This assert 
    // ensures that a node being added meets those constraints.
    noz_assert(GetChildCount()==0||GetChild(0)->logical_parent_==child->logical_parent_||child->logical_parent_==nullptr);

    if(i>=GetLogicalChildCount()) {
      i = (noz_uint32)children_.size() - private_child_count_;
      children_.push_back(child);
    } else {
      children_.insert(children_.begin()+private_child_count_+i, child);

      // Patch the index
      if(!logical_children_only_) {
        for(noz_uint32 ii=private_child_count_+i+1, c=children_.size(); ii<c; ii++) children_[ii]->index_ = ii - private_child_count_;
      }

      // Patch the logical index
      if(child->logical_parent_ == nullptr || logical_children_only_) {
        for(noz_uint32 ii=private_child_count_+i+1, c=children_.size(); ii<c; ii++) children_[ii]->logical_index_ = ii - private_child_count_;
      }
    }

    // Associate the child with this node as parent
    if(!logical_children_only_) {
      child->parent_ = this;
      child->index_ = i;
    }

    noz_assert(child->logical_parent_==nullptr || child->logical_parent_->logical_children_only_);

    // If the child node has no logical parent yet or this node is a logical child container
    // then set the logical parent as well.
    if(child->logical_parent_ == nullptr || logical_children_only_) {
      noz_assert(child->logical_index_-1);
      child->logical_parent_ = this;
      child->logical_index_ = i;
    }
  }

  if(!logical_children_only_ || child->IsPrivate())  {
    InvalidateTransform();
    child->PropagateParentChange(child);
  }

#if defined(NOZ_DEBUG)
  noz_assert(child->logical_parent_);
  if(!child->IsPrivate()) {
    if(child->parent_ == this) {
      for(noz_uint32 ii=0,cc=GetChildCount(); ii<cc; ii++) {
        Node* n = GetChild(ii);
        noz_assert(n->index_ == ii);
      }
    }

    if(child->logical_parent_ == this) {
      for(noz_uint32 ii=0,cc=GetLogicalChildCount(); ii<cc; ii++) {
        Node* n = GetLogicalChild(ii);
        noz_assert(n->logical_index_ == ii);
      }
    }
  }
#endif

  if(!child->IsPrivate()) OnChildAdded(child);
}

Node* Node::ReleaseChildAt (noz_uint32 i) {
  Node* child = children_[i+private_child_count_];
  child->Orphan();
  return child;
}

void Node::RemoveChildAt (noz_uint32 i) {
  Node* child = children_[i+private_child_count_];
  child->Orphan();
  child->Destroy();  
}

void Node::RemovePrivateChildAt (noz_uint32 i) {
  Node* child = children_[i];
  child->Orphan();
  child->Destroy();  
}

void Node::RemoveAllChildren(void) {
  for(noz_uint32 i=children_.size();i>private_child_count_;i--) RemoveChildAt(children_.size()-1-private_child_count_);
}

void Node::RemoveAllPrivateChildren (void) {
  for(noz_uint32 i=private_child_count_;i>0;i--) RemovePrivateChildAt(private_child_count_-1);
}

void Node::SetClipChildren (bool clip) {
  clip_children_ = clip;
}

void Node::SetAnimationState (const Name& name) {
  // Set the state in all animator components
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) {
    if(components_[i]->IsTypeOf(typeof(Animator))) ((Animator*)(Component*)components_[i])->SetState(name);
  }
}

void Node::SetTransform (Transform* transform) {
  if(transform==transform_) return;
  if(transform_) {
    delete transform_;
  }
  transform_ = transform;
  transform_->node_ = this;
  InvalidateTransform();
}

void Node::SetChildCapacity(noz_uint32 capacity) {
  children_.reserve(capacity);
}

void Node::SetCapture (void) {
  Application::SetCapture(this);
}

void Node::ReleaseCapture(void) {
  if(Application::GetCapture() == this) Application::SetCapture(nullptr);
}

bool Node::HasCapture(void) const {
  return Application::GetCapture() == this;
}

void Node::SetPrivate (bool v) {
  if(IsPrivate() == v) return;

  Node* old_parent = logical_parent_;

  Orphan();

  private_ = v;

  if(old_parent) old_parent->AddChild(this);
}

void Node::SetVisibility (Visibility v) {
  if(visibility_ == v) return;
  visibility_ = v;
  InvalidateTransform();

  // Update visible state..
  UpdateVisible();
}

void Node::Render(RenderContext* rc) {  
  // Do not render if hidden
  if(!IsVisible()) return;

  // Push a new empty state to the to of the state stack.
  if(rc) {
    rc->PushState();
    if(opacity_ != 1.0f) rc->PushOpacity(opacity_);
  }

  // Render self
  render_culled_ = !Render( rc, render_rect_);

  // Calculate the render rectangle for the node.
  render_rect_ = LocalToWindow(rect_);

  if(clip_children_) {
    rc->PushMatrix();
    rc->MultiplyMatrix(GetLocalToViewport());  
    rc->PushMask(GetRectangle(),nullptr,render_rect_);  
    rc->PopMatrix();
    render_rect_ = LocalToWindow(rect_);
  }

  // Render all children
  RenderOverride(rc);

  if(clip_children_) { 
    rc->PopMask();
  } else {
    // Combine all of the childrens render structure to ours.
    for(noz_uint32 i=0,c=GetRealChildCount();i<c;i++) {
      Node* child = children_[i];
      if(child->render_culled_) continue;
      if(render_culled_) {
        render_culled_ = false;
        render_rect_ = child->render_rect_;
      } else {
        render_rect_ = render_rect_.Union(child->render_rect_);
      }
    }
  }

  // Clear any state our children may have left behind such as masks, etc
  if(rc) {
    if(opacity_!=1.0f) rc->PopOpacity();
    rc->PopState();
  }
}

bool Node::Render (RenderContext* rc, Rect& render_rect) {
  return true;
}

void Node::RenderOverride (RenderContext* rc) {
  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) children_[i]->Render(rc);
}

bool Node::IsDescendantOf (Node* parent) const { 
  for(Node* p=GetParent(); p; p=p->GetParent()) {
    if(p==parent) return true;
  }
  return false;
}

Scene* Node::GetScene(void) const {
  return IsViewport() ? ((Viewport*)this)->scene_ : nullptr;
}

Node* Node::GetAncestor(Type* type) const {
  for(Node* p=GetParent(); p; p=p->GetParent()) {
    if(p->IsTypeOf(type)) return p;
  }
  return nullptr;
}

Node* Node::GetNextSibling (void) const {
  if (nullptr==parent_) return nullptr;
  if(index_+1 >= (noz_int32)(parent_->children_.size()-parent_->private_child_count_)) return nullptr;
  return parent_->children_[index_+1+parent_->private_child_count_];
}

Node* Node::GetPrevSibling (void) const {
  if(nullptr==parent_) return nullptr;
  if(index_==0) return nullptr;
  return parent_->children_[index_-1+parent_->private_child_count_];
}

Node* Node::GetNextLogicalSibling (void) const {
  if (nullptr==logical_parent_) return nullptr;
  if(logical_index_ + 1 >= (noz_int32)(logical_parent_->children_.size()-logical_parent_->private_child_count_)) return nullptr;
  return logical_parent_->children_[logical_index_+1+logical_parent_->private_child_count_];
}

Node* Node::GetPrevLogicalSibling (void) const {
  if(nullptr==logical_parent_) return nullptr;
  if(logical_index_==0) return nullptr;
  return logical_parent_->children_[logical_index_-1+logical_parent_->private_child_count_];
}

void Node::SetPosition(const Vector2& position) {
  NOZ_FIXME()
//  scale_ = scale;
//  UpdateTransforms();
}

Node* Node::FindChild (const char* name, bool recursive) {
  // Create a node path since we dont know if its a name or a path that was given.
  NodePath np(name);

  // If it was just a name and not a path then just find using the name instead
  if(np.IsRelative() && np.GetLength()==1) {
    return FindChild(np[0],recursive);
  }

  // Find using the path
  return FindChild(np,recursive);
}

Node* Node::FindChild (const Name& name, bool recursive) {
  // Check this node first..
  if(name==GetName()) return this;

  Node* result = nullptr;
  if(recursive) {
    for(noz_uint32 i=0,c=GetRealChildCount();!result && i<c;i++) {
      result = children_[i]->FindChild(name,true);
    }
  } else {
    for(noz_uint32 i=0,c=GetRealChildCount();!result && i<c;i++) {
      if(children_[i]->GetName()==name) result = children_[i];
    }
  }

  return result;
}

Node* Node::FindChild (const NodePath& path, bool recursive) {
  if(path.GetLength()==0) return nullptr;
  return FindChild(path,0,recursive);
}

Node* Node::FindChild (const NodePath& path, noz_int32 index, bool recursive) {
  Node* n = FindChild(path[index], recursive && index==0 && (path.IsRelative() || index==0));
  if(n==nullptr) {
    return nullptr;
  }

  if(index + 1 >= (noz_int32)path.GetLength()) {
    return n;
  }

  return FindChild(path,index+1,recursive);
}

void Node::Awaken (void) {
  // Ensure we are not already awake
  if(awake_) return;

  // Flag as awake
  awake_ = true;

  // Awaken all components
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnAwake();

  // Awaken all children
  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) children_[i]->Awaken();

  // Now that the node is an active member of society its transform must be updated
  InvalidateTransform();
}

Vector2 Node::LocalToWindow (const Vector2& v) {    
  if(nullptr==viewport_ || viewport_==this) return v;
  return viewport_->viewport_to_window_ * LocalToViewport(v);
}

Vector2 Node::LocalToScreen (const Vector2& v) {  
  if(nullptr == window_) return v;
  return window_->LocalToScreen(LocalToWindow(v));
}

Vector2 Node::WindowToLocal(const Vector2& v) {
  if(nullptr==viewport_) return v;
  return ViewportToLocal(viewport_->window_to_viewport_ * v);
}

Rect Node::LocalToViewport (const Rect& r) {
  Vector2 wmin = LocalToViewport(r.GetMin());
  Vector2 wmax = LocalToViewport(r.GetMax());
  Rect result;
  result.x = Math::Min(wmin.x,wmax.x);
  result.y = Math::Min(wmin.y,wmax.y);
  result.w = Math::Max(wmin.x,wmax.x) - result.x;
  result.h = Math::Max(wmin.y,wmax.y) - result.y;
  return result;
}

Rect Node::LocalToWindow(const Rect& r) {
  Vector2 wmin = LocalToWindow(r.GetMin());
  Vector2 wmax = LocalToWindow(r.GetMax());
  Rect result;
  result.x = Math::Min(wmin.x,wmax.x);
  result.y = Math::Min(wmin.y,wmax.y);
  result.w = Math::Max(wmin.x,wmax.x) - result.x;
  result.h = Math::Max(wmin.y,wmax.y) - result.y;
  return result;
}

Rect Node::ViewportToLocal (const Rect& r) {
  Vector2 wmin = ViewportToLocal(r.GetMin());
  Vector2 wmax = ViewportToLocal(r.GetMax());
  Rect result;
  result.x = Math::Min(wmin.x,wmax.x);
  result.y = Math::Min(wmin.y,wmax.y);
  result.w = Math::Max(wmin.x,wmax.x) - result.x;
  result.h = Math::Max(wmin.y,wmax.y) - result.y;
  return result;
}

bool Node::IsArrangeDependentOnChildren (void) const {
  if(transform_) return transform_->IsDependentOnMeasure();

  // Default implementation of node arranges all children the size rather than relative to 
  // each other in any way.  Therefore the node's arrange is not dependent on its children.
  return false;
}

void Node::InvalidateTransform (void) {
  // Ignore calls to invalidate if the node is an orphan.  This is because when the
  // not eventually becomes adopted by the node tree the root node which was adopted
  // will be invalidated and thus the entire tree will be.
  if(!IsApplicationRoot() && !IsWindowRoot() && !IsSceneRoot() && !IsViewport() && IsOrphan()) {
    return;
  }

  // Set dirty state 
  if(dirty_state_==DirtyState::Clean) dirty_state_ = DirtyState::Dirty;

  // If already marked dirty and not currently cleaning then
  // just early out since it is already in the list.  If we are cleaning
  // then something invalidated the transform during the clean so to be 
  // sure it is processed it must be readded
  if(dirty_state_!=DirtyState::Cleaning && invalid_transform_) return;

  if(invalid_transform_) return;

  Node* invalid_node = this;
  for(Node* pp=parent_; pp; pp=pp->parent_) {
    // If arrange is dependent on children then this node can become the new invalid node
    if(pp->IsArrangeDependentOnChildren()) invalid_node = pp;
  }

  // Add the invalid node to the invalid transforms list
  invalid_node->invalid_transform_ = true;
  invalid_transforms_.push_back(invalid_node);
}

void Node::ArrangeHide (void) {
  if(arrange_visible_ == false) return;
  arrange_visible_ = false;
  UpdateVisible();
}


void Node::Arrange (const Rect& r) {
#if ENABLE_COUNTERS
  _counter_update_transform.Increment();
#endif

  if(visibility_ == Visibility::Collapsed) return;

  // If the arrangement was previously hidden then unhide it and update visibility
  if(arrange_visible_==false) {
    arrange_visible_ = true;
    UpdateVisible();
  }

  // Save the arrange rect 
  cached_arrange_rect_ = r;

  if(transform_) {
    rect_ = transform_->Update(r,measured_size_);
  } else {
    rect_ = r;
  }

  // If the node has a transform apply it now.
  Matrix3 mat;
  mat.identity();
  ApplyTransform(rect_,mat);
  
  // Apply the parent transform..
  Node* parent = GetParent();
  if(nullptr==parent) {
    local_to_viewport_ = mat;
  } else if (parent->IsViewport()) {
    local_to_viewport_ = mat;
  } else {
    local_to_viewport_ = mat * parent->GetLocalToViewport();
  }

  // Calculate the world position
  position_ = ViewportToLocal (Vector2());
  
  // Invert the matrix for world to local
  // TODO: this could wait till someone requests it.
  viewport_to_local_ = local_to_viewport_;
  viewport_to_local_.invert();

  Rect child_rect = rect_;
  if(transform_) child_rect = transform_->AdjustArrangeRect(rect_);

  // Arrange all children
  ArrangeChildren(child_rect);
}

void Node::ArrangeChildren(const Rect& r) {
  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) children_[i]->Arrange(r);
}

void Node::ApplyTransform(const Rect& r, Matrix3& mat) {
  if(nullptr != transform_) transform_->Apply(mat);
}

Vector2 Node::Measure (const Vector2& available_size) {
  // Cache available size to allow node to be remeasured without its parents being remeasured
  cached_available_size_ = available_size;

  Vector2 a = available_size;

  // When collapsed just return an empty vector
  if(visibility_==Visibility::Collapsed) return Vector2::Empty;

  // Allow the transform to adjust the available size to account for things such as margins.
  if(transform_) a = transform_->AdjustAvailableSize(a);

  // Reset measured size.
  measured_size_.clear();

  // Measure all children
  measured_size_ = Math::Max(measured_size_,MeasureChildren(a));

  // Final adjustments to measured size.  This could be adding margins, fixed width/height, etc.
  if(transform_) measured_size_ = transform_->AdjustMeasuredSize (measured_size_);

  // Return total measured size.
  return measured_size_;
}

Vector2 Node::MeasureChildren(const Vector2& a) {
  Vector2 size;
  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) {
    size = Math::Max(size, children_[i]->Measure(a));
  }
  return size;
}

void Node::UpdateTransforms (void) {
  if(dirty_state_ == DirtyState::Clean) return;

#if ENABLE_COUNTERS
  _counter_node_count.Clear();
  _counter_node_count.IncrementBy(_node_count);
#endif

  // Set state to cleaning
  dirty_state_ = DirtyState::Cleaning;

  noz_uint32 s;
  noz_uint32 e;
  noz_uint32 i;

  // Continually clean until nothing is dirty
  for(s=0, e=invalid_transforms_.size(); s<e; e=invalid_transforms_.size()) {
    // Optimize the dirty nodes by removing any nodes that have parents that are already in the list.
    for(i=s; i<e; ) {
      Node* n = invalid_transforms_[i];
      if(nullptr==n) {
        invalid_transforms_[i]=invalid_transforms_.back();
        invalid_transforms_.pop_back();
        e--;
        continue;
      }

      Node* p = n;
      for(Node* pp=n->parent_; pp; pp=pp->parent_) {
        if(pp->invalid_transform_) {
          p->invalid_transform_=false; 
          p=pp;
        }
      }
      if(p!=n) {      
        invalid_transforms_[i]=invalid_transforms_.back();
        invalid_transforms_.pop_back();
        e--;
      } else {
        i++;
      }
    }

    for(; s<e; s++) {
      Node* n = invalid_transforms_[s];

      // If a node was deleted after being put in the dirty list then a nullptr will be in list
      if(nullptr == n) continue;

      // Skip if transform is no longer invalid. This generally happens because the transform
      // was a child of another node that was already updated.
      if(!n->invalid_transform_) continue;

      // Mark the transform as valid before we measure and arrange.  This is important to ensure
      // that if measure or arrange invalidates again that this node is considered valid so it 
      // can be entered back into the dirty list.
      n->invalid_transform_ = false;

      // Measure the node using the nodes cached values
      n->Measure();
      
      // Arrange using cached values
      n->Arrange();

      // If measure or arrange invalidated a node then break this loop and 
      // let the outer loop handle it.
      if(e != invalid_transforms_.size()) break;
    }
  }

  invalid_transforms_.clear();
  dirty_state_ = DirtyState::Clean;
}

#if defined(NOZ_DEBUG)
void Node::PrintTree(noz_uint32 depth, bool private_nodes) {
  StringBuilder sb;
  sb.Append(String::Format("[%08x] : ", this));
  for(noz_uint32 i=0;i<depth;i++) sb.Append("  ");
  sb.Append(String::Format("%s%s", IsPrivate() ? "* " : "- ", GetType()->GetName().ToCString()));
  if(!GetName().IsEmpty()) sb.Append(String::Format(" \"%s\"", GetName().ToCString()));

  sb.Append(String::Format(" [%s%s%s%s]", 
    viewport_!=nullptr? "V" : "", 
    IsAwake()?"a":"", 
    parent_?"p":"", 
    visible_?"v":""));

  sb.Append(String::Format(" (%g,%g,%g,%g) (%g,%g)", GetRectangle().x, GetRectangle().y, GetRectangle().w, GetRectangle().h, measured_size_.x, measured_size_.y));

  if(HasTransform() || HasComponents()) {
    sb.Append(" :");
    if(HasTransform()) {
      sb.Append(" <");
      sb.Append(transform_->GetType()->GetName().ToCString());
      sb.Append(">");
    }
    if(HasComponents()) {
      for(noz_uint32 i=0,c=(noz_uint32)components_.size();i<c;i++) {
        sb.Append(" <");
        sb.Append(components_[i]->GetType()->GetName().ToCString());
        sb.Append(">");
      }
    }
  }

  Console::WriteLine(sb.ToString().ToCString());

  if(private_nodes) {
    for(noz_uint32 i=0,c=GetRealChildCount();i<c;i++) children_[i]->PrintTree(depth+1,private_nodes);
  } else {
    for(noz_uint32 i=private_child_count_,c=children_.size();i<c;i++) children_[i]->PrintTree(depth+1);
  }
}
#endif

void Node::UpdateVisible(bool recursive) {
  // Calculate new visible state.
  bool visible = arrange_visible_ && GetParent() && GetParent()->IsVisible() && visibility_==Visibility::Visible;

  // If visible state has not changed then early out
  if(visible_ == visible) return; 

  // Set new visible state
  visible_ = visible;

  // Propegate to all children
  if(recursive) for(noz_uint32 i=0,c=GetRealChildCount();i<c;i++) children_[i]->UpdateVisible();
}

void Node::HandleMouseButtonEvent (SystemEvent* e) {
  // Ensure the mouse over the node or the node has capture before continuing.
  if(!mouse_over_ && !HasCapture()) return;

  // Allow the node to preview the mouse button first
  OnPreviewMouseDown(e);

  // If handled stop the message from continuing up the child list further.
  if(!e->IsHandled()) {
    // Walk tree backwards but tunnel upwards..
    for(noz_uint32 i=GetRealChildCount(); i>0; i--) {
      children_[i-1]->HandleMouseButtonEvent(e);    

      if(e->IsHandled()) return;
    }
  }

  // Call the appropriate button handler method
  switch(e->GetEventType()) {
    case SystemEventType::MouseDown: OnMouseDown(e); break;
    case SystemEventType::MouseUp: OnMouseUp(e); break;
    case SystemEventType::MouseWheel: OnMouseWheel(e); break;
    
    default: break;
  }
}

void Node::HandleMouseMoveEvent (SystemEvent* e) {
  HitTestResult hit = HitTestResult::None;

  if(!e->IsHandled() && 
     (HasCapture() || Application::GetCapture()==nullptr)) {
     hit = HitTest(e->GetPosition());     
  }

  // Walk tree backwards but tunnel upwards..
  bool child_hit = false;
  for(noz_uint32 i=GetRealChildCount(); i>0; i--) {
    children_[i-1]->HandleMouseMoveEvent(e);    
    child_hit |= children_[i-1]->mouse_over_;
  }

  if(hit == HitTestResult::Rect || e->IsHandled() || child_hit) {
    if(!mouse_over_) {
      mouse_over_ = true;
      OnMouseEnter();
    }
  } else if(mouse_over_) {
    mouse_over_ = false;
    OnMouseLeave();
  }

  if(hit == HitTestResult::Rect || e->IsHandled() || HasCapture()) {
    OnMouseOver(e);
  }
}

void Node::OnKeyDown(SystemEvent* e) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnKeyDown(e);
}

void Node::OnKeyUp(SystemEvent* e) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnKeyUp(e);
}

void Node::OnPreviewMouseDown(SystemEvent* e) {
  //for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnPreviewMouseDown(e);
}

void Node::OnMouseDown(SystemEvent* e) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnMouseDown(e);
}

void Node::OnMouseUp(SystemEvent* e) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnMouseUp(e);
}

void Node::OnMouseWheel(SystemEvent* e) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnMouseWheel(e);
}

void Node::OnMouseOver(SystemEvent* e) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnMouseOver(e);
}

void Node::OnMouseEnter(void) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnMouseEnter();
}

void Node::OnMouseLeave(void) {
  for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnMouseLeave();
}

Node* Node::HitTest (Node* root, const Vector2& spos) {
  noz_assert(root);

  HitTestResult hit_result = root->HitTest(spos);
  if(hit_result == HitTestResult::None) return nullptr;

  for(noz_uint32 i=root->GetRealChildCount(); i>0; i--) {
    Node* result = HitTest(root->children_[i-1], spos);
    if(nullptr != result) return result;
  }

  if(hit_result == HitTestResult::Rect) return root;

  return nullptr;
}

HitTestResult Node::HitTest (const Vector2& spos) {
  // Cannot be hit if not visible..
  if(!IsVisible()) return HitTestResult::None;

  // If the node was not actually rendered on screen then we are done
  if(render_culled_) return HitTestResult::None;

  // Ensure that the position is within the render rectangle
  if(!render_rect_.Contains(spos)) return HitTestResult::None;

  // Convert screen coordinate to a local coordinate
  Vector2 lpos = WindowToLocal(spos);

  // Test local coordinate and if it fails just return the render rect hit
  if(!rect_.Contains(lpos)) return HitTestResult::RenderRect;
 
  return HitTestResult::Rect;
}

void Node::PropagateParentChange (Node* originator) {
  if(!(attr_ & NodeAttributes::WindowRoot)) {
    if(parent_) {
      window_ = parent_->window_;
    } else {
      window_ = nullptr;
    }
  }

  // Propegate the viewport from parent
  if(!(attr_ & NodeAttributes::Viewport)) {
    if(parent_) {
      viewport_ = parent_->viewport_;
    } else {
      viewport_ = nullptr;
    }
  }

  // Set orphan state
  orphan_ = (nullptr==viewport_ || nullptr==viewport_->GetScene());

  // Update the visible state (non recursively)
  UpdateVisible(false);

  // Awaken the node if not awake
  if(parent_ && parent_->awake_ && !awake_) {
    awake_ = true;
    OnAwake();

    // Awaken all components as well.
    for(noz_uint32 i=0,c=components_.size(); i<c; i++) components_[i]->OnAwake();

    InvalidateTransform();
  }
  
  if(originator == this) {
    // Inform the node that its parent changed.
    OnParentChanged();
  }

  OnLineageChanged();

  // Recurse through the children
  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) children_[i]->PropagateParentChange(originator);
}

bool Node::SerializeChildren (Serializer& s) {
  noz_uint32 public_size = children_.size()-private_child_count_;
  s.WriteStartSizedArray(public_size);
  if(public_size) {
    for(noz_uint32 i=private_child_count_,c=children_.size();i<c;i++) {
      s.WriteValueObject(children_[i]);
    }
  }
  s.WriteEndArray();
  return true;
}

bool Node::DeserializeChildren (Deserializer& s) {
  noz_uint32 size = 0;
  if(!s.ReadStartSizedArray(size)) return false;
  if(size) {
    children_.reserve(private_child_count_ + size);
    for(noz_uint32 i=0; i<size; i++) {
      Object* o = nullptr;
      if(!s.ReadValueObject(o,typeof(Node))) return false;
      if(o) {
        AddChild ((Node*)o);
      }
    }
  }
  return s.ReadEndArray();
}

Component* Node::GetComponent (Type* t) const {
  if(t->IsCastableTo(typeof(Transform))) return transform_;

  for(noz_uint32 i=0,c=components_.size();i<c;i++) {
    if(components_[i]->IsTypeOf(t)) return components_[i];
  }
  return nullptr;
}

void Node::ReleaseComponent (Component* component) {
  if(component->GetNode() != this) return;

  if(component == transform_) {
    Transform* t = transform_;
    transform_ = nullptr;
    t->node_ = nullptr;
    t->OnDetach(this);
    return;
  }

  for(noz_uint32 i=0,c=components_.size(); i<c; i++) {
    if(components_[i] == component) {
      components_.erase(components_.begin()+i);
      component->node_ = nullptr;
      component->OnDetach(this);
      InvalidateTransform();
      return;
    }
  }
}     

void Node::AddComponent(Component* component) {
  noz_assert(component);
  noz_assert(component->GetNode()==nullptr);

  // Allow transforms to be added via AddComponent but they will still
  // be redirected to the Transform member
  if(component->IsTypeOf(typeof(Transform))) {
    SetTransform((Transform*)component);
    return;
  }

  // Add the component to the variable components list
  components_.push_back(component);

  // Link the component's node to ourself
  component->node_ = this;

  // Awaken the component if the node is already awake.
  if(awake_) component->OnAwake();
}

void Node::Animate (void) {
  NOZ_TODO("use global list and register for animation");

  for(noz_uint32 i=0,c=components_.size(); i<c; i++) {
    if(components_[i]->IsTypeOf(typeof(Animator))) ((Animator*)(Component*)components_[i])->Animate();
  }

  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) children_[i]->Animate();
}

void Node::Update(void) {
  for(noz_uint i=0,c=components_.size(); i<c; i++) components_[i]->Update();

  for(noz_uint32 i=0,c=GetRealChildCount(); i<c; i++) children_[i]->Update();  
}



NodeManager* NodeManager::this_ = nullptr;

NodeManager::NodeManager(void) {
  invalid_ = true;
  nodes_.reserve(1024);
}

void NodeManager::Initialize (void) {
  if(nullptr!=this_) return;

  this_ = new NodeManager;
}

void NodeManager::Uninitialize (void) {
  if(nullptr==this_) return;

  delete this_;
  this_ = nullptr;
}


void NodeManager::Invalidate(void) {
  if(this_==nullptr) return;
  this_->invalid_ = true;
}

void NodeManager::Update (void) {
  if(!this_->invalid_) return;

  // Erase all components in the existing vector
  this_->nodes_.clear();

  // Update the components list depth first
  this_->Update (Application::GetRootNode());
}

void NodeManager::Update (Node* n) {
/*
  nodes_.push_back(n);

  // Add all nodes within components
  for(auto it=n->GetComponents().begin(); it!=n->GetComponents().end(); it++) {
    INodeProvider* provider = (*it)->GetChildren();
    if(nullptr==provider) continue;
    for(noz_uint32 c=0;c<provider->GetChildCount(); c++) {
      Update(provider->GetChild(c));
    }
  }

  // Add all direct children
  for(noz_uint32 c=0;c<n->GetChildren()->GetChildCount(); c++) Update(n->GetChildren()->GetChild(c));
*/
}


void Node::DestroyNodes (void) {
  for(noz_uint32 i=0;i<destroyed_nodes_.size();i++) {
    Node* n = destroyed_nodes_[i];
    if(n) delete n;
  }
  destroyed_nodes_.clear();
}

void Node::SortChildren (noz_int32 (*sort_proc) (const Node* node1, const Node* node2)) {
#if defined(NOZ_WINDOWS)
  std::sort(children_.begin()+private_child_count_, children_.end(), [sort_proc] (const ObjectPtr<Node>& lhs, const ObjectPtr<Node>& rhs) {
    return sort_proc((Node*)lhs, (Node*)rhs) < 0;
  });  
  if(logical_children_only_) {
    for(noz_uint32 i=private_child_count_;i<children_.size();i++) children_[i]->logical_index_ = i-private_child_count_;
  } else {
    for(noz_uint32 i=private_child_count_;i<children_.size();i++) children_[i]->index_ = i-private_child_count_;
  }
#endif
}

void Node::SetName (const Name& name) {
  if(name_ == name) return;
  name_ = name;
}
