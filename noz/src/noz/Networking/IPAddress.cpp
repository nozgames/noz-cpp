///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "IPAddress.h"

using namespace noz;
using namespace noz::Networking;

IPAddress::IPAddress(void) {
  address_ = 0;
  family_ =  AddressFamily::InterNetwork;
}

IPAddress::IPAddress(noz_uint32 address) {
  address_ = address;
  family_ = AddressFamily::InterNetwork;
}

IPAddress::IPAddress(noz_uint64 address) {
  address_ = address;
  family_ = AddressFamily::InterNetworkV6;
}

