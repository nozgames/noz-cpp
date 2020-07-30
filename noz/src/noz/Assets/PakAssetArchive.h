///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PakAssetArchive_h__
#define __noz_PakAssetArchive_h__

#include "AssetArchive.h"

namespace noz { namespace Editor { class PakWriter; } }

namespace noz {

  class PakAssetArchive : public AssetArchive {
    NOZ_OBJECT()

    friend class Editor::PakWriter;

    private: static const noz_uint32 FourCC;

    private: struct FileRecord {
      Guid guid_;
      noz_uint32 offset_;
      noz_uint32 size_;
      noz_uint32 name_offset_;
    };

    private: String path_;

    private: std::vector<FileRecord> files_;

    private: FileStream stream_;

    public: PakAssetArchive(const String& path);

    public: virtual Stream* OpenFile (const Guid& guid) override;

    private: bool Open (void);
  };

} // namespace noz


#endif // __noz_PakAssetArchive_h__

