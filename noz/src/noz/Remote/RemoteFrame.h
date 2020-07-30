///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteFrame_h__
#define __noz_RemoteFrame_h__

namespace noz {

  class RemoteMessage;

  class RemoteFrame : public Object {
    public: enum class Command {
      Unknown,
      Asset,
      SetTransform,
      DrawRenderMesh
    };

    public: typedef std::map<noz_uint64, Asset*> AssetMap;

    /// Stream used to write frame data
    private: MemoryStream stream_;

    /// Assets included in this frame.
    private: AssetMap assets_;

    private: noz_uint64 frame_id_;

    public: RemoteFrame(noz_uint64 frame_id);

    public: const AssetMap& GetAssets(void) const {return assets_;}

    public: noz_uint64 GetId(void) const {return frame_id_; }

    public: void WriteRectangle(const Rect& rect);
    public: void WriteCommand(Command cmd);
    public: void WriteMatrix(const Matrix3& mat);
    public: void WriteColor(Color color);
    public: void WriteRenderMesh (const RenderMesh& mesh);
    public: void WriteFloat (noz_float v);
    
    public: Stream* GetStream (void) {return &stream_;}

    public: Command ReadCommand(void);
    public: Rect ReadRectangle(void);
    public: Matrix3 ReadMatrix(void);
    public: Color ReadColor(void);
    public: void ReadRenderMesh (RenderMesh& mesh);
    public: noz_float ReadFloat (void);

    public: void SeekBegin(void);

    public: RemoteMessage* ToMessage (void);

    public: static RemoteFrame* FromMessage (RemoteMessage* msg);
  };

} // namespace noz


#endif //__noz_RemoteFrame_h__


