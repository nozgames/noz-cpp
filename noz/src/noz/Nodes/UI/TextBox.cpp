///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TextBox.h"

#include <noz/Nodes/UI/ScrollView.h>
#include <noz/Nodes/Render/EditableTextNode.h>

using namespace noz;

TextBox::TextBox (void) {
  select_on_focus_ = false;  
  selection_drag_ = 0;
  SetFocusable();
  SetRequiresKeyboard();
}

bool TextBox::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == text_node_) return false;

  // Set initial text
  text_node_->SetText(text_);

  // Optional place holder text.
  if(placeholder_text_node_) placeholder_text_node_->SetText(placeholder_text_);

  return true;
}

void TextBox::SetText(const char* text) {
  // Do not allow setting text while the control has focus
  if(HasFocus()) return;

  // Ensure the text changed..
  if(text_.Equals(text)) return;

  // Set the text
  text_ = text;

  // Set the text in the text node 
  if(IsStyleValid()) {
    text_node_->SetText(text);
    UpdateAnimationState();
  }
}

void TextBox::SetPlaceholderText(const char* text) {
  // Ensure the text changed..
  if(placeholder_text_.Equals(text)) return;

  // Set the text
  placeholder_text_ = text;

  // Set the text in the text node 
  if(placeholder_text_node_) placeholder_text_node_->SetText(placeholder_text_);

  // Update placeholder text visibility
  UpdateAnimationState();
}

void TextBox::OnMouseDown (SystemEvent* e) {
  Control::OnMouseDown(e);

  // Ignore the mouse button if the style is invalid.
  if(!IsStyleValid()) return;

  if(e->GetButton() == MouseButton::Left || e->GetButton() == MouseButton::Right) {
    bool had_focus = HasFocus();
    if(!had_focus) SetFocus();
    if(!select_on_focus_ || had_focus) {
      text_node_->Select(text_node_->GetCharIndexFromPosition(text_node_->WindowToLocal(e->GetPosition())));

      ScrollText();

      // Set capture for selection drag.
      if(e->GetButton() == MouseButton::Left) {
        selection_drag_ = text_node_->GetSelectionStart();
        SetCapture();
      }
    }

    e->SetHandled();
  }
}

void TextBox::OnMouseOver (SystemEvent* e) {
  Control::OnMouseOver(e);

  // Ignore the mouse move if the style is invalid
  if(!IsStyleValid()) return;

  // If we have capture then selection is being draged so update it now.
  if(HasCapture() && !e->GetDelta().empty()) {
    text_node_->SetSelectionEnd(text_node_->GetCharIndexFromPosition(text_node_->WindowToLocal(e->GetPosition())));
    ScrollText();
  }
}


void TextBox::OnMouseUp (SystemEvent* e) {
  Control::OnMouseUp(e);

  // Ignore the mouse button if the style is invalid
  if(!IsStyleValid()) return;

  ReleaseCapture();
  e->SetHandled();
}


void TextBox::OnGainFocus (void) {  
  Control::OnGainFocus();

  if(!IsStyleValid()) return;

  if(select_on_focus_) {
    text_node_->SelectAll();
    //ScrollText();
  }
}

void TextBox::OnLoseFocus(void) {
  Control::OnLoseFocus();

  if(!IsStyleValid()) return;

  if(!text_.Equals(text_node_->GetText())) {
    text_ = text_node_->GetText();
    TextCommited(this);
  }
}

void TextBox::OnKeyDown (SystemEvent* e) {
  noz_assert(e);
  noz_assert(text_node_);

  e->SetHandled();

  switch(e->GetKeyCode()) {
    case Keys::Escape:
      if(text_node_) {
        text_node_->SetText(text_);
        text_node_->SelectAll();        
        UpdateAnimationState();
      }
      break;

    case Keys::Enter:
      if(text_node_) text_node_->SelectAll();
      text_ = text_node_->GetText();
      TextCommited(this);
      break;

    case Keys::Tab: return;
    case Keys::Back:
      if(text_node_->GetText().GetLength()>0) {
        if(text_node_->HasSelection()) {
          text_node_->ReplaceText("");
        } else {
          text_node_->Select(text_node_->GetSelectionStart()-1,text_node_->GetSelectionStart());
          text_node_->ReplaceText("");
        }
        TextChanged(this);
        UpdateAnimationState();
        ScrollText();
      } 
      return;                    

    case Keys::Delete:
      if(text_node_->HasSelection()) {
        text_node_->ReplaceText("");
        ScrollText();
      } else if(text_node_->GetSelectionStart()<text_node_->GetText().GetLength()) {
        noz_int32 sel = text_node_->GetSelectionStart();
        text_node_->Select(text_node_->GetSelectionStart(),text_node_->GetSelectionStart()+1);
        text_node_->ReplaceText("");
        text_node_->Select(sel);
        ScrollText();
      }
      TextChanged(this);
      UpdateAnimationState();
      return;

    case Keys::Home:
      if(e->IsShift()) {
        text_node_->SetSelectionEnd(0);
      } else {
        text_node_->Select(0);
      }
      ScrollText();
      return;
           
    case Keys::End: {
      noz_int32 c = text_node_->GetLastCharIndexFromLine(text_node_->GetLineFromCharIndex(text_node_->GetSelectionEnd()));
      if(e->IsShift()) {
        text_node_->SetSelectionEnd(c);
      } else {
        text_node_->Select(c);
      }
      ScrollText();
      return;
    }

    case Keys::Left:
      if(e->IsShift()) {
        text_node_->SetSelectionEnd(text_node_->GetSelectionEnd()-1);
      } else if(text_node_->HasSelection()) {
        text_node_->Select(text_node_->GetSelectionMin());
      } else {
        text_node_->Select(text_node_->GetSelectionMin()-1);
      }
      ScrollText();
      return;

    case Keys::Right:
      if(e->IsShift()) {
        text_node_->SetSelectionEnd(text_node_->GetSelectionEnd()+1);
      } else if(text_node_->HasSelection()) {
        text_node_->Select(text_node_->GetSelectionMax());
      } else {
        text_node_->Select(text_node_->GetSelectionMin()+1);
      }
      ScrollText();
      return;
          
    default: {
      char c = e->GetKeyChar();
      if(c!=0) {
        // Cut/Copy
        if((c=='c' || c=='C' || c=='x' || c=='X') && e->IsControl()) {
          StringObject s(text_node_->GetSelectedText());
          if(!s.GetValue().IsEmpty()) Application::SetClipboard(&s);

          //Cut
          if(c=='x' || c=='X') {
            text_node_->ReplaceText("");
            TextChanged(this);
            UpdateAnimationState();
            ScrollText();
          }
        // Pase
        } else if((c=='v' || c=='V') && e->IsControl()) {
          StringObject* so = (StringObject*)Application::GetClipboard(typeof(StringObject));
          if(so) {
            text_node_->ReplaceText(so->GetValue().ToCString());
            TextChanged(this);
            UpdateAnimationState();
            ScrollText();
            delete so;
          }
        // Select All
        } else if((c=='a' || c=='A') && e->IsControl()) {
          text_node_->SelectAll();
        // Normal characters
        } else {
          text_node_->ReplaceText(e->GetKeyChar());
          TextChanged(this);
          UpdateAnimationState();
          ScrollText();
        }
      }
      break;
    }
  }
}

void TextBox::ScrollText(void) {
  if(scroll_view_==nullptr) return;

  if(!text_node_->IsVisible()) return;

  // World position of caret
  Vector2 caret = text_node_->GetPositionFromCaret();

  Vector2 coffset = scroll_view_->GetOffsetFromPosition(scroll_view_->ViewportToLocal(text_node_->LocalToViewport(caret)));
  Vector2 offset = scroll_view_->GetOffset();
  Vector2 vsize = scroll_view_->GetViewportSize();

  if(coffset.x - offset.x > vsize.x - 10 || coffset.x < offset.x) {
    scroll_view_->ScrollToHorizontalOffset(coffset.x - vsize.x + 10);
  } 
}

void TextBox::UpdateAnimationState(void) {
  Control::UpdateAnimationState();
  
  if(HasFocus()) {
    SetAnimationState(UI::StateFocused);
  } else {
    SetAnimationState(UI::StateUnFocused);
  }

  if(nullptr==text_node_ || text_node_->GetText().IsEmpty()) {
    SetAnimationState(UI::StateEmpty);
  } else {
    SetAnimationState(UI::StateNotEmpty);
  }
}

const String& TextBox::GetText(void) const {
  if(text_node_) return text_node_->GetText();
  return text_;
}
