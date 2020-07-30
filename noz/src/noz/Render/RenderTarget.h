///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RenderTarget_h__
#define __noz_RenderTarget_h__

#include "Image.h"

namespace noz {

  namespace Platform { class RenderTargetHandle; }

  class RenderTarget {
    private: ObjectPtr<Image> image_;

    private: Platform::RenderTargetHandle* handle_;

    public: RenderTarget(void);

    public: ~RenderTarget (void);

    public: void SetImage (Image* image);

    public: Image* GetImage (void) const {return image_;}

    public: Platform::RenderTargetHandle* GetHandle(void) const {return handle_;}
  };

} // namespace noz


#endif //__noz_RenderTarget_h__

