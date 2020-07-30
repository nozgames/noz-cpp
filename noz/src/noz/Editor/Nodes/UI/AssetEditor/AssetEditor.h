///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetEditor_h__
#define __noz_Editor_AssetEditor_h__

namespace noz {
namespace Editor {

  class Memento;

  class AssetEditor : public EditorDocument {
    NOZ_OBJECT(Abstract)

    private: AssetFile* file_;

    public: AssetEditor (void);

    public: ~AssetEditor (void);

    public: bool Open (AssetFile* file);

    public: AssetFile* GetFile (void) const {return file_;}

    protected: virtual bool Load (AssetFile* file) {return false;}

    private: void OnAssetRenamed (AssetFile* file);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AssetEditor_h__

