
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Physics/World.h>
#include <noz/Editor/Asset/AssetDatabase.h>
#include <noz/Editor/Nodes/UI/Workspace.h>
#include <noz/Nodes/Prefab.h>
#include <noz/Nodes/Render/TextNode.h>
#include <noz/System/Process.h>
#include <noz/Remote/RemoteServerView.h>
#include <noz/UI/ApplicationSettings.h>

#include "EditorApplication.h"
#include "EditorFactory.h"
#include "EditorSettings.h"

#include <noz/Editor/Tool/PakWriter.h>
#include <noz/Editor/Tool/Makefile.h>
#include <noz/Editor/Tool/ProjectGen/ProjectGen.h>
#include <noz/Editor/Tool/GlueGen/GlueGen.h>


using namespace noz;
using namespace noz::Editor;

EditorApplication* EditorApplication::this_ = nullptr;
PropertyChangedEventHandler EditorApplication::PropertyChanged;

void EditorApplication::Initialize(void) {
  if(this_!=nullptr) return;

  // Create the application object
  this_ = new EditorApplication;

  // Parse the command line options
  this_->ParseOptions();

  EditorSettings::Initialize();

  // Initialize sub systems
  World::Initialize();
  AssetDatabase::Initialize();
  EditorFactory::Initialize();

  // Execute the desired run command.
  switch(this_->options_.run_command_) {
    case RunCommand::Deploy:
      Deploy (this_->options_.platform_, this_->options_.target_);
      break;

    case RunCommand::Edit:
      this_->window_ = this_->CreateEditorWindow ();
      break;

    case RunCommand::Play:
      this_->window_ = this_->CreatePlayerWindow ();
      break;

    case RunCommand::Thumbnail:
      noz_assert(false);
      break;
  }

  if(nullptr != this_->window_) this_->window_->Show();
}

void EditorApplication::Uninitialize(void) {
  if(this_==nullptr) return;

  if(this_->options_.run_command_ == RunCommand::Play) ApplicationEnd();

  EditorSettings::Uninitialize();

  // Save window size.
  if(Window::GetPrimaryWindow()) {
    Rect srect = Window::GetPrimaryWindow()->GetScreenRect();
    Rect crect = Window::GetPrimaryWindow()->GetClientRect();
    if(this_->options_.run_command_==RunCommand::Edit) {
      Prefs::SetInt32("EditorWindowX",(noz_int32)srect.x);
      Prefs::SetInt32("EditorWindowY",(noz_int32)srect.y);
      Prefs::SetInt32("EditorWindowWidth",(noz_int32)crect.w);
      Prefs::SetInt32("EditorWindowHeight",(noz_int32)crect.h);
    } else {
      Prefs::SetInt32("PlayerWindowX",(noz_int32)srect.x);
      Prefs::SetInt32("PlayerWindowY",(noz_int32)srect.y);
      Prefs::SetInt32("PlayerWindowWidth",(noz_int32)crect.w);
      Prefs::SetInt32("PlayerWindowHeight",(noz_int32)crect.h);
    }
  }

  EditorFactory::Uninitialize();
  AssetDatabase::Uninitialize();
  World::Uninitialize();

  delete this_;
  this_ = nullptr;
}

EditorApplication::EditorApplication (void) {
  window_ = nullptr;
  workspace_ = nullptr;
}

EditorApplication::~EditorApplication(void) {
}

void EditorApplication::EditAsset(AssetFile* file) {
  this_->workspace_->EditAsset(file);
}

void EditorApplication::Inspect (Object* o) {
}

void EditorApplication::Frame(void) {
  // ComponentEvent::EditorUpdate
  NOZ_FIXME()
  /*
  const std::vector<ObjectPtr<Component>>& components = ComponentManager::GetComponents();
  ComponentManager::Update();
  for(noz_uint32 i=0;i<components.size();i++) {
    Component* c = components[i];
    if(c && c->IsEventEnabled(ComponentEvent::Animate)) c->EditorUpdate();
  }
  */

  // Clean the dirty nodes again to update any manipulators
  Node::UpdateTransforms();
}

void EditorApplication::ParseOptions (void) {
  options_.run_command_ = RunCommand::Play;
  options_.width_ = 750;
  options_.height_ = 1334;
  options_.remote_ = false;
  options_.scene_guid_ = ApplicationSettings::GetMainSceneGuid();

  for(noz_uint32 i=1,c=Environment::GetCommandArgs().size(); i<c; i++) {
    const String& arg = Environment::GetCommandArgs()[i];

    if(arg[0] != '-') continue;
    
    String option = arg.Substring(1);
    String value;
    noz_int32 equal = option.IndexOf('=');
    if(equal != -1) {
      value = option.Substring(equal+1);
      option = option.Substring(0,equal);
    }      

    if(option.Equals("edit",StringComparison::OrdinalIgnoreCase)) {
      options_.run_command_ = RunCommand::Edit;
      continue;
    }

    if(option.Equals("deploy",StringComparison::OrdinalIgnoreCase)) {
      options_.run_command_ = RunCommand::Deploy;
      continue;
    }

    if(option.Equals("target",StringComparison::OrdinalIgnoreCase)) {
      options_.target_ = value;
      continue;
    }

    if(option.Equals("platform",StringComparison::OrdinalIgnoreCase)) {
      options_.platform_ = value;
      continue;
    }

    if(option.Equals("scene",StringComparison::OrdinalIgnoreCase)) {
      options_.scene_guid_ = Guid::Parse(value);
      continue;
    }

    if(option.Equals("width",StringComparison::OrdinalIgnoreCase)) {
      options_.width_ = Int32::Parse(value);
      continue;
    }

    if(option.Equals("height",StringComparison::OrdinalIgnoreCase)) {
      options_.height_ = Int32::Parse(value);
      continue;
    }

    if(option.Equals("remote",StringComparison::OrdinalIgnoreCase)) {
      options_.remote_ = true;
      continue;
    }
  }
}

Window* EditorApplication::CreateEditorWindow(void) {
  Window* window = new Window(WindowAttributes::Primary);
  window->SetTitle("NoZ Editor");
  window->MoveTo(
    Rect(
      (noz_float)Prefs::GetInt32("EditorWindowX", 10),
      (noz_float)Prefs::GetInt32("EditorWindowY", 10),
      (noz_float)Prefs::GetInt32("EditorWindowWidth", 750),
      (noz_float)Prefs::GetInt32("EditorWindowHeight", 1334)
    ),
    nullptr
  );

  if(EditorSettings::GetTestPrefabGuid().IsEmpty()) {
    Scene* scene = AssetManager::LoadAsset<Scene>(EditorSettings::GetEditorSceneGuid());
    window->GetRootNode()->AddChild(scene->GetRootNode());
    this_->workspace_ = Cast<Workspace>(scene->GetRootNode()->FindChild("WORKSPACE",true));
#if 0
    window->GetRootNode()->PrintTree(0,true);
#endif
  } else {
    Scene* scene = new Scene;
    window->GetRootNode()->AddChild(scene->GetRootNode());

    Prefab* t = AssetManager::LoadAsset<Prefab>(EditorSettings::GetTestPrefabGuid());
    if(t) {
      PrefabNode* node = t->Instantiate();
      if(node) {
        scene->GetRootNode()->AddChild(node);
      }
    }
  }

  return window;
}



Window* EditorApplication::CreatePlayerWindow(void) {
  ApplicationBegin ( );

  Window* window = new Window(WindowAttributes::Primary);
  window->SetTitle (ApplicationSettings::GetName());
  window->SetSize(Vector2((noz_float)options_.width_,(noz_float)options_.height_));

  Scene* scene = AssetManager::LoadAsset<Scene>(options_.scene_guid_.IsEmpty() ? ApplicationSettings::GetMainSceneGuid() : options_.scene_guid_);
  if(scene) {
    if(options_.remote_) {
      // Create a remote server...
      RemoteServerView* rv = new RemoteServerView;
      rv->Connect();
      rv->AddChild(scene->GetRootNode());    
      window->GetRootNode()->AddChild(rv);
    } else {
      window->GetRootNode()->AddChild(scene->GetRootNode());
    }
  } else {
    Scene* scene = new Scene;
   
    TextNode* text = new TextNode;
    text->SetText("No main scene specified in applicaiton settings");
    text->SetHorizontalAlignment(Alignment::Center);
    text->SetVerticalAlignment(Alignment::Center);
    scene->GetRootNode()->AddChild(text);
    window->GetRootNode()->AddChild(scene->GetRootNode());
  }

  return window;
}

void EditorApplication::Deploy (const String& platform, const String& target) {
#if 0
#if 1
  String pak_path = Path::Combine(target,String::Format("%s.nozpak",Path::GetFilenameWithoutExtension(Project::GetPath()).ToCString()));
  Editor::PakWriter writer;
  writer.Open(pak_path);
  std::vector<Editor::AssetFile*> files = Editor::AssetDatabase::GetFiles(typeof(Object),nullptr);
  for(noz_uint32 i=0,c=files.size(); i<c; i++) {
    writer.AddAsset(files[i]);
  }
  writer.Close();
#endif

  Makefile::ParseOptions options;
  options.defines.insert("Deploy");
  options.platform = Makefile::StringToPlatformType(platform.ToCString());
  if(options.platform == Makefile::PlatformType::Unknown) return;    

  Makefile* makefile = Makefile::Parse(EditorSettings::GetMakefilePath(),options);
  if(nullptr == makefile) return;

  String target_dir = Path::Canonical(Path::Combine(target,Path::GetRelativePath(makefile->target_dir_,Environment::GetCurrentDirectory())));

  Makefile::BuildFile* pak_file = new Makefile::BuildFile;
  pak_file->directory_ = false;
  pak_file->path_ = String::Format("../../%s.nozpak", Path::GetFilenameWithoutExtension(Project::GetPath()).ToCString());
  pak_file->glue_ = false;
  pak_file->precompiled_header_ = Makefile::PrecompiledHeader::None;
  pak_file->group_ = "res";
  makefile->files_.push_back(pak_file);

  Deploy(makefile,target_dir);

  delete makefile;
#endif
}

bool EditorApplication::Deploy (Makefile* makefile, const String& target_dir) {
  if(!Directory::CreateDirectory(target_dir)) return false;

  if(makefile->glue_) {
    for(noz_uint32 i=0,c=makefile->configurations_.size(); i<c; i++) {
      Makefile::Configuration& cfg = *makefile->configurations_[i];
      GlueGen::Options glue_options;
      glue_options.defines_.insert("Deploy");
      glue_options.clean_ = true;
      glue_options.config_ = makefile->configurations_[i]->name_;
      glue_options.platform_ = makefile->platform_;
      glue_options.output_path_ = Path::Combine(target_dir,Path::Combine(Path::Combine(cfg.output_directory_,"glue"),String::Format("%s.glue.h", makefile->name_.ToCString())));
      
      GlueGen::Generate(makefile->path_, glue_options);

      makefile->configurations_[i]->include_directories_.push_back(Path::GetRelativePath(glue_options.output_path_,makefile->target_dir_));
      makefile->glue_ = false;
    }
  }

  ProjectGen::Generate(makefile, target_dir);

  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    String src = Path::Canonical(Path::Combine(makefile->target_dir_,makefile->files_[i]->path_));
    String dst = Path::Canonical(Path::Combine(target_dir, makefile->files_[i]->path_));

    if(makefile->files_[i]->directory_) {
      Directory::Copy(src,dst);
    } else {
      Directory::CreateDirectory(Path::GetDirectoryName(dst));
      File::Copy(src,dst,true); 
    }
  }

  for(noz_uint32 i=0,c=makefile->references_.size(); i<c; i++) {
    Makefile* ref_makefile = Makefile::Parse(Path::Canonical(Path::Combine(makefile->target_dir_,makefile->references_[i].path_)),makefile->options_);
    if(nullptr == ref_makefile) continue;
    String ref_target_dir = Path::GetRelativePath(ref_makefile->target_dir_,makefile->target_dir_);
    ref_target_dir = Path::Canonical(Path::Combine(target_dir,ref_target_dir));
    Deploy(ref_makefile,ref_target_dir);
    delete ref_makefile;
  }

  return true;
}

void EditorApplication::CommitUndoGroup (void) {
  if(nullptr==this_) return;
  if(nullptr==this_->workspace_) return;
  if(nullptr==this_->workspace_->GetActiveDocument()) return;
  this_->workspace_->GetActiveDocument()->CommitUndoGroup();
}
