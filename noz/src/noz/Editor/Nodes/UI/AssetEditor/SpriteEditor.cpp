///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/MeasureScaler.h>
#include <noz/Nodes/UI/ToggleButton.h>
#include <noz/Nodes/UI/ScrollView.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include <noz/Serialization/JsonSerializer.h>
#include "SpriteEditor.h"

using namespace noz;
using namespace noz::Editor;


SpriteEditor::SpriteEditor(void) {  
  zoom_ = 1.0f;

  if(!Application::IsInitializing()) {
    EditorApplication::PropertyChanged += PropertyChangedEventHandler::Delegate(this, &SpriteEditor::OnPropertyChanged);
  }
}

SpriteEditor::~SpriteEditor(void) {
}

bool SpriteEditor::Load (AssetFile* file) {
  sprite_ = AssetManager::LoadAsset<Sprite>(file->GetGuid());
  if(sprite_ && image_node_) image_node_->SetImage(sprite_->GetImage());
  SetInspector(EditorFactory::CreateInspector(sprite_));
  UpdateDimensions();
  UpdateManipulator();
  return true;
}

bool SpriteEditor::OnApplyStyle (void) {
  if(!AssetEditor::OnApplyStyle()) return false;
  if(nullptr == image_node_) return false;

  if(sprite_) image_node_->SetImage(sprite_->GetImage());

  if(zoom_drop_down_list_) {
    DropDownListItem* item;

    item = new DropDownListItem;
    item->SetText("20%");
    zoom_drop_down_list_->AddChild(item);

    item = new DropDownListItem;
    item->SetText("50%");
    zoom_drop_down_list_->AddChild(item);
   
    item = new DropDownListItem;
    item->SetText("70%");
    zoom_drop_down_list_->AddChild(item);

    item = new DropDownListItem;
    item->SetText("100%");
    zoom_drop_down_list_->AddChild(item);

    item = new DropDownListItem;
    item->SetText("150%");
    zoom_drop_down_list_->AddChild(item);

    item = new DropDownListItem;
    item->SetText("200%");
    zoom_drop_down_list_->AddChild(item);

    item = new DropDownListItem;
    item->SetText("400%");
    zoom_drop_down_list_->AddChild(item);
  }

  if(selection_tool_) {
    selection_tool_->SetChecked(true);
    selection_tool_->Click += ClickEventHandler::Delegate(this, &SpriteEditor::OnSelectToolClick);
  }

  if(magic_wand_tool_) {
    magic_wand_tool_->SetChecked(false);
    magic_wand_tool_->Click += ClickEventHandler::Delegate(this, &SpriteEditor::OnMagicWandToolClick);
  }

  if(manipulator_) {
    LayoutTransform* t = new LayoutTransform;    
    manipulator_->SetTransform(t);
  }

  UpdateDimensions();
  UpdateManipulator();

  return true;
}

void SpriteEditor::OnSelectToolClick (UINode* sender) {
  selection_tool_->SetChecked(true);
  magic_wand_tool_->SetChecked(false);
  zoom_node_->SetCursor(Cursors::Default);
}

void SpriteEditor::OnMagicWandToolClick (UINode* sender) {
  magic_wand_tool_->SetChecked(true);
  selection_tool_->SetChecked(false);
  zoom_node_->SetCursor(Cursors::Cross);
}

void SpriteEditor::SetZoom (noz_float zoom) {
  zoom = Math::Clamp(zoom,0.01f,40.0f);
  if(zoom == zoom_) return;
  zoom_ = zoom;
  zoom_node_->SetScale(zoom_);
  grid_node_->SetScale(zoom_);
}


void SpriteEditor::OnMouseWheel (SystemEvent* e) {
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

void SpriteEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(GetFile()->GetPath(),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save sprite");
    return;
  }

  JsonSerializer().Serialize(sprite_,&fs);
  fs.Close();

  AssetDatabase::ReloadAsset (GetFile()->GetGuid());
  SetModified(false);
}

void SpriteEditor::OnPropertyChanged (PropertyChangedEventArgs* args) {
  // Only care about properties within the font definition
  if(args->GetTarget() != sprite_) return;

  // Update the image if the image property changes.
  if(args->GetProperty()->GetName() == "Image") {
    if(sprite_ && image_node_) image_node_->SetImage(sprite_->GetImage());
    UpdateManipulator();
    UpdateDimensions();
    return;
  }

  if(args->GetProperty()->GetName() == "Rectangle") {
    UpdateManipulator();
    return;
  }
}

void SpriteEditor::UpdateDimensions(void) {
  if(sprite_ && sprite_->GetImage()) {  
    if(width_text_) {
      width_text_->SetVisibility(Visibility::Visible);
      width_text_->SetText(String::Format("%d", sprite_->GetImage()->GetWidth()));
    }
    if(height_text_) {
      height_text_->SetVisibility(Visibility::Visible);
      height_text_->SetText(String::Format("%d", sprite_->GetImage()->GetHeight()));
    }
  } else {
    if(width_text_) width_text_->SetVisibility(Visibility::Hidden);
    if(height_text_) height_text_->SetVisibility(Visibility::Hidden);
  }    
}



void SpriteEditor::MagicWand (const Vector2& uv) {
  struct Point { 
    noz_int32 x:12;
    noz_int32 y:12;
    noz_int32 d:2;
    Point (noz_int32 xx, noz_int32 yy, noz_int32 dd) {x=xx; y=yy; d=dd;}
  };

  Vector2 uv_min = uv;
  Vector2 uv_max = uv;

  noz_int32 x = (noz_int32)(sprite_->GetImage()->GetSize().x * uv.x);
  noz_int32 y = (noz_int32)(sprite_->GetImage()->GetSize().y * uv.y);
  noz_int32 d;

  noz_int32 id = sprite_->GetImage()->GetDepth();
  noz_int32 iw = sprite_->GetImage()->GetWidth();
  noz_int32 ih = sprite_->GetImage()->GetHeight();
  noz_int32 is = sprite_->GetImage()->GetStride();
  noz_float iwf = sprite_->GetImage()->GetSize().x;
  noz_float ihf = sprite_->GetImage()->GetSize().y;
  noz_float iwf_inv = 1.0f / sprite_->GetImage()->GetSize().x;
  noz_float ihf_inv = 1.0f / sprite_->GetImage()->GetSize().y;

  std::vector<Point> q;
  q.reserve(sprite_->GetImage()->GetWidth() * sprite_->GetImage()->GetHeight());
  q.push_back(Point(x,y,1));
  q.push_back(Point(x-1,y,-1));

  std::vector<noz_byte> visited;
  visited.resize(sprite_->GetImage()->GetWidth() * sprite_->GetImage()->GetHeight());
  memset(&visited[0], 0, visited.size());

  noz_byte* p = sprite_->GetImage()->Lock();

  while(!q.empty()) {
    Point& qq = q.back();
    x = qq.x;
    y = qq.y;
    d = qq.d;
    q.pop_back();

    // Validate x and y
    if(x<0 || y<0 || x>=iw || y>=ih) continue;

    if(visited[y*iw+x]) continue;
    visited[y*iw+x] = 1;

    // Look for zero alpha boundary
    if(p[y*is+x*id+3]==0) continue;

    noz_float xx = ((noz_float)x) * iwf_inv;
    noz_float yy = ((noz_float)y) * ihf_inv;
  
    uv_min.x = Math::Min(xx,uv_min.x);
    uv_max.x = Math::Max(xx,uv_max.x);
    uv_min.y = Math::Min(yy,uv_min.y);
    uv_max.y = Math::Max(yy,uv_max.y);    

    q.push_back(Point(x+d,y,d));
    q.push_back(Point(x,y+1,d));
    q.push_back(Point(x,y-1,d));
  }

  sprite_->GetImage()->Unlock();

  Rect r;
  r.x = uv_min.x * sprite_->GetImage()->GetSize().x;
  r.y = uv_min.y * sprite_->GetImage()->GetSize().y;
  r.w = (uv_max.x * sprite_->GetImage()->GetSize().x) - r.x + 1;
  r.h = (uv_max.y * sprite_->GetImage()->GetSize().y) - r.y + 1;
  sprite_->SetRectangle(r);

  UpdateManipulator();
  SetInspector(EditorFactory::CreateInspector(sprite_));
}

void SpriteEditor::OnMouseDown (SystemEvent* e) {
  AssetEditor::OnMouseDown(e);

  if(magic_wand_tool_ && magic_wand_tool_->IsChecked()) {
    if(image_node_->HitTest(e->GetPosition()) == HitTestResult::Rect) {
      Vector2 lpos = image_node_->WindowToLocal(e->GetPosition());
      Vector2 uv;
      uv.x = (lpos.x - image_node_->GetRectangle().x) / image_node_->GetRectangle().w;
      uv.y = (lpos.y - image_node_->GetRectangle().y) / image_node_->GetRectangle().h;
      MagicWand(uv);
      e->SetHandled();
    }
  }
}

void SpriteEditor::UpdateManipulator(void) {
  if(nullptr == manipulator_) return;
  if(nullptr == sprite_ || nullptr == sprite_->GetImage()) {
    manipulator_->SetVisibility(Visibility::Hidden);
    return;
  } 

  manipulator_->SetVisibility(Visibility::Visible);

  LayoutTransform* t = Cast<LayoutTransform>(manipulator_->GetTransform());
  noz_assert(t);

  const Rect& r = sprite_->GetRectangle();

  t->SetWidth(LayoutLength(LayoutUnitType::Percentage, 100.0f * (r.w/sprite_->GetImage()->GetSize().x)));
  t->SetHeight(LayoutLength(LayoutUnitType::Percentage, 100.0f * (r.h/sprite_->GetImage()->GetSize().y)));
  t->SetMarginLeft(LayoutLength(LayoutUnitType::Percentage, 100.0f * (r.x/sprite_->GetImage()->GetSize().x)));
  t->SetMarginTop(LayoutLength(LayoutUnitType::Percentage, 100.0f * (r.y/sprite_->GetImage()->GetSize().y)));
  t->SetMarginRight(LayoutLength(LayoutUnitType::Auto, 0.0f));
  t->SetMarginBottom(LayoutLength(LayoutUnitType::Auto, 0.0f));
}

void SpriteEditor::Update (void) {
  AssetEditor::Update();

  if(scroll_view_==nullptr) return;
  if(Input::GetKey(Keys::Space)) {
    scroll_view_->SetCursor(Cursors::Hand);
    scroll_view_->SetScrollMovementType(ScrollMovementType::Clamped);
  } else {
    scroll_view_->SetCursor(Cursors::Default);
    scroll_view_->SetScrollMovementType(ScrollMovementType::None);
  }
}

