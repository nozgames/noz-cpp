///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Application.h"
#include "ApplicationSettings.h"
#include <noz/Networking/Dns.h>
#include <noz/Physics/World.h>
#include <noz/IO/Directory.h>
#include <noz/Environment.h>
#include <noz/UI/Cursors.h>
#include <noz/Nodes/UI/WindowNode.h>
#include <noz/Nodes/UI/Popup.h>

#if defined(NOZ_DEBUG)
#include <noz/Diagnostics/PerformanceCounter.h>
#include <noz/Diagnostics/Debugger.h>
#endif

#if defined(NOZ_EDITOR)
#include <noz/Editor/EditorApplication.h>
#endif

using namespace noz;
using namespace noz::Networking;

namespace noz { void RegisterTypes (void); }

Application* Application::this_ = nullptr;
FocusChangedEventHandler Application::FocusChanged;

static const Name ROOT ("ROOT");

Application::Application(void) {
  this_ = nullptr;  
  frame_number_ = 1;
  root_node_ = nullptr;
}


Application::~Application(void) {
}

void Application::Initialize(int argc, const char* argv[]) {
  if(this_ != nullptr) {
    return;
  }

  this_ = new Application;
  this_->initializing_ = true;

  ObjectManager::Initialize();

  Environment::Initialize (argc,argv);

  Console::Initialize();

  Input::Initialize();

  // Initialzie all types exported by the noz framework
  RegisterTypes();

  Type::GenerateMasks();

  // Create root node
  this_->root_node_ = new Node(ROOT,NodeAttributes::ApplicationRoot);
  this_->root_node_->Awaken();  

#if defined(NOZ_DEBUG)
  Debugger::Initialize();
#endif  

  this_->name_ = Path::ChangeExtension(Path::GetFilename(argv[0]), "");

  AssetManager::Initialize();

  Prefs::Initialize();

  Cursors::Initialize();

  ApplicationSettings::Initialize ();

  this_->initializing_ = false;

  this_->Run ();
}


void Application::Uninitialize(void) {
  if(this_ == nullptr) return;

#if defined(NOZ_EDITOR)
  Editor::EditorApplication::Uninitialize();
#else
  ApplicaitonEnd();
#endif

  ApplicationSettings::Uninitialize();

  this_->root_node_->Destroy();
  Node::DestroyNodes();

  Cursors::Uninitialize();

  // Uninitialize the physics world if it was initialized
  World::Uninitialize();

  AssetManager::Uninitialize();
  
  Prefs::Uninitialize();

  Input::Uninitialize();

  Environment::Uninitialize();

  Console::Uninitialize();

  // Unregister all types
  Type::UnregisterAllTypes();

  ObjectManager::Uninitialize();

  // Ensure that all names once they become unreferenced will be deleted.
  Name::SetPurgeOnUnreferenced(true);

  delete this_;
  this_ = nullptr;
}

void Application::Run (void) {
#if !defined(NOZ_EDITOR)
  AssetManager::AddArchive(new PakAssetArchive(Path::Combine(Environment::GetFolderPath(SpecialFolder::Application),String::Format("%s.nozpak", Environment::GetExecutableName().ToCString()))));

  ApplicationBegin();

  Window* window = new Window(WindowAttributes::Primary);
  window->SetTitle(ApplicationSettings::GetName());
  window->SetSize(Vector2(750,1334));
  window->SetScene(AssetManager::LoadAsset<Scene>(ApplicationSettings::GetMainSceneGuid())));
  window->Show();
#else
  Editor::EditorApplication::Initialize();
#endif
}

void Application::RunModal (Window* win) {
/*
  ObjectPtr<Window> parent = win->GetParent();
  if(parent) parent->SetInteractive(false);
  Run(win);
  if(parent) {
    parent->SetInteractive(true);
  }
*/
}

void Application::Frame(void) {
  // Advance the current frame number
  this_->frame_number_++;
  
  // Update the frame time.
  Time::BeginFrame();
  
  // Update input
  Input::Update ();

  // Dispatch all events.
  if(!this_->DispatchEvents()) return;

  // Perform fixed updates
  while (Time::GetFixedTime() + Time::GetFixedDeltaTime() < Time::GetTime()) {
    // Begin the fixed frame.
    Time::BeginFixedFrame();

    // ComponentEvent::FixedUpdate
    NOZ_FIXME()
/*
    for(i=0,c=components.size();i<c;i++) {
      Component* c = components[i];
      if(c && c->IsEventEnabled(ComponentEvent::FixedUpdate)) c->FixedUpdate();
    }
*/

    // Update the physics world
    World::Update();

    // End the fixed frame
    Time::EndFixedFrame();
  }

  // ComponentEvent::Update
    NOZ_FIXME()
/*
  ComponentManager::Update();
  NOZ_TODO("Should change Update to ComponentManager::Lock() / Unlock to prevent changes during iteration, Lock can update if necessary");
  for(i=0,c=components.size();i<c;i++) {
    Component* c = components[i];
    if(c && c->IsEventEnabled(ComponentEvent::Update)) c->Update();
  }

  // ComponentEvent::LateUpdate
  ComponentManager::Update();
  for(i=0,c=components.size();i<c;i++) {
    Component* c = components[i];
    if(c && c->IsEventEnabled(ComponentEvent::LateUpdate)) c->Update();
  }

  */
  this_->root_node_->Update();
  this_->root_node_->Animate();

  // Update all dirty node transforms
  Node::UpdateTransforms();

#if defined(NOZ_EDITOR)
  // Editor frame
  Editor::EditorApplication::Frame();
#endif

  // Render all nodes
  this_->root_node_->Render(nullptr);

  // Destroy all nodes queued for destruction
  Node::DestroyNodes ( );

#if defined(NOZ_DEBUG)
  PerformanceCounter::PrintAll();
  PerformanceCounter::ClearAll();
#endif
}

bool Application::DispatchEvents(void) {  
  bool mouse_move = false;

  noz_uint64 saved_frame_number = frame_number_;

  // Process all events in the event queue
  while(saved_frame_number==frame_number_ && SystemEvent::GetQueueCount()) {
    SystemEvent e = SystemEvent::PopEvent();

    // It is possible a window that generated events will be destroyed during
    // this loop which means any events generated on that window after it will
    // contain an invalid window pointer which we can use to filter out those events.
    if(e.GetWindow()==nullptr) continue;

    Vector2 old_mouse_position = Input::GetMousePosition();
    Window* old_mouse_window = Input::GetMouseWindow();

    // Allow the input to handle the event
    Input::HandleEvent(&e);

    // Allow drag drop to handle events.
    DragDrop::HandleEvent(&e);

    // If the mouse changed positions send a virtual mouse move 
    if(old_mouse_window != Input::GetMouseWindow() || old_mouse_position != Input::GetMousePosition()) {
      OnMouseMove(&e);
      mouse_move = true;
    }

    switch(e.GetEventType()) {
      case SystemEventType::CaptureChanged: {
        if(capture_ && capture_->GetWindow() == e.GetWindow()) SetCapture(nullptr);
        break;
      }

      case SystemEventType::KeyDown: OnKeyDown(&e); continue;
      case SystemEventType::KeyUp: OnKeyUp(&e); continue;

      case SystemEventType::MouseMove: mouse_move = true; OnMouseMove(&e); break;
      case SystemEventType::MouseDown: OnMouseDown(&e); break; 
      case SystemEventType::MouseUp: OnMouseUp(&e); break; 
      case SystemEventType::MouseWheel: OnMouseWheel(&e); break;

      default:
        break;
    }

    if(e.IsHandled()) continue;

    // Dispatch the event to the window
    if(e.GetWindow()) e.GetWindow()->DispatchEvent(&e);
  }

  // If the frame number advanced during the event processing then a modal dialog
  // was opened.  If this ocurred just return now.
  if(saved_frame_number != frame_number_) return false;

  // Mouse move events are not enough to handle hover checks because of animating elements.  To handle
  // this we fake a mouse move event every frame using the last known mouse window and position.
  if(!DragDrop::IsDragDropActive() && !mouse_move && Input::GetMouseWindow()) {
    SystemEvent e(SystemEventType::MouseMove, Input::GetMouseWindow(), Input::GetMousePosition(), Input::GetMouseDelta(), 0);
    OnMouseMove(&e);
  }

  return true;
}

void Application::OnKeyDown(SystemEvent* e) {
#if defined(NOZ_WINDOWS) 
  switch(e->GetKeyCode()) {
    case Keys::OemComma:  if(e->IsControl()) {Console::SetVariableInt32("ogl_debug_batches", !Console::GetVariableInt32("ogl_debug_batches")); e->SetHandled(); } break;
    case Keys::OemPeriod: if(e->IsControl()) {Console::SetVariableInt32("ogl_debug_counters", !Console::GetVariableInt32("ogl_debug_counters")); e->SetHandled(); } break;
    case Keys::W: if(e->IsControl()) {Console::SetVariableInt32("window_mouse_over", !Console::GetVariableInt32("window_mouse_over")); e->SetHandled(); } break;
  }
#endif

  if(e->IsHandled()) return;

  if(e->GetWindow()) e->GetWindow()->DispatchEvent(e);  
}

void Application::OnKeyUp(SystemEvent* e) {
  if(e->GetWindow()) e->GetWindow()->DispatchEvent(e);  
}

void Application::OnMouseMove (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetWindow());

  e->GetWindow()->GetRootNode()->HandleMouseMoveEvent(e);
  e->GetWindow()->SetCursor(e->GetCursor());  
}

void Application::OnMouseDown (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetWindow() == Input::GetMouseWindow());

  // Close any popups if abutton was pressed outside of them.
  for(noz_uint32 i=GetRootNode()->GetChildCount(); i>0; i--) {
    WindowNode* node = Cast<WindowNode>(GetRootNode()->GetChild(i-1));
    if(nullptr == node) continue;
    if(node->GetWindow() == e->GetWindow()) break;
    Popup* popup = node->GetWindow()->GetPopup();
    if(popup) {
      e->SetHandled();
      popup->Close();
      break;
    }
  }

  // If the mouse caused a popup to close then we are done.
  if(e->IsHandled()) return;

  if(capture_) {
    capture_->HandleMouseButtonEvent(e);
  } else {
    e->GetWindow()->GetRootNode()->HandleMouseButtonEvent(e);
  }
}

void Application::OnMouseUp (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetWindow() == Input::GetMouseWindow());
  noz_assert(e->GetPosition() == Input::GetMousePosition());

  if(capture_) {
    capture_->HandleMouseButtonEvent(e);
  } else {
    e->GetWindow()->GetRootNode()->HandleMouseButtonEvent(e);
  }

  SetCapture(nullptr);

#if defined(NOZ_EDITOR)
  Editor::EditorApplication::CommitUndoGroup();
#endif
}

void Application::OnMouseWheel (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetWindow() == Input::GetMouseWindow());
  noz_assert(e->GetPosition() == Input::GetMousePosition());
  
  e->GetWindow()->GetRootNode()->HandleMouseButtonEvent(e);
}

Node* Application::SetCapture(Node* capture) {
  // Ensure capture is changing
  if(this_->capture_==capture) return nullptr;

  // Get the capture window.
  Window* capture_window = nullptr;
  if(capture) {
    capture_window = capture->GetWindow();
    if(nullptr == capture_window) return nullptr;
  }

  // Change capture
  noz_assert(this_);
  Node* old = this_->capture_;
  Window* old_window = this_->capture_window_;
  this_->capture_ = capture;
  this_->capture_window_ = capture_window;

  // Set real window capture too
  if(capture_window && (old_window != capture_window)) {
    capture->GetWindow()->SetCapture();
  // Release real capture if needed
  } else if (nullptr == capture_window && old_window) {
    old_window->ReleaseCapture();
  }

  return old;
}

void Application::SetFocus(Window* focus) {
  noz_assert(this_);

  if(this_->focused_ == focus) return;

  // Set the new focus
  Window* old_focus = this_->focused_;
  this_->focused_ = focus;

  // Notify old focus window
  if(old_focus) old_focus->OnLoseFocus();

  // Notify new focus window
  if(focus) focus->OnGainFocus();
}

void Application::ClosePopupWindows (Window* parent) {  
  for(noz_uint32 i=GetRootNode()->GetChildCount(); i>0; i--) {
    WindowNode* node = Cast<WindowNode>(GetRootNode()->GetChild(i-1));
    if(nullptr == node) continue;
    
    Window* window = node->GetWindow();
    if(nullptr == window) continue;

    if(parent==window) continue;
    if(!window->IsPopup()) continue;
    if(!window->IsDescendantOf(parent)) continue;

    window->GetPopup()->Close();
  }
}