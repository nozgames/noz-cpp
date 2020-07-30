///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "WindowNode.h"

using namespace noz;

static const Name WINDOW ("WINDOW");
static Int32ConsoleVariable* window_mouse_over = nullptr;

WindowNode::WindowNode(Window* window) {
  SetName(WINDOW);  

  window_ = window;
  attr_ = attr_ | NodeAttributes::WindowRoot;

  // Console variable for debugging mouse overs
  if(window_mouse_over==nullptr) window_mouse_over = Console::RegisterInt32Variable("window_mouse_over", 0, 0, 1);
}

WindowNode::~WindowNode(void) {
}

void WindowNode::Render (RenderContext* rc) {
  // Begin painting the window..
  window_->handle_->BeginPaint();

#if 0
  static Image* image = new Image(512,512,ImageFormat::R8G8B8);
  static RenderTarget* target = new RenderTarget;
  target->SetImage(image);

#if 0
  window_->rc_->Begin(Vector2(image->GetWidth(),image->GetHeight()), target);
#else
  window_->rc_->Begin(
    Vector2(
      window_->handle_->GetClientRect().w,
      window_->handle_->GetClientRect().h
    ),
    target
  );
#endif

  rc = window_->rc_;
  UINode::Render(rc);
  window_->rc_->End();

  FileStream* fs = File::CreateStream("e:/temp/test.png", FileMode::Truncate);
  image->Save(fs);
  fs->Close();
  delete fs;

  // Begin graphics frame 
  window_->rc_->Begin(
    Vector2(
      window_->handle_->GetClientRect().w,
      window_->handle_->GetClientRect().h
    ),
    nullptr
  );

#if 1
  RenderMesh mesh(4,6);
  mesh.AddTriangle(0,1,2);
  mesh.AddTriangle(0,2,3);
  mesh.SetTexture(image);
  mesh.AddVertex(Vector2(0.0f,0.0f),Vector2::Zero,Color::White);
  mesh.AddVertex(Vector2(window_->handle_->GetClientRect().w,0.0f),Vector2::OneZero,Color::White);
  mesh.AddVertex(Vector2(window_->handle_->GetClientRect().w,window_->handle_->GetClientRect().h),Vector2::One,Color::White);
  mesh.AddVertex(Vector2(0.0f,window_->handle_->GetClientRect().h),Vector2::ZeroOne,Color::White);
  rc->Draw(&mesh);
#endif

  // End graphics frame.
  rc->End();

#else

  // Begin graphics frame 
  window_->rc_->Begin(
    Vector2(
      window_->handle_->GetClientRect().w,
      window_->handle_->GetClientRect().h
    ),
    nullptr
  );

  rc = window_->rc_;

  UINode::Render(rc);

  if(window_mouse_over->GetValue()) {
    Node* hit = HitTest(this, Input::GetMousePosition());
    static ObjectPtr<Node> last_hit;
    if(hit) {
      if(last_hit != hit) {
        Console::WriteLine("[%08x] : - %s %s",hit, hit->GetType()->GetName().ToCString(), hit->GetName().IsEmpty()?"":hit->GetName().ToCString());
        last_hit = hit;
      }
      Rect r = hit->LocalToWindow(hit->GetRectangle());
      window_->rc_->DrawDebugLine(r.GetTopLeft(), r.GetBottomLeft(),Color::Red);
      window_->rc_->DrawDebugLine(r.GetBottomLeft(), r.GetBottomRight(),Color::Red);
      window_->rc_->DrawDebugLine(r.GetBottomRight(), r.GetTopRight(),Color::Red);
      window_->rc_->DrawDebugLine(r.GetTopRight(), r.GetTopLeft(),Color::Red);
      window_->rc_->DrawDebugLine(hit->GetRenderRectangle().GetTopLeft(), hit->GetRenderRectangle().GetBottomLeft(),Color::White);
      window_->rc_->DrawDebugLine(hit->GetRenderRectangle().GetBottomLeft(), hit->GetRenderRectangle().GetBottomRight(),Color::White);
      window_->rc_->DrawDebugLine(hit->GetRenderRectangle().GetBottomRight(), hit->GetRenderRectangle().GetTopRight(),Color::White);
      window_->rc_->DrawDebugLine(hit->GetRenderRectangle().GetTopRight(), hit->GetRenderRectangle().GetTopLeft(),Color::White);
    } else if (last_hit) {
      Console::WriteLine("[00000000] : None");
      last_hit = nullptr;
    }
  }

  // End graphics frame.
  rc->End();
#endif

  // End window render frame
  window_->handle_->EndPaint();
}

void WindowNode::Arrange (const Rect& r) {
  UINode::Arrange(window_->GetClientRect());
}

Vector2 WindowNode::Measure (const Vector2& a) {
  return UINode::Measure(window_->GetClientRect().GetSize());
}

