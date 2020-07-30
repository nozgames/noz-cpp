///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/MeasureScaler.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "FontEditor.h"

using namespace noz;
using namespace noz::Editor;


FontEditor::FontEditor(void) {  
  zoom_ = 1.0f;

  if(!Application::IsInitializing()) {
    EditorApplication::PropertyChanged += PropertyChangedEventHandler::Delegate(this, &FontEditor::OnPropertyChanged);
  }
}

FontEditor::~FontEditor(void) {
  if(font_ && !font_->IsManaged()) delete font_; 
  if(inspector_) inspector_->Destroy();
}

bool FontEditor::Load (AssetFile* file) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(file->GetPath(),FileMode::Open)) return nullptr;

  // Deserialize the stream into the FontDef 
  if(nullptr==JsonDeserializer().Deserialize(&fs,&font_def_)) {
    return nullptr;
  }

  // Close the input file
  fs.Close();  

  std::vector<String> result = Font::GetFontFamilies();
  std::vector<String> syyles = Font::GetFontStyles("Arial Narrow");

  inspector_ = EditorFactory::CreateInspector(&font_def_);
  SetInspector(inspector_);

  font_ = AssetManager::LoadAsset<Font>(file->GetGuid());
  if(font_ && image_node_) image_node_->SetImage(font_->GetImage());

  UpdateDimensions();

  return true;
}

bool FontEditor::OnApplyStyle (void) {
  if(!AssetEditor::OnApplyStyle()) return false;
  if(nullptr == image_node_) return false;

  if(font_) image_node_->SetImage(font_->GetImage());

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

  UpdateDimensions();

  return true;
}


void FontEditor::SetZoom (noz_float zoom) {
  zoom = Math::Clamp(zoom,0.01f,40.0f);
  if(zoom == zoom_) return;
  zoom_ = zoom;
  zoom_node_->SetScale(zoom_);
  grid_node_->SetScale(zoom_);
}


void FontEditor::OnMouseWheel (SystemEvent* e) {
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

void FontEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(GetFile()->GetPath(),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save font");
    return;
  }

  JsonSerializer().Serialize(&font_def_,&fs);
  fs.Close();

  AssetDatabase::ReloadAsset (GetFile()->GetGuid());

  SetModified(false);
}

void FontEditor::OnPropertyChanged (PropertyChangedEventArgs* args) {
  // Only care about properties within the font definition
  if(args->GetTarget() != &font_def_) return;

  // Free any memory for the 
  if(font_ && !font_->IsManaged()) delete font_;

  // Create a new temporary font with the new font definition parameters
  Font* font = new Font;
  if(!FontFile::Import(GetFile()->GetPath(), font_def_, (Font*)font)) {
    delete font;
    return;
  }

  // Set the new temporary font as the active font in the editor
  font_ = font;

  // Update the font image.
  if(font_ && image_node_) image_node_->SetImage(font_->GetImage());
}

void FontEditor::UpdateDimensions(void) {
  if(font_ && font_->GetImage()) {  
    if(width_text_) {
      width_text_->SetVisibility(Visibility::Visible);
      width_text_->SetText(String::Format("%d", font_->GetImage()->GetWidth()));
    }
    if(height_text_) {
      height_text_->SetVisibility(Visibility::Visible);
      height_text_->SetText(String::Format("%d", font_->GetImage()->GetHeight()));
    }
  } else {
    if(width_text_) width_text_->SetVisibility(Visibility::Hidden);
    if(height_text_) height_text_->SetVisibility(Visibility::Hidden);
  }    
}
