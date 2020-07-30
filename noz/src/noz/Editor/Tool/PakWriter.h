///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_PakWriter_h__
#define __noz_Editor_PakWriter_h__

#include <noz/Assets/PakAssetArchive.h>

namespace noz {
namespace Editor {

  class AssetFile;

  class PakWriter {
    private: std::vector<PakAssetArchive::FileRecord> records_;

    private: FileStream stream_;

    public: PakWriter (void);

    public: ~PakWriter (void);

    public: bool Open (const String& path);

    public: void Close (void);

    public: bool AddAsset (AssetFile* file);
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_PakWriter_h__

