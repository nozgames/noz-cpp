///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_Dns_h__
#define __noz_Networking_Dns_h__

#include "IPHostEntry.h"

namespace noz {
namespace Networking {

  class Dns {
    public: static String GetHostName (void);

    public: static IPHostEntry GetHostEntry(const char* name);
  };

} // namespace Networking
} // namespace noz


#endif // __noz_Networking_Dns_h__


