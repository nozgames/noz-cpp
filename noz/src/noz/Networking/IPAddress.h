///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Networking_IPAddress_h__
#define __noz_Networking_IPAddress_h__

namespace noz {
namespace Networking {

  enum class AddressFamily {
    InterNetwork,
    InterNetworkV6,
  };

  class IPAddress {
    private: noz_uint64 address_;

    private: AddressFamily family_;

    public: IPAddress(void);
    public: IPAddress(noz_uint32 address);
    public: IPAddress(noz_uint64 address);

    public: static IPAddress Parse (const char* address);

    public: noz_uint32 GetAddressIPv4 (void) const {return (noz_uint32)address_;}

    public: noz_uint64 GetAddressIPV6 (void) const {return (noz_uint64)address_;}

    public: AddressFamily GetAddressFamily(void) const {return family_;}

    public: String ToString(void) const;
  };

  typedef std::vector<IPAddress> IPAddressVector;

} // namespace Networking
} // namespace noz


#endif //__noz_Networking_IPAddress_h__


