///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef _noz_VisualStudioProjectGen_h__
#define _noz_VisualStudioProjectGen_h__

#include <noz/Editor/Tool/ProjectGen/ProjectGen.h>

namespace noz {
  
  class TextWriter;

  class VisualStudioProjectGen : public ProjectGen {
    private: enum class ItemType {
      ClInclude,
      ClCompile,
      ResourceCompile,
      None,
    };

    private: ItemType GetItemType (const Makefile::BuildFile& file) const;

    private: String ItemTypeToString (ItemType it) const;

    private: bool IsReferenceOf (Makefile* makefile, Makefile* reference) const;

    private: bool WriteSolution (Makefile* makefile, const String& target_dir);

    private: void WriteFilters (Makefile* makefile, TextWriter& writer, ItemType item_type);

    private: bool WriteFilters (Makefile* makefile, const String& target_dir);

    protected: virtual bool GenerateOverride (Makefile* makefile, const String& target_dir) override;  
  };

} // namespace noz

#endif // _noz_VisualStudioProjectGen_h__
