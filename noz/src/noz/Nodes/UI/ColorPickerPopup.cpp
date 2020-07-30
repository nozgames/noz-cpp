///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/TextBox.h>
#include "ColorPickerPopup.h"

using namespace noz;

ColorPickerPopup* ColorPickerPopup::this_ = nullptr;

ColorPickerPopup::ColorPickerPopup (void) {
  refresh_ = false;
}

ColorPickerPopup::~ColorPickerPopup(void) {
}

/*
noz_float checkComp(noz_float v, noz_float t1, noz_float t2) {
  if (v < 0.0f) v = v + 1.0f;
  if (v > 1.0f) v = v - 1.0f;
  if ((v * 6.0f) < 1.0f) {
    v = t1 + (t2 - t1) * 6.0f * v;
  } else if ((v * 2.0f) < 1.0f) {
    v = t2;
  } else if ((v * 3.0f) < 2.0f) {
    v = t1 + (t2 - t1) * ((2.0f / 3.0f) - v) * 6.0f;
  } else {
    v = t1;
  }       
  return v;
}

Vector3 hsv2rgb(const Vector3& hsv) {
  Vector3 rval;
  noz_float t1;
  noz_float t2;
    
  if (hsv.y == 0.0f) {
    rval.x = hsv.z;
    rval.y = hsv.z;
    rval.z = hsv.z;
  } else {
    if (hsv.z < 0.5f) {
      t2 = hsv.z * (1.0f + hsv.y);
    } else {
      t2 = hsv.z + hsv.y - hsv.z * hsv.y;
    }
            
    t1 = 2 * hsv.z - t2;
        
    rval.x = checkComp(hsv.x + (1.0f / 3.0f), t1, t2);
    rval.y = checkComp(hsv.x, t1, t2);
    rval.z = checkComp(hsv.x - (1.0f / 3.0f), t1, t2);
  }
    
  return rval;
}
*/

Vector3 hsv2rgb(const Vector3& hsv) {
  noz_float hue, sat, val;
  hue = hsv.x;
  sat = hsv.y;
  val = hsv.z;


  noz_float r, g, b;
  noz_float i, f, p, q, t;


 


        r = 0;


        g = 0;


        b = 0;


 


        if(val == 0) {


                r = 0;


                g = 0;


                b = 0;


        } else {


                hue *= 6.0f;


                i = Math::Floor(hue);


                f = hue - i;


                p = val * (1 - sat);


                q = val * (1 - (sat * f));


                t = val * (1 - (sat * (1 - f)));


 


                if (i == 0)      { r = val; g = t;   b = p;   } 


                else if (i == 1) { r = q;   g = val; b = p;   }


                else if (i == 2) { r = p;   g = val; b = t;   }


                else if (i == 3) { r = p;   g = q;   b = val; }


                else if (i == 4) { r = t;   g = p;   b = val; }


                else if (i == 5) { r = val; g = p;   b = q;   }


        }


 

        return Vector3(r,g,b);


}


/*
private void rgb2hsv(int r, int g, int b, int hsv[]) {

		

		int min;    //Min. value of RGB

		int max;    //Max. value of RGB

		int delMax; //Delta RGB value

		

		if (r > g) { min = g; max = r; }

		else { min = r; max = g; }

		if (b > max) max = b;

		if (b < min) min = b;

								

		delMax = max - min;

	 

		float H = 0, S;

		float V = max;

		   

		if ( delMax == 0 ) { H = 0; S = 0; }

		else {                                   

			S = delMax/255f;

			if ( r == max ) 

				H = (      (g - b)/(float)delMax)*60;

			else if ( g == max ) 

				H = ( 2 +  (b - r)/(float)delMax)*60;

			else if ( b == max ) 

				H = ( 4 +  (r - g)/(float)delMax)*60;   

		}

								 

		hsv[0] = (int)(H);

		hsv[1] = (int)(S*100);

		hsv[2] = (int)(V*100);

	}
  */

Vector3 rgb2hsv(const Vector3 rgb) {
  noz_float min;    //Min. value of RGB
	noz_float max;    //Max. value of RGB
  noz_float delta; //Delta RGB value

  if (rgb.x > rgb.y) { 
    min = rgb.y; 
    max = rgb.x;
  } else { 
    min = rgb.x;
    max = rgb.y;
  }
	if (rgb.z > max) max = rgb.z;
  if (rgb.z < min) min = rgb.z;

  delta = max - min;

  noz_float H = 0.0f;
  noz_float S = 0.0f;
  noz_float V = max;

  if (delta == 0.0f || max==0.0f) {
    H = 0.0f;
    S = 0;
  } else {         
    S = delta / max;
  
  	if (rgb.x == max) {
      H = ((rgb.y - rgb.z) / delta) / 6.0f;
    } else if (rgb.y == max) {
      H = (2.0f + (rgb.z - rgb.x) / delta) / 6.0f;
    } else if (rgb.z == max) {
      H = (4.0f + (rgb.x - rgb.y) / delta) / 6.0f;   
		}
  }

  return Vector3(Math::Abs(H),S,V);
}

bool ColorPickerPopup::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;

  if(slider_r_) slider_r_->ValueChanged += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnSliderR);
  if(slider_g_) slider_g_->ValueChanged += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnSliderG);
  if(slider_b_) slider_b_->ValueChanged += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnSliderB);
  if(slider_a_) slider_a_->ValueChanged += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnSliderA);
  if(slider_h1_) slider_h1_->ValueChanged += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnHueChanged);
  if(color_square_) color_square_->ValueChanged += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnColorSquareValueChanged);
  if(text_hex_) text_hex_->TextCommited += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnTextHex);
  if(text_r_) text_r_->TextCommited += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnTextR);
  if(text_g_) text_g_->TextCommited += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnTextG);
  if(text_b_) text_b_->TextCommited += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnTextB);
  if(text_a_) text_a_->TextCommited += ValueChangedEventHandler::Delegate(this, &ColorPickerPopup::OnTextA);

  Refresh();

  return true;
}

void ColorPickerPopup::UpdateHSV(void) {
  if(refresh_) return;
  hsv_ = rgb2hsv(rgb_);
  Color old = color_;
  color_ = Color(rgb_.x, rgb_.y, rgb_.z, alpha_);
  if(color_ != old) delegate_(color_);
}

void ColorPickerPopup::UpdateRGB(void) {
  if(refresh_) return;
  rgb_ = hsv2rgb(hsv_);
  Color old = color_;
  color_ = Color(rgb_.x, rgb_.y, rgb_.z, alpha_);
  if(color_ != old) delegate_(color_);
}

void ColorPickerPopup::Refresh(void) {
  if(refresh_) return;

  refresh_ = true;

  if(slider_h1_) slider_h1_->SetValue(Vector2(0.0f,1.0f-hsv_.x));
  if(slider_h2_) slider_h2_->SetValue(Vector2(0.0f,hsv_.x));

  // Color Square
  if(color_square_) color_square_->SetValue(Vector2(hsv_.y,1.0f-hsv_.z));
  if(color_quare_colors_) {
    Vector3 rgb = hsv2rgb(Vector3(hsv_.x, 1.0f, 1.0f));
    Color hue_color1 (rgb.x, rgb.y, rgb.z, 1.0f);
    Color hue_color2 (rgb.x, rgb.y, rgb.z, 0.0f);
    color_quare_colors_->SetTopRight(hue_color1);
    color_quare_colors_->SetBottomRight(hue_color1);
    color_quare_colors_->SetTopLeft(hue_color2);
    color_quare_colors_->SetBottomLeft(hue_color2);
  }

  // Brightness
  if(slider_v_) slider_v_->SetValue(Vector2(0.0f,hsv_.z));
  if(colors_v_) {
    Vector3 rgb = hsv2rgb(Vector3(hsv_.x, hsv_.y, 1.0f));
    Color value_color (rgb.x, rgb.y, rgb.z, 1.0f);
    colors_v_->SetTopLeft(value_color);
    colors_v_->SetTopRight(value_color);
  }

  // Red
  if(text_r_) text_r_->SetText(String::Format("%d",color_.r).ToCString());
  if(slider_r_) slider_r_->SetValue(Vector2(rgb_.x, 0.0f));
  if(colors_r_) {
    colors_r_->SetBottomRight(Color(255,color_.g,color_.b,0xFF));
    colors_r_->SetTopRight(Color(255,color_.g,color_.b,255));
    colors_r_->SetBottomLeft(Color(0,color_.g,color_.b,255));
    colors_r_->SetTopLeft(Color(0,color_.g,color_.b,255));
  }

  // Green
  if(text_g_) text_g_->SetText(String::Format("%d",color_.g).ToCString());
  if(slider_g_) slider_g_->SetValue(Vector2(rgb_.y, 0.0f));
  if(colors_g_) {
    colors_g_->SetBottomRight(Color(color_.r,255,color_.b,255));
    colors_g_->SetTopRight(Color(color_.r,255,color_.b,255));
    colors_g_->SetBottomLeft(Color(color_.r,0,color_.b,255));
    colors_g_->SetTopLeft(Color(color_.r,0,color_.b,255));
  }

  // Blue
  if(text_b_) text_b_->SetText(String::Format("%d",color_.b).ToCString());
  if(slider_b_) slider_b_->SetValue(Vector2(rgb_.z, 0.0f));
  if(colors_b_) {
    colors_b_->SetBottomRight(Color(color_.r,color_.g, 255,255));
    colors_b_->SetTopRight(Color(color_.r,color_.g,255,255));
    colors_b_->SetBottomLeft(Color(color_.r,color_.g,0,255));
    colors_b_->SetTopLeft(Color(color_.r,color_.g,0,255));
  }

  // Alpha
  if(text_a_) text_a_->SetText(String::Format("%d",color_.a).ToCString());
  if(slider_a_) slider_a_->SetValue(Vector2(alpha_, 0.0f));
  if(colors_a_) {
    colors_a_->SetBottomRight(Color(color_.r,color_.g,color_.b,255));
    colors_a_->SetTopRight(Color(color_.r,color_.g,color_.b,255));
    colors_a_->SetBottomLeft(Color(color_.r,color_.g,color_.b,0));
    colors_a_->SetTopLeft(Color(color_.r,color_.g,color_.b,0));
  }

  // Hex
  if(text_hex_) text_hex_->SetText(color_.ToString().Upper().ToCString()+1);

  refresh_ = false;
}

void ColorPickerPopup::OnColorSquareValueChanged (UINode* sender) {
  hsv_.y = color_square_->GetValue().x;
  hsv_.z = 1.0f - color_square_->GetValue().y;
  UpdateRGB();
  Refresh();
}

void ColorPickerPopup::OnSliderA(UINode* sender) {
  noz_float v = slider_a_->GetValue().x;
  if(v == alpha_) return;
  alpha_ = v;  
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnSliderR(UINode* sender) {
  noz_float v = slider_r_->GetValue().x;
  if(v == rgb_.x) return;
  rgb_.x = v;
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnSliderG(UINode* sender) {
  noz_float v = slider_g_->GetValue().x;
  if(v == rgb_.y) return;
  rgb_.y = v;
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnSliderB(UINode* sender) {
  noz_float v = slider_b_->GetValue().x;
  if(v == rgb_.z) return;
  rgb_.z = v;
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnHueChanged(UINode* sender) {
  hsv_[0] = 1.0f-slider_h1_->GetValue().y;
  UpdateRGB();
  Refresh();
}

void ColorPickerPopup::OnTextHex(UINode* sender) {
  Color v = Color::Parse(text_hex_->GetText());
  if(v == color_) return;
  color_ = v;
  rgb_ = Vector3(v.r / 255.0f, v.g / 255.0f, v.b / 255.0f);
  alpha_ = v.a / 255.0f;
  delegate_(color_);
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnTextR(UINode* sender) {
  noz_byte v = Byte::Parse(text_r_->GetText());
  if(v == color_.r) return;
  rgb_.x = v / 255.0f;
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnTextG(UINode* sender) {
  noz_byte v = Byte::Parse(text_g_->GetText());
  if(v == color_.g) return;
  rgb_.y = v / 255.0f;
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnTextB(UINode* sender) {
  noz_byte v = Byte::Parse(text_b_->GetText());
  if(v == color_.b) return;
  rgb_.z = v / 255.0f;
  UpdateHSV();
  Refresh();
}

void ColorPickerPopup::OnTextA(UINode* sender) {
  noz_byte v = Byte::Parse(text_a_->GetText());
  if(v == color_.a) return;
  alpha_ = v / 255.0f;
  color_.a = v;
  delegate_(color_);
  Refresh();
}

void ColorPickerPopup::OnMouseDown (SystemEvent* e) {  
  SetFocus();
  e->SetHandled();
}

void ColorPickerPopup::OnKeyDown (SystemEvent* e) {
  Control::OnKeyDown(e);

  if(e->GetKeyCode() == Keys::Escape) {
    delegate_(original_color_);
    popup_->Close();
  }
}

void ColorPickerPopup::Show (Color color, PopupPlacement placement, const Vector2& placement_offset, Node* placement_target, const ColorChangedEvent::Delegate& delegate) {
  if(this_==nullptr) {
    this_ = new ColorPickerPopup;
    this_->popup_ = new Popup;
    this_->popup_->AddChild(this_);
  }

  this_->original_color_ = color;
  this_->color_ = color;
  this_->rgb_ = Vector3(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
  this_->alpha_ = color.a / 255.0f;
  this_->UpdateHSV();
  this_->delegate_ = delegate;
  this_->popup_->SetPlacement(placement);
  this_->popup_->SetPlacementTarget(placement_target);
  this_->popup_->SetPlacementOffset(placement_offset);
  this_->Refresh();
  this_->popup_->Open();
  this_->SetFocus();
}

void ColorPickerPopup::Hide (void) {
  if(nullptr==this_) return;

  this_->popup_->Close();
}

ColorPickerSlider::ColorPickerSlider (void) {
  SetLogicalChildrenOnly();
  horizontal_ = true;
  vertical_ = true;
}

void ColorPickerSlider::OnMouseDown (SystemEvent* e) {
  UINode::OnMouseDown(e);

  SetFocus();
  SetCapture();
  e->SetHandled();
}

void ColorPickerSlider::OnMouseUp (SystemEvent* e) {
  UINode::OnMouseUp(e);

  ReleaseCapture();
  e->SetHandled();
}

void ColorPickerSlider::OnMouseOver (SystemEvent* e) {
  UINode::OnMouseOver(e);

  if(!HasCapture()) return;

  Vector2 p = WindowToLocal(e->GetPosition());
  const Rect& r = GetRectangle();
  
  // Set new value.
  Vector2 v; 
  v.x = ((p.x - r.x) / r.w);
  v.y = ((p.y - r.y) / r.h);
  v.x = Math::Clamp(v.x, 0.0f, 1.0f);
  v.y = Math::Clamp(v.y, 0.0f, 1.0f);
  SetValue(v); 
}

void ColorPickerSlider::SetValue (const Vector2& v) {
  if(value_ == v) return;
  value_ = v;
  InvalidateTransform();
  ValueChanged(this);
}

void ColorPickerSlider::OnChildAdded (Node* child) {
  UINode::OnChildAdded(child);

  if(nullptr==content_container_) {
    content_container_ = new Node;
    AddPrivateChild(content_container_);
  }

  content_container_->AddChild(child);
}

void ColorPickerSlider::ArrangeChildren (const Rect& r) {
  if(nullptr == content_container_) return;

  const Vector2& msize = content_container_->GetMeasuredSize();
  
  Rect crect;
  crect.x = r.x + r.w * (horizontal_ ? value_.x : 0.5f)- msize.x * 0.5f;
  crect.y = r.y + r.h * (vertical_ ? value_.y : 0.5f) - msize.y * 0.5f;
  crect.w = msize.x;
  crect.h = msize.y;
  content_container_->Arrange(crect);
}
