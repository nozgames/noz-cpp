///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_FPSMeter_h__
#define __noz_FPSMeter_h__

namespace noz {

  class TextNode;

  class FPSMeter : public Control {
    NOZ_OBJECT(DefaultStyle="{5E7A82B9-F405-4035-92A7-5C8B53067FFC}")

    NOZ_CONTROL_PART(Name=TextNode)
    private: ObjectPtr<TextNode> text_node_;

    private: noz_float elapsed_;

    NOZ_PROPERTY(Name=UpdateInterval)
    private: noz_float update_interval_;

    public: FPSMeter(void);

    public: virtual void Update(void) override;

    private: void UpdateText(void);
  };

} // namespace noz


#endif // __noz_FPSMeter_h__

