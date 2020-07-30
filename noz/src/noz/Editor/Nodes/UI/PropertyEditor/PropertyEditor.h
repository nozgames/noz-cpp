///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_PropertyEditor_h__
#define __noz_Editor_PropertyEditor_h__

namespace noz {
namespace Editor {

  class PropertyEditor : public Control {
    NOZ_OBJECT(Abstract)

    public: ValueChangedEventHandler ValueChanged;

    private: ObjectPtr<Object> target_;

    private: ObjectPtr<Property> target_property_;

    protected: bool writing_property_;

    protected: bool reading_property_;

    public: PropertyEditor (void);

    public: ~PropertyEditor (void);

    public: virtual bool IsExpander (void) const {return false;}

    public: void SetTarget (Object* target, Property* prop);

    public: Object* GetTarget (void) const {return target_;}

    public: Property* GetTargetProperty (void) const {return target_property_;}

    protected: virtual void WriteProperty (Object* target, Property* prop) = 0;

    protected: virtual void ReadProperty (Object* target, Property* prop) = 0;

    protected: void WriteProperty (void);

    protected: void ReadProperty (void);

    protected: bool IsReadingProperty (void) const {return reading_property_;}

    protected: bool IsWritingProperty (void) const {return writing_property_;}

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnSetTarget (void) {}

    private: void OnPropertyChanged (PropertyChangedEventArgs* args);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_PropertyEditor_h__
