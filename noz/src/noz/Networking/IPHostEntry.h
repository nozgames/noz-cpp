///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_IPHostEntry_h__
#define __noz_Networking_IPHostEntry_h__

#include "IPAddress.h"

namespace noz {
namespace Networking {

  class IPHostEntry {
    private: String host_name_;
    private: IPAddressVector addresses_;

    public: void SetHostName(const char* name) {host_name_ = name;}

    public: const String& GetHostName(void) const {return host_name_;}

    public: IPAddressVector& GetAddresses(void) {return addresses_;}
  };

} // namespace Networking
} // namespace noz


#endif //__noz_Networking_IPHostEntry_h__


