///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Networking/IPAddress.h>
#include <noz/Networking/Dns.h>
#include <noz/Networking/TcpClient.h>
#include <noz/IO/BinaryReader.h>
#include <noz/IO/BinaryWriter.h>
#include "RemoteGraphicsContext.h"
#include "RemoteClientView.h"
#include "RemoteFrame.h"
#include "RemoteMessage.h"
#include "RemoteArchive.h"

using namespace noz;
using namespace noz::Networking;

/// Time in seconds for the remote client to timeout and reset connection.
static const noz_float RemoteClientTimeout = 5.0f;

/// Check for new connetions every second.
static const noz_float RemoteClientConnectionRetry = 1.0f;

/// Port to connect client to
static const noz_int32 RemoteClientServerPort = 7878;

RemoteClientView::RemoteClientView(void) : archive_(this) {
  server_port_ = 7878;
  client_ = nullptr;
  frame_ = nullptr;
  client_ = new TcpClient;
  waiting_image_ack_ = false;
}

RemoteClientView::~RemoteClientView(void) {
  Disconnect();
  delete client_;
}

void RemoteClientView::Connect (void) {
  // Close any existing connection.
  client_->Close();

  receive_time_ = Time::GetApplicationTime();
  waiting_image_ack_ = false;

  // Async Connect
  client_->Connect(IPAddress::Parse(server_address_.ToCString()),server_port_);
}

void RemoteClientView::Disconnect(void) {
  if(client_->IsDisconnected()) return;

  if(client_->IsConnected()) AssetManager::RemoveArchive(&archive_);

  client_->Close();

  waiting_image_ack_ = false;
  delete frame_;
  frame_ = nullptr;
}

void RemoteClientView::Update (void) {
  RemoteView::Update();

  noz_float elapsed = Time::GetApplicationTime()-receive_time_;

  bool is_connected = client_->IsConnected();

  // Has it been too long since the server sent us anything?
  if(is_connected && elapsed > RemoteClientTimeout) Disconnect();

  // Continually attempt new connections...
  if(!client_->IsConnected() && elapsed > RemoteClientConnectionRetry) Connect();

  // If disconnected we are done
  if(client_->IsDisconnected()) return;  

  // Update client state
  client_->Update ();

  // Check to make sure the client did not disconnect
  if(client_->IsDisconnected()) return;  

  // If connection state changed add the archive
  if(!is_connected && client_->IsConnected()) AssetManager::AddArchive(&archive_,false);

  // If not yet connected we are done
  if(client_->IsConnecting()) return;

  // Send system input events back to server...
  for(noz_uint i=0,c=Input::GetTouchCount(); i<c; i++) {
    const Touch& touch = Input::GetTouch(i);
    if(touch.GetTimestamp() != Time::GetTime()) continue;

    SystemEventType et = SystemEventType::TouchBegan;
    switch(touch.GetPhase()) {
      case TouchPhase::Began: et = SystemEventType::TouchBegan; break;
      case TouchPhase::Moved: et = SystemEventType::TouchMoved; break;
      case TouchPhase::Ended: et = SystemEventType::TouchEnded; break;
      case TouchPhase::Cancelled: et = SystemEventType::TouchCancelled; break;
    }

    RemoteMessage* msg = new RemoteMessage(RemoteMessageType::SystemEvent, nullptr, 0);
    BinaryWriter writer(msg->GetStream());
    writer.WriteByte((noz_byte)et);
    writer.WriteUInt64(touch.GetId());
    writer.WriteFloat(touch.GetPosition().x);
    writer.WriteFloat(touch.GetPosition().y);
    writer.WriteByte((noz_byte)touch.GetTapCount());
    PutMessage(msg);
  }

  for(RemoteMessage* msg = GetMessage(); msg; msg=GetMessage()) {
    switch(msg->GetMessageType()) {
      case RemoteMessageType::Frame: {
        delete frame_;
        frame_ = RemoteFrame::FromMessage(msg);

        noz_uint64 id = frame_->GetId();
        PutMessage(new RemoteMessage(RemoteMessageType::FrameAck, (noz_byte*)&id, sizeof(noz_uint64)));
        break;
      }

      case RemoteMessageType::GetImageAck: {
        Guid guid = BinaryReader(msg->GetStream()).ReadGuid();

        waiting_image_ack_ = false;

        auto it = pending_assets_.find(guid);
        if(it != pending_assets_.end()) {
          pending_assets_.erase(it);
        }

        if(msg->GetStream()->GetLength()-sizeof(Guid) == 0) break;

        String path = Path::Combine(Environment::GetFolderPath(SpecialFolder::Cache), 
          String::Format("RemoteAssets/%02X/%s.nozasset", (noz_byte)(guid.GetHighOrder()>>56), guid.ToString().ToCString())
        );  
        FileStream fs;
        if(fs.Open(path,FileMode::Truncate)) {
          noz_byte* temp = new noz_byte[msg->GetStream()->GetLength() - sizeof(Guid)];
          msg->GetStream()->Read((char*)temp, 0, msg->GetStream()->GetLength() - sizeof(Guid));
          fs.Write((char*)temp,0,msg->GetStream()->GetLength() - sizeof(Guid));
          fs.Close();
          delete[] temp;         
        }
        break;
      }
    }

    delete msg;
  }

  if(!waiting_image_ack_ && !pending_assets_.empty()) {
    // Send a message to retrieve the image
    RemoteMessage* msg = new RemoteMessage(RemoteMessageType::GetImage, nullptr, 0);
    BinaryWriter(msg->GetStream()).WriteGuid(*pending_assets_.begin());
    PutMessage(msg);
    waiting_image_ack_ = true;
  }

  ProcessMessageQueue();
}

void RemoteClientView::RenderOverride (RenderContext* rc) {
  RemoteView::RenderOverride(rc);

  if(nullptr == frame_) return;

  rc->PushMatrix();

  RenderMesh mesh;

  frame_->SeekBegin();

  for(RemoteFrame::Command cmd = frame_->ReadCommand();
      cmd != RemoteFrame::Command::Unknown;
      cmd = frame_->ReadCommand()) {
    switch(cmd) {
      case RemoteFrame::Command::SetTransform: {
        rc->PopMatrix();
        rc->PushMatrix();
        rc->MultiplyMatrix(frame_->ReadMatrix());
        break;
      }

      case RemoteFrame::Command::DrawRenderMesh: {
        mesh.Clear();
        rc->PushOpacity(frame_->ReadFloat());
        frame_->ReadRenderMesh(mesh);
        rc->Draw(&mesh);
        rc->PopOpacity();
        break;
      }

    }
  }

  rc->PopMatrix();
}

Stream* RemoteClientView::OpenAssetFile (const Guid& guid) {
  // Is the asset being retrieved?
  if(pending_assets_.find(guid) != pending_assets_.end()) return nullptr;

  String path = Path::Combine(Environment::GetFolderPath(SpecialFolder::Cache), 
    String::Format("RemoteAssets/%02X/%s.nozasset", (noz_byte)(guid.GetHighOrder()>>56), guid.ToString().ToCString())
   );    

  FileStream* fs = File::CreateStream(path, FileMode::Open);
  if(nullptr != fs) return fs;

  pending_assets_.insert(guid);

  return nullptr;
}
