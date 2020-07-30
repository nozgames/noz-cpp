///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_StyleSheet_h__
#define __noz_StyleSheet_h__

namespace noz {

  class Style;

  class StyleSheet : public Asset {
    NOZ_OBJECT(Managed)

    /// Styles contained in the sheet
    NOZ_PROPERTY(Name=Styles,Add=AddStyle)
    private: std::vector<ObjectPtr<Style>> styles_;

    /// Default constructor
    public: StyleSheet (void);

    /// Default destructor
    public: ~StyleSheet (void);

    public: Style* FindStyle (Type* type) const;    

    public: void AddStyle (Style* style);
  };

} // namespace noz


#endif // __noz_StyleSheet_h__

