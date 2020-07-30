///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Application_h__
#define __noz_Application_h__

#include "../SystemEvent.h"
#include "UIEvents.h"
#include "UIStates.h"
#include "Input.h"
#include "Window.h"

namespace noz {
 
  class UINode;

  class Application : public Object {    
    public: static FocusChangedEventHandler FocusChanged;
    
    /// True if the application is still being initialized.
    private: bool initializing_;

    /// Name of the application
    private: Name name_;

    /// Root node of entire application
    private: Node* root_node_;

    /// Current frame number
    private: noz_uint64 frame_number_;

    /// Window that currently has focus
    private: ObjectPtr<Window> focused_;

    /// Window that currently has capture
    private: ObjectPtr<Window> capture_window_;

    /// Node that currently has capture
    private: ObjectPtr<Node> capture_;

    /// Singleton instance.
    private: static Application* this_;

    /// Static class, private constructor
    private: Application(void);

    /// Default destructor
    public: ~Application(void);

    /// Initialize the singleton.
    public: static void Initialize(int argc, const char* argv[]);

    /// Uninitialize the singleton.
    public: static void Uninitialize(void);

    /// Return true if the application is still intiailzing
    public: static bool IsInitializing (void) {return this_ && this_->initializing_;}
   
    /// Run the given window moodally.
    public: static void RunModal (Window* win);

    /// end a frame update
    public: static void Frame (void);

    /// Return the application root node
    public: static Node* GetRootNode (void) {return this_->root_node_;}

    /// Return the friendly application name.
    public: static const Name& GetName(void) {return this_->name_;}

    /// Return the current frame number which is incremented each frame.
    public: static noz_uint64 GetFrameNumber(void) {return this_->frame_number_;}

    /// Set the focused window and return the previously focused window
    public: static void SetFocus (Window* focus);

    /// Set the capture node and return the old capture node
    public: static Node* SetCapture (Node* capture);

    /// Return the UINode that currently has focus
    public: static Window* GetFocus (void) {return this_->focused_;}

    /// Return the capture node
    public: static Node* GetCapture (void) {return this_->capture_;}

    public: static void SetClipboard (Object* o);

    public: static Object* GetClipboard (Type* type);

    public: static bool IsClipboardTypeAvailable (Type* t);

    private: bool DispatchEvents(void);

    private: void ClosePopupWindows (Window* parent);

    private: void OnKeyDown (SystemEvent* e);
    private: void OnKeyUp (SystemEvent* e);

    private: void OnMouseDown (SystemEvent* e);
    private: void OnMouseMove (SystemEvent* e);
    private: void OnMouseUp (SystemEvent* e);
    private: void OnMouseWheel (SystemEvent* e);

    private: void Run (void);
  };

} // namespace noz


#endif //__noz_Application_h__
