///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EditorApplication_h__
#define __noz_Editor_EditorApplication_h__

#include "EditorEvents.h"
#include "EditorFactory.h"
#include "EditorSettings.h"
#include "EditorDocument.h"
#include "Nodes/UI/Workspace.h"

namespace noz { class Makefile; }

namespace noz {
namespace Editor {

  class Workspace;
  class AssetFile;

  class EditorApplication {
    private: enum class RunCommand {
      Play,
      Edit,
      Deploy,
      Thumbnail
    };
      
    private: struct Options {
      RunCommand run_command_;

      String platform_;

      String target_;

      Guid scene_guid_;

      noz_int32 width_;

      noz_int32 height_;

      bool remote_;
    };

    private: static EditorApplication* this_;

    private: Window* window_;

    private: Workspace* workspace_;

    private: Options options_;

    public: static PropertyChangedEventHandler PropertyChanged;

    public: EditorApplication (void);

    public: ~EditorApplication (void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static EditorApplication* GetInstance(void) {return this_;}
    
    /// Open the given asset for editing.
    public: static void EditAsset (AssetFile* asset);

    public: static void Inspect (Object* o);

    public: static void Frame (void);

    private: void ParseOptions (void);

    private: Window* CreateEditorWindow (void);

    private: Window* CreatePlayerWindow (void);

    public: static void Deploy (const String& platform, const String& target);

    public: static void CommitUndoGroup (void);

    private: bool Deploy (Makefile* makefile, const String& target);
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_EditorApplication_h__