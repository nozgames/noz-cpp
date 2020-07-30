///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Enum_h__
#define __noz_Enum_h__

namespace noz {

  template <typename T> class Enum {
    public: static Type* type__;
    public: static const Name& (*GetName)(T value);
    public: static T (*GetValue)(const Name& value);
    public: static const std::vector<Name>& (*GetNames) (void);
  };
  
  template <typename T> Type* Enum<T>::type__ = nullptr;
  template <typename T> const Name& (*Enum<T>::GetName)(T value) = nullptr;
  template <typename T> T (*Enum<T>::GetValue)(const Name& name) = nullptr;
  template <typename T> const std::vector<Name>& (*Enum<T>::GetNames) (void) = nullptr;

} // namespace noz


#endif //__noz_Enum_h__

