///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_VectorProperty_h__
#define __noz_VectorProperty_h__

namespace noz {

  /**
   * Property which represents a vector of intrinsic values.
   *   ex. std::vector<noz_int32>
   */
  template <typename T> class IntrinsicVectorProperty : public Property {
    NOZ_TEMPLATE()

    public: IntrinsicVectorProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Overriden in property implementation to return property vector value 
    public: virtual std::vector<T>& Get (Object* t) = 0;
  };

}


#endif // __noz_VectorProperty_h__
