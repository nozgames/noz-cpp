///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteClientView_h__
#define __noz_RemoteClientView_h__

#include "RemoteView.h"
#include "RemoteArchive.h"

namespace noz {

  class RemoteFrame;
  class RemoteArchive;

  class RemoteClientView : public RemoteView {
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=ServerAddress) String server_address_;
    private: NOZ_PROPERTY(Name=ServerPort) noz_int32 server_port_;

    /// Current cached frame.
    private: RemoteFrame* frame_; 

    private: RemoteArchive archive_;

    private: std::set<Guid> pending_assets_;

    private: bool waiting_image_ack_;

    public: RemoteClientView (void);

    public: ~RemoteClientView (void);

    public: Stream* OpenAssetFile (const Guid& guid);
   
    protected: virtual void Update (void) override;
    protected: virtual void RenderOverride (RenderContext* rc) override;

    private: void Connect (void);
    private: void Disconnect (void);
  };
  
} // namespace noz


#endif // __noz_RemoteClientView_h__

