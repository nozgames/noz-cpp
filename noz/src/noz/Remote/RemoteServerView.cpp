///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Networking/IPAddress.h>
#include <noz/Networking/Dns.h>
#include <noz/Networking/TcpClient.h>
#include <noz/Networking/TcpListener.h>
#include <noz/IO/BinaryReader.h>
#include <noz/IO/BinaryWriter.h>
#include "RemoteGraphicsContext.h"
#include "RemoteServerView.h"
#include "RemoteFrame.h"

using namespace noz;
using namespace noz::Networking;

/// Time in seconds for the remote client to timeout and reset connection.
static const noz_float RemoteClientTimeout = 1.0f;

/// Check for new connetions every second.
static const noz_float RemoteClientConnectionRetry = 1.0f;

/// Port to connect client to
static const noz_int32 RemoteClientServerPort = 7878;


RemoteServerView::RemoteServerView(void) {
  port_ = 7878;
  server_frame_ = 0;
  client_frame_ = 0;
  elapsed_frame_ = 0;
  listener_ = nullptr;
  client_ = nullptr;
}

RemoteServerView::~RemoteServerView(void) {
}

void RemoteServerView::Connect (void) {
  // Get the local host name.
  String name = Networking::Dns::GetHostName();

  // Get the local host entry.
  IPHostEntry he = Networking::Dns::GetHostEntry(name.ToCString());

  // Cannot start the remote server without the local address
  if(he.GetAddresses().empty()) return;

  listener_ = new Networking::TcpListener(he.GetAddresses()[0], port_);
  if(!listener_->Start()) {
    delete listener_;
    listener_ = nullptr;
    return;
  }
}

void RemoteServerView::Update (void) {
  RemoteView::Update();

  if(nullptr == listener_) return;

  // Look for new clients coming in.. If a new client comes in the old client is replaced.
  if(listener_->IsPending()) {
    delete client_;
    client_ = listener_->AcceptTcpClient();
    if(client_) {
      remote_assets_.clear();
      server_frame_ = 0;
      client_frame_ = 0;
      elapsed_frame_ = 0;
    }
  }

  // No active client?
  if(nullptr == client_) return;

  // Process all pending messages.
  for(RemoteMessage* msg = GetMessage(); msg; msg = GetMessage()) {    
    switch(msg->GetMessageType()) {
      case RemoteMessageType::GetImage: {
        Guid guid = BinaryReader(msg->GetStream()).ReadGuid();
        Image* image = AssetManager::LoadAsset<Image>(guid);
        if(image) {
          RemoteMessage* ack = new RemoteMessage(RemoteMessageType::GetImageAck, nullptr, 0);
          BinaryWriter(ack->GetStream()).WriteGuid(guid);
          BinarySerializer().Serialize(image, ack->GetStream());
          PutMessage(ack);
        } else {
          RemoteMessage* ack = new RemoteMessage(RemoteMessageType::GetImageAck, nullptr, 0);
          BinaryWriter(ack->GetStream()).WriteGuid(guid);
          PutMessage(ack);
        }
        break;
      }

      case RemoteMessageType::FrameAck:
        msg->GetStream()->Read((char*)&client_frame_, 0, sizeof(noz_uint64));
        break;

      case RemoteMessageType::SystemEvent: {
        BinaryReader reader(msg->GetStream());
        SystemEventType et = (SystemEventType)reader.ReadByte();
        switch(et) {
          case SystemEventType::TouchBegan:
          case SystemEventType::TouchEnded:
          case SystemEventType::TouchCancelled:
          case SystemEventType::TouchMoved: {
            noz_uint64 id = reader.ReadUInt64();
            Vector2 position;
            position.x = reader.ReadFloat();
            position.y = reader.ReadFloat();
            noz_uint32 tap_count = reader.ReadUInt32();
            SystemEvent::PushEvent(et,GetWindow(),id,position,tap_count);
            break;
          }
        }
        break;
      }
    }        

    delete msg;
  }

  ProcessMessageQueue();
}

void RemoteServerView::RenderOverride (RenderContext* rc) {
  RemoteView::RenderOverride(rc);

  // If there is no connected client then nothing else to do.
  if(client_ == nullptr) return;

  // Track the amount of time that has elapsed since a frame was sent
  elapsed_frame_ += Time::GetDeltaTime();

  // Time to send another frame?
  if(elapsed_frame_ > 1.0f / 60.0f && server_frame_ - client_frame_ < 5) {
    while(elapsed_frame_ > 1.0f / 60.0f) elapsed_frame_ -= (1.0f / 60.0f);
    server_frame_++;

    // Render a single frame.
    RemoteFrame f(server_frame_);
    RemoteGraphicsContext rrc(&f);
    RenderContext local_rc(&rrc);
    RemoteView::Render(&local_rc);
    PutMessage(f.ToMessage());
  }
}
