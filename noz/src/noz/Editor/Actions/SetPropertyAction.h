///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SetPropertyAction_h__
#define __noz_Editor_SetPropertyAction_h__

#include "Action.h"

namespace noz {
namespace Editor {

  class SetPropertyAction : public Action {
    NOZ_OBJECT()

    protected: ObjectPtr<Object> target_;
    protected: Property* property_;

    public: SetPropertyAction (Object* target, Property* property) : target_(target), property_(property) { 
      noz_assert(target_);
      noz_assert(property_);
    }

    public: Object* GetTarget (void) const {return target_;}

    public: Property* GetTargetProperty (void) const {return property_;}

    public: virtual bool CanMerge (Action* action) const final {
      noz_assert(action);
      if(action->GetType() != GetType()) return false;
      SetPropertyAction* merge = (SetPropertyAction*)action;
      if(merge->target_ != target_) return false;
      if(merge->property_ != property_) return false;
      return true;
    }    
  };

  template <typename PT, typename VT> class SetPropertyActionT : public SetPropertyAction {
    NOZ_TEMPLATE()

    protected: VT value_;
    protected: VT undo_value_;

    public: SetPropertyActionT (Object* t, Property* p, const VT& v) : SetPropertyAction (t, p) {
      noz_assert(t);
      noz_assert(p);
      noz_assert(p->IsTypeOf(typeof(PT)));

      value_ = v;
      undo_value_ = ((PT*)p)->Get(t);
    }

    /// Execute the action
    public: virtual void Do (void) final {
      noz_assert(target_);
      noz_assert(property_);

      ((PT*)property_)->Set(target_, value_);

      PropertyChangedEventArgs args(property_, target_);
      EditorApplication::PropertyChanged(&args);
    }

    /// Execute the action
    public: virtual void Undo (void) final {
      noz_assert(target_);
      noz_assert(property_);

      ((PT*)property_)->Set(target_, undo_value_);

      PropertyChangedEventArgs args(property_, target_);
      EditorApplication::PropertyChanged(&args);
    }

    public: virtual void Merge (Action* action) final {
      noz_assert(action);
      noz_assert(action->GetType() == GetType());

      value_ = ((SetPropertyActionT*)action)->value_;
    }

    public: const VT& GetValue(void) const {return value_;}
  };

  class SetColorPropertyAction : public SetPropertyActionT<ColorProperty,Color> {
    NOZ_OBJECT()
    public: SetColorPropertyAction(Object* t, Property* p, Color v) : SetPropertyActionT<ColorProperty,Color> (t,p,v) {}
  };

  class SetEnumPropertyAction : public SetPropertyActionT<EnumProperty,Name> {
    NOZ_OBJECT()
    public: SetEnumPropertyAction(Object* t, Property* p, const Name& v) : SetPropertyActionT(t,p,v) {}
  };

  class SetFloatPropertyAction : public SetPropertyActionT<FloatProperty,noz_float> {
    NOZ_OBJECT()
    public: SetFloatPropertyAction(Object* t, Property* p, noz_float v) : SetPropertyActionT(t,p,v) {}
  };

  class SetVector2PropertyAction : public SetPropertyActionT<Vector2Property,Vector2> {
    NOZ_OBJECT()
    public: SetVector2PropertyAction(Object* t, Property* p, const Vector2& v) : SetPropertyActionT(t,p,v) {}
  };

  class SetObjectPtrPropertyAction : public SetPropertyActionT<ObjectPtrProperty,Object*> {
    NOZ_OBJECT()
    public: SetObjectPtrPropertyAction(Object* t, Property* p, Object* v) : SetPropertyActionT(t,p,v) {}
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_SetPropertyAction_h__
