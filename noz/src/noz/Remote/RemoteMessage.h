///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteMessage_h__
#define __noz_RemoteMessage_h__

#include <noz/Networking/TcpClient.h>

namespace noz {

  enum class RemoteMessageType {
    Unknown,
    Asset,
    Validate,         /// Validate a connection to a remote server.
    ValidateAck,      /// Ack returned by server to signify the connection is valid.
    Frame,
    FrameAck,
    GetImage,
    GetImageAck,
    SystemEvent
  };

  class RemoteMessage : public Object {
    /// Raw message bytes
    private: MemoryStream* stream_;

    private: noz_uint32 written_;

    private: RemoteMessageType type_;

    public: RemoteMessage (RemoteMessageType type, noz_byte* bytes, noz_int32 size);

    public: Stream* GetStream(void) { return stream_; }

    public: RemoteMessageType GetMessageType(void) const {return type_;}

    /// Send message to client.
    public: bool Send(Networking::TcpClient* client);
  };

} // namespace noz


#endif //__noz_RemoteMessage_h__


