///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteServerView_h__
#define __noz_RemoteServerView_h__

#include "RemoteView.h"

namespace noz {

  class RemoteServerView : public RemoteView {
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=Port) noz_int32 port_;
    
    /// Listener for connecting to a new client..
    private: Networking::TcpListener* listener_;

    /// Set of all assets that are remote for the current client.
    private: std::set<noz_uint64> remote_assets_;

    /// Frame number the client is on.
    private: noz_uint64 client_frame_;

    /// Frame number the server is on
    private: noz_uint64 server_frame_;

    /// Elapsed time since last frame was sent
    private: noz_float elapsed_frame_;

    public: RemoteServerView (void);

    public: ~RemoteServerView (void);

    public: void Connect (void);

    protected: virtual void Update (void) override;
    protected: virtual void RenderOverride (RenderContext* rc) override;
  };

} // namespace noz

#endif // __noz_RemoteServerView_h__

