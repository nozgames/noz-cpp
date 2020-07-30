///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DialogBox_h__
#define __noz_DialogBox_h__

namespace noz {

  enum class DialogBoxResult {
    Unknown,
    Cancel,
    No,
    None,
    Ok,
    Yes
  };

  class DialogBox : public Control {
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=Title,Set=SetTitle) String title_;

    private: DialogBoxResult result_;

    public: DialogBoxResult DoModal (Window* parent);
    
    public: DialogBox(void);

    public: void SetTitle (const char* v);
    public: void SetTitle (const String& v) {SetTitle(v.ToCString());}

    protected: void EndDialog (DialogBoxResult result);
  };

} // namespace noz


#endif //__noz_DialogBox_h__

