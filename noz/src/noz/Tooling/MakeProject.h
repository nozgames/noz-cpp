///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_MakeProject_h__
#define __noz_MakeProject_h__

#include <noz/IO/TextReader.h>
#include <noz/IO/Stream.h>

namespace noz {

  class JsonObject;

  class MakeProject {
    public: static JsonObject* Load (const String& path, const Name& platform);
    public: static JsonObject* Load (const String& path, const Name& platform, const std::set<Name>& defines);
  };

} // namespace noz

#endif // __noz_MakeProject_h__
