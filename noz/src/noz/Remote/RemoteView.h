///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteView_h__
#define __noz_RemoteView_h__

namespace noz { namespace Networking { class TcpListener; class TcpClient; } }

#include "RemoteMessage.h"

namespace noz {

  class RemoteView : public UINode {
    NOZ_OBJECT()
    
    protected: noz_int32 receive_size_;
    protected: RemoteMessageType receive_type_;
    protected: MemoryStream receive_stream_;
    protected: Networking::TcpClient* client_;

    /// Last time data was received from the client.
    protected: noz_float receive_time_;

    private: std::vector<RemoteMessage*> send_queue_;

    public: RemoteView(void);

    public: ~RemoteView(void);

    protected: RemoteMessage* GetMessage (void);

    protected: void PutMessage (RemoteMessage* msg);

    protected: void ProcessMessageQueue (void);
  };

} // namespace noz


#endif // __noz_RemoteView_h__

