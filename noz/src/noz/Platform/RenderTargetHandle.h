///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_RenderTargetHandle_h__
#define __noz_Platform_RenderTargetHandle_h__

namespace noz { class RenderTarget; }

namespace noz {
namespace Platform { 

  class RenderTargetHandle : public Handle {
    public: static RenderTargetHandle* CreateInstance(RenderTarget* target);

  };

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_RenderTargetHandle_h__

