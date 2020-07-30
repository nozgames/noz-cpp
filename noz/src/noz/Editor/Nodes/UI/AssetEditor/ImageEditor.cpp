///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Nodes/UI/MeasureScaler.h>
#include "ImageEditor.h"

using namespace noz;
using namespace noz::Editor;


ImageEditor::ImageEditor(void) {  
  zoom_ = 1.0f;

  if(!Application::IsInitializing()) {
    EditorApplication::PropertyChanged += PropertyChangedEventHandler::Delegate(this, &ImageEditor::OnPropertyChanged);
  }
}

ImageEditor::~ImageEditor(void) {
}

bool ImageEditor::Load (AssetFile* file) {
  image_ = AssetManager::LoadAsset<Image>(file->GetGuid());
  if(image_ && image_node_) image_node_->SetImage(image_);

  FileStream fs;
  if(fs.Open(String::Format("%s.nozimage", file->GetPath().ToCString()), FileMode::Open)) {
    JsonDeserializer().Deserialize(&fs, &image_def_);
  }  

  inspector_ = EditorFactory::CreateInspector(&image_def_);
  SetInspector(inspector_);
  UpdateDimensions();

  return true;
}

void ImageEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(String::Format("%s.nozimage", GetFile()->GetPath().ToCString()),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save image");
    return;
  }

  JsonSerializer().Serialize(&image_def_,&fs);
  fs.Close();
  SetModified(false);
}

bool ImageEditor::OnApplyStyle (void) {
  if(!AssetEditor::OnApplyStyle()) return false;

  if(image_ && image_node_) image_node_->SetImage(image_);
  if(zoom_button_) zoom_button_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton);
  if(zoom_button_20_) zoom_button_20_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton20);
  if(zoom_button_50_) zoom_button_50_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton50);
  if(zoom_button_70_) zoom_button_70_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton70);
  if(zoom_button_100_) zoom_button_100_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton100);
  if(zoom_button_150_) zoom_button_150_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton150);
  if(zoom_button_200_) zoom_button_200_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton200);
  if(zoom_button_400_) zoom_button_400_->Click += ClickEventHandler::Delegate(this, &ImageEditor::OnZoomButton400);

  UpdateDimensions();

  return true;
}


void ImageEditor::SetZoom (noz_float zoom) {
  zoom = Math::Clamp(zoom,0.01f,40.0f);
  if(zoom == zoom_) return;
  zoom_ = zoom;
  zoom_node_->SetScale(zoom_);
  grid_node_->SetScale(zoom_);
}

void ImageEditor::OnZoomButton (UINode* sender) {
  zoom_popup_->Open();
}

void ImageEditor::OnZoomButton20 (UINode* sender) {
  SetZoom(0.2f);
  zoom_popup_->Close();
}

void ImageEditor::OnZoomButton50 (UINode* sender) {
  SetZoom(0.5f);
  zoom_popup_->Close();
}

void ImageEditor::OnZoomButton70 (UINode* sender) {
  SetZoom(0.7f);
  zoom_popup_->Close();
}

void ImageEditor::OnZoomButton100 (UINode* sender) {
  SetZoom(1.0f);
  zoom_popup_->Close();
}

void ImageEditor::OnZoomButton150 (UINode* sender) {
  SetZoom(1.5f);
  zoom_popup_->Close();
}

void ImageEditor::OnZoomButton200 (UINode* sender) {
  SetZoom(2.0f);
  zoom_popup_->Close();
}

void ImageEditor::OnZoomButton400 (UINode* sender) {
  SetZoom(4.0f);
  zoom_popup_->Close();
}

void ImageEditor::OnMouseWheel (SystemEvent* e) {
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

void ImageEditor::UpdateDimensions(void) {
  if(image_) {  
    if(width_text_) {
      width_text_->SetVisibility(Visibility::Visible);
      width_text_->SetText(String::Format("%d", image_->GetWidth()));
    }
    if(height_text_) {
      height_text_->SetVisibility(Visibility::Visible);
      height_text_->SetText(String::Format("%d", image_->GetHeight()));
    }
  } else {
    if(width_text_) width_text_->SetVisibility(Visibility::Hidden);
    if(height_text_) height_text_->SetVisibility(Visibility::Hidden);
  }    
}

void ImageEditor::OnPropertyChanged (PropertyChangedEventArgs* args) {
  // Only care about properties within the image def.
  if(args->GetTarget() != &image_def_) return;

  // Push wrap mode to actual image.
  if(args->GetProperty()->GetName() == "WrapMode") {
    image_->SetWrapMode(image_def_.wrap_mode_);
    return;
  }

  // Push filter mode actual image.
  if(args->GetProperty()->GetName() == "FilterMode") {
    image_->SetFilterMode(image_def_.filter_mode_);
    return;
  }

  // Push filter mode actual image.
  if(args->GetProperty()->GetName() == "ResizeWidth" || 
     args->GetProperty()->GetName() == "ResizeHeight"   ) {
    Save();
    AssetDatabase::ReloadAsset(image_->GetGuid());
    UpdateDimensions();
    return;
  }

  // Push filter mode actual image.
  if(args->GetProperty()->GetName() == "SDF") {
    Save();
    AssetDatabase::ReloadAsset(image_->GetGuid());
    UpdateDimensions();
    return;
  }

}

