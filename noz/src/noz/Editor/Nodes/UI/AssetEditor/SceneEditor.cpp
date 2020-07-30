///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/System/Process.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/DropDownList.h>
#include <noz/Nodes/UI/DropDownListItem.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include "SceneEditor.h"

using namespace noz;
using namespace noz::Editor;


SceneEditor::SceneEditor(void) {  
  playing_process_ = nullptr;

  panning_ = false;
  zoom_= 1.0f;

  // Create a new inspector for scene editor.
  if(!Application::IsInitializing()) {
    // Create a new hierarchy for scene editor.
    if(nullptr==hierarchy_) {
      hierarchy_ = new Hierarchy;
      SetHierarchy(hierarchy_);
    }
  }
}

SceneEditor::~SceneEditor(void) {
  if(hierarchy_) hierarchy_->Destroy();
  if(inspector_) inspector_->Destroy();
}

bool SceneEditor::OnApplyStyle (void) {
  noz_assert(hierarchy_);

  if(!AssetEditor::OnApplyStyle()) return false;
  if(nullptr == scene_root_) return false;

  if(scene_) {
    hierarchy_->SetTarget(scene_->GetRootNode());;
    scene_root_->AddChild(scene_->GetRootNode());
  }

  hierarchy_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&SceneEditor::OnHeirarchySelectionChanged);

  if(resolution_) {
    resolution_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &SceneEditor::OnResolutionSelected);
    RefreshResolutions();
  }

  if(play_) play_->Click += ClickEventHandler::Delegate(this, &SceneEditor::OnPlay);

  return true;
}

bool SceneEditor::Load (AssetFile* file) {
  scene_ = AssetManager::LoadAsset<Scene>(file->GetGuid());
  return true;
}

void SceneEditor::OnHeirarchySelectionChanged (UINode* sender) {
  SetInspector(EditorFactory::CreateInspector(hierarchy_->GetSelected()));
    
  // Find the animator that controls the selected node.  The controlling animator is 
  // either the animator on the node itself or the first animator in the nodes ancestry.
  Animator* animator = nullptr;
  for(Node* node = hierarchy_->GetSelected(); !animator && node && node!=scene_root_; node = node->GetLogicalParent()) {
    animator = node->GetComponent<Animator>();
  }

  // Set the new animation view source to the controlling animator
  SetAnimationViewSource(animator);
}

void SceneEditor::RefreshResolutions (void) {
  if(nullptr == resolution_) return;

  for(noz_uint32 i=0,c=EditorSettings::GetResolutionCount(); i<c; i++) {
    DropDownListItem* r = new DropDownListItem;
    r->SetText(String::Format("%dx%d - %s", EditorSettings::GetResolution(i).GetWidth(), EditorSettings::GetResolution(i).GetHeight(), EditorSettings::GetResolution(i).GetName().ToCString()));
    r->SetUserData((void*)i);
    resolution_->AddChild(r);
  }

  resolution_->GetFirstChildItem()->Select();
}

void SceneEditor::OnResolutionSelected(UINode* sender) {  
  DropDownListItem* item = resolution_->GetSelectedItem();
  if(nullptr == item) return;
  noz_uint32 i = (noz_uint32)(noz_uint64)item->GetUserData();
  if(i>=EditorSettings::GetResolutionCount()) i=0;

  if(nullptr==scene_root_) return;
  
  LayoutTransform* t = Cast<LayoutTransform>(scene_root_->GetTransform());
  if(nullptr == t) {
    t = new LayoutTransform;
    t->SetMargin(LayoutLength(LayoutUnitType::Auto,0.0f));
    t->SetPivot(Vector2::Zero);
    scene_root_->SetTransform(t);
  }

  const Resolution& r = EditorSettings::GetResolution(i);
  t->SetWidth((noz_float)r.GetWidth());
  t->SetHeight((noz_float)r.GetHeight());
}

void SceneEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(GetFile()->GetPath(),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save scene");
    return;
  }

  JsonSerializer().Serialize(scene_,&fs);
  SetModified(false);
}

void SceneEditor::OnActivate (void) {
  AssetEditor::OnActivate();
}

void SceneEditor::OnPlay(UINode*) {
  if(playing_process_ != nullptr) return;

  play_->SetInteractive(false);

  String size;
  LayoutTransform* t = Cast<LayoutTransform>(scene_root_->GetTransform());
  if(nullptr != t) {
    size = String::Format("-width=%d -height=%d", (noz_int32)t->GetWidth().value_, (noz_int32)t->GetHeight().value_);
  }

  playing_process_ = Process::Start(
    Environment::GetExecutablePath().ToCString(),
    String::Format("-remote -scene=%s %s", scene_->GetGuid().ToString().ToCString(),size.ToCString()).ToCString()
  );
}

void SceneEditor::Update (void) {
  AssetEditor::Update();

  if(playing_process_ && playing_process_->HasExited()) {
    delete playing_process_;
    playing_process_ = nullptr;
    play_->SetInteractive(true);
  }
}

void SceneEditor::OnMouseDown (SystemEvent* e) {
  // Space being held starts a drag move
  if(Input::GetKey(Keys::Space)) {
    pan_start_ = e->GetPosition();
    panning_ = true;
    SetCapture();
    e->SetHandled();
    return;
  }
  

  Node* n = Node::HitTest(scene_->GetRootNode(), e->GetPosition());
  if(n) {
    hierarchy_->SetSelected(n);
  }
}

void SceneEditor::OnMouseOver (SystemEvent* e) {
  if(panning_) {
    grid_node_->SetOffset(grid_node_->GetOffset() + (e->GetPosition()-pan_start_));
    Vector2 pos = pan_node_->GetTransform()->GetLocalPosition();
    pos += (e->GetPosition()-pan_start_);
    pan_start_ = e->GetPosition();
    pan_node_->GetTransform()->SetLocalPosition(pos);
  }
}

void SceneEditor::OnMouseUp (SystemEvent* e) {
  panning_ = false;
}

void SceneEditorRootNode::RenderOverride (RenderContext* rc) {
  Node::RenderOverride(rc);

  SceneEditor* editor = GetAncestor<SceneEditor>();
  if(nullptr == editor) return;

  rc->PushMatrix();
  rc->MultiplyMatrix(GetLocalToViewport());
 
  for(noz_uint32 i=0, c=editor->hierarchy_->GetSelectedItemCount(); i<c; i++) {
    HierarchyItem* item = editor->hierarchy_->GetSelectedItem(i);
    Node* n = item->GetTarget();
    if(n->IsViewport() || n->IsSceneRoot()) continue;

    Rect r = n->LocalToWindow(n->GetRectangle());
    Vector2 tl = WindowToLocal(r.GetTopLeft());
    Vector2 br = WindowToLocal(r.GetBottomRight());
    Vector2 tr(br.x,tl.y);
    Vector2 bl(tl.x,br.y);
    rc->DrawDebugLine(tl,tr,Color::White);
    rc->DrawDebugLine(tr,br,Color::White);
    rc->DrawDebugLine(br,bl,Color::White);
    rc->DrawDebugLine(bl,tl,Color::White);
  }

  rc->PopMatrix();
}

void SceneEditor::OnMouseWheel (SystemEvent* e) {
  AssetEditor::OnMouseWheel(e);

  // Is the mouse over the grid?
  if(grid_node_ && grid_node_->HitTest(e->GetPosition()) == HitTestResult::Rect) {
    // If the mouse if over the node then zoom
    if(e->GetDelta().y > 0.0f) {
      SetZoom(zoom_ * 1.1f);
    } else {
      SetZoom(zoom_ * (1/1.1f));
    }
      
    e->SetHandled();
  }
}

void SceneEditor::SetZoom (noz_float zoom) {
  zoom = Math::Clamp(zoom,0.01f,40.0f);
  if(zoom == zoom_) return;
  zoom_ = zoom;
  zoom_node_->GetTransform()->SetScale(zoom_);
  grid_node_->SetScale(zoom_);
}
