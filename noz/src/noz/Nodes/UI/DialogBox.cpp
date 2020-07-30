///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include "DialogBox.h"

using namespace noz;

DialogBoxResult DialogBox::DoModal (Window* parent) {
  Window* window = new Window(WindowAttributes::Default,parent);

  Vector2 size = Measure(parent->GetScreenSize());
  Rect r;
  r.x = parent->GetScreenRect().w * 0.5f - size.x * 0.5f;
  r.y = parent->GetScreenRect().h * 0.5f - size.y * 0.5f;
  r.w = size.x;
  r.h = size.y;

  Scene* scene = new Scene;
  scene->GetRootNode()->AddChild(this);
  window->SetTitle(title_);
  window->GetRootNode()->AddChild(scene->GetRootNode());

  window->MoveTo(r,parent);
  window->SetSize(size);

  window->Show();
  Application::RunModal(window);

  Orphan();

  delete window;

  return result_;
}

DialogBox::DialogBox(void) {
  result_ = DialogBoxResult::Unknown;
}

void DialogBox::EndDialog(DialogBoxResult result) {
  result_ = result;
  GetWindow()->Close();
}

void DialogBox::SetTitle(const char* v) {
  title_ = v;
}
