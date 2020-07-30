///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ProjectGen_h__
#define __noz_ProjectGen_h__

#include <noz/Editor/Tool/Makefile.h>

namespace noz {

  class ProjectGen {
    protected: std::vector<Makefile*> references_;

    public: virtual ~ProjectGen (void) { }

    public: static bool Generate (const String& makefile_path, const Makefile::ParseOptions& options);

    public: static bool Generate (Makefile* makefile, const String& target_directory);

    protected: virtual bool GenerateOverride (Makefile* makefile, const String& target_directory) = 0;

    private: bool LoadReferences (Makefile* makefile);
    protected: noz_int32 FindReference (const String& path) const;
    protected: Makefile* GetReferenceMakefile (Makefile* makefile, const String& ref) const;
  };

} // namespace noz

#endif // __noz_ProjectGen_h__
