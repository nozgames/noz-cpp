///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ProgressBar_h__
#define __noz_ProgressBar_h__

namespace noz {

  class LayoutTransform;

  class ProgressBar : public Control {
    NOZ_OBJECT(DefaultStyle="{C32BD633-3DE2-4F1A-AC72-FB804B03D5A3}")

    NOZ_CONTROL_PART(Name=ProgressTransform)
    private: ObjectPtr<LayoutTransform> progress_transform_;

    private: NOZ_PROPERTY(Name=Maximum,Set=SetMaximum) noz_float maximum_;
    private: NOZ_PROPERTY(Name=Minimum,Set=SetMinimum) noz_float minimum_;
    private: NOZ_PROPERTY(Name=Value,Set=SetValue) noz_float value_;
    private: NOZ_PROPERTY(Name=Step,Set=SetStep) noz_float step_;

    public: ProgressBar(void);

    public: void SetMinimum (noz_float v);
    public: void SetMaximum (noz_float v);
    public: void SetStep (noz_float v);
    public: void SetValue (noz_float v);

    public: void PerformStep (void);

    protected: virtual bool OnApplyStyle (void) override;

    private: void UpdateProgressTransform (void);
  };

} // namespace noz


#endif // __noz_ProgressBar_h__

