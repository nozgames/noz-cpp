///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EditorEvents_h__
#define __noz_Editor_EditorEvents_h__

#include <noz/Event.h>

namespace noz {
namespace Editor {

  class AssetFile;

  class PropertyChangedEventArgs {
    /// Property which has changed
    private: Property* property_;

    /// Target the property changed on
    private: Object* target_;

    public: PropertyChangedEventArgs(Property* prop, Object* target) {
      property_ = prop;
      target_ = target;
    }

    public: Property* GetProperty (void) const {return property_;}
    public: Object* GetTarget (void) const {return target_;}
  };

  typedef Event<PropertyChangedEventArgs*> PropertyChangedEventHandler;

  typedef Event<UINode*,Asset*> AssetSelectedEventHandler;
  typedef Event<AssetFile*> AssetRenamedEvent;
  typedef Event<UINode*,Type*> TypeSelectedEventHandler;

} // namespace EDitor
} // namespace noz


#endif // __noz_Editor_EditorEvents_h__

